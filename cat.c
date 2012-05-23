#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "spot.h"
#include "despotify/despotify.h"

static void announce(struct track *track) {
  fprintf(stderr, "Track %02d: %s (%d:%02d at %d kbit/s)\n",
      track->tracknumber, track->title, track->length/60000,
      (track->length % 60000)/1000, track->file_bitrate/1000);
}

int cat_uri(int argc, char **argv) {
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
        fprintf(stderr, "Album: %s - %s\n", album->artist, album->name);

        for (track = album->tracks; track; track = next) {
          next = track->next;
          announce(track);
          fetch(track, stdout);
        }
        despotify_free_album_browse(album);
        break;

      case LINK_TYPE_TRACK:
        if (!(track = despotify_link_get_track(ds, link))) {
          error(0, 0, "Track '%s' not found", argv[i]);
          continue;
        }
        announce(track);
        fetch(track, stdout);
        despotify_free_track(track);
        break;

      default:
        error(0, 0, "Link '%s' not supported", argv[i]);
    }
    despotify_free_link(link);
  }
  return EXIT_SUCCESS;
}
