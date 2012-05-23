#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "spot.h"
#include "despotify/despotify.h"

static void filename_clean(char *filename, size_t size) {
  char *cursor;

  /* Ensure every whitespace character is a space */
  for (cursor = filename; *cursor; cursor++)
    if (isspace(*cursor))
      *cursor = ' ';

  /* Replace each slash with " - " */
  while ((cursor = strrchr(filename, '/'))) {
    filename[size - 3] = 0;
    memmove(cursor + 2, cursor, strlen(cursor) + 1);
    memcpy(cursor, " - ", 3);
  }

  /* Remove leading, duplicate and trailing whitespace */
  while (*filename == ' ')
    memmove(filename, filename + 1, strlen(filename));
  while ((cursor = strstr(filename, "  ")))
    memmove(cursor, cursor + 1, strlen(cursor));
  while (filename[strlen(filename) - 1] == ' ')
    filename[strlen(filename - 1)] = 0;
}

static void filename_create(char *filename, size_t size, struct track *track,
    struct album_browse *album) {
  char *cursor;

  printf("Track %02d: %s (%d:%02d at %d kbit/s)\n", track->tracknumber,
      track->title, track->length/60000, (track->length % 60000)/1000,
      track->file_bitrate/1000);

  if (album) {
    snprintf(filename, size, "%s - %s", album->artist, album->name);
    filename_clean(filename, size);
    mkdir(filename, 0777);
    snprintf(filename + strlen(filename), size - strlen(filename),
        "/%02d: %s.ogg", track->tracknumber, track->title);
    if ((cursor = strchr(filename, '/')))
      filename_clean(cursor + 1, size + filename - cursor - 1);
  } else {
    snprintf(filename, size, "%s.ogg", track->title);
    filename_clean(filename, size);
  }
}

static void gain(char *filename, bool album) {
  int pid;

  switch (pid = fork()) {
    case -1:
      error(EXIT_FAILURE, errno, "fork");
    case 0:
      execlp("vorbisgain", "vorbisgain", album ? "-afqs" : "-fqs",
          filename, NULL);
      error(EXIT_FAILURE, errno, "Warning: failed to run vorbisgain");
    default:
      waitpid(pid, NULL, 0);
  }
}

static void get_track(char *filename, struct track *track) {
  FILE *out;
  char album_uri[37] = "spotify:album:", track_uri[37], tmpname[PATH_MAX];
  int fd[2], pid, status;
  struct artist *artist;

  despotify_id2uri(track->album_id, album_uri + strlen(album_uri));
  despotify_track_to_uri(track, track_uri);

  if (!access(filename, F_OK)) {
    printf("%38s already downloaded: skipping\n", track_uri);
    return;
  } else if (!track->playable) {
    printf("%38s not playable: skipping\n", track_uri);
    return;
  }

  snprintf(tmpname, sizeof(tmpname), "%s.tmp", filename);
  if (!(out = fopen(tmpname, "wb")))
    error(EXIT_FAILURE, errno, "Unable to create file '%s'", tmpname);

  fetch(track, out);
  fclose(out);

  switch (pipe(fd), pid = fork()) {
    case -1:
      error(EXIT_FAILURE, errno, "fork");
    case 0:
      dup2(fd[0], STDIN_FILENO);
      close(fd[0]);
      close(fd[1]);
      execlp("vorbiscomment", "vorbiscomment", "-aqR", tmpname,
          filename, NULL);
      error(EXIT_FAILURE, errno, "Warning: failed to run vorbiscomment");
    default:
      close(fd[0]);
      dprintf(fd[1], "TITLE=%s\n", track->title);
      for (artist = track->artist; artist; artist = artist->next)
        dprintf(fd[1], "ARTIST=%s\n", artist->name);
      dprintf(fd[1], "ALBUM=%s\n", track->album);
      dprintf(fd[1], "TRACKNUMBER=%d\n", track->tracknumber);
      dprintf(fd[1], "DATE=%d\n", track->year);
      dprintf(fd[1], "SPOTIFY_ALBUM=%s\n", album_uri);
      dprintf(fd[1], "SPOTIFY_TRACK=%s\n", track_uri);
      close(fd[1]);
      waitpid(pid, &status, 0);
      if (status)
        rename(tmpname, filename);
      else
        unlink(tmpname);
  }
}

int get_uri(int argc, char **argv) {
  char filename[PATH_MAX];
  struct album_browse *album;
  struct link *link;
  struct track *track, *next;

  for (size_t i = 0; i < argc; i++) {
    link = despotify_link_from_uri(argv[i]);
    switch (link->type) {
      case LINK_TYPE_ALBUM:
        if (!(album = despotify_link_get_album(ds, link))) {
          error(0, 0, "Album '%s' not found", argv[i]);
          continue;
        }
        printf("Album: %s - %s\n", album->artist, album->name);

        for (track = album->tracks; track; track = next) {
          next = track->next;
          filename_create(filename, sizeof(filename), track, album);
          get_track(filename, track);
        }
        despotify_free_album_browse(album);
        if (strchr(filename, '/')) {
          *strchr(filename, '/') = 0;
          gain(filename, true);
        }
        break;

      case LINK_TYPE_TRACK:
        if (!(track = despotify_link_get_track(ds, link))) {
          error(0, 0, "Track '%s' not found", argv[i]);
          continue;
        }
        filename_create(filename, sizeof(filename), track, NULL);
        get_track(filename, track);
        despotify_free_track(track);
        gain(filename, false);
        break;

      default:
        error(0, 0, "Link '%s' not supported", argv[i]);
    }
    despotify_free_link(link);
  }
  return EXIT_SUCCESS;
}
