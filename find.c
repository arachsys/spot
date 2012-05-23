#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "spot.h"
#include "despotify/despotify.h"

static int find_albums(char *match) {
  int count = 0;
  struct album *album;
  struct search_result *search;

  if ((search = despotify_search(ds, match, MAX_SEARCH_RESULTS))
         && search->playlist) {
    if (search->suggestion[0])
      fprintf(stderr, "Did you mean '%s'?\n", search->suggestion);
    if (search->total_albums > 0) {
      for (album = search->albums; album; count++, album = album->next) {
        char uri[37] = "spotify:album:";
        struct album_browse *ab = despotify_get_album(ds, album->id);
        despotify_id2uri(album->id, uri + strlen(uri));
        printf("%36s\t%s - %s [%d, %d tracks]\n", uri, album->artist,
            album->name, ab->year, ab->num_tracks);
      }
      if (count == search->total_albums)
        fprintf(stderr, "%d %s found\n", search->total_albums,
            search->total_albums == 1 ? "album": "albums");
      else
        fprintf(stderr, "%d albums found; returned first %d matches\n",
            search->total_albums, count);
    } else {
      fprintf(stderr, "No albums found\n");
    }
    return EXIT_SUCCESS;
  }
  return EXIT_FAILURE;
}

static int find_tracks(char *match) {
  int count = 0;
  struct search_result *search;
  struct track *track;

  if ((search = despotify_search(ds, match, MAX_SEARCH_RESULTS))
         && search->playlist) {
    if (search->suggestion[0])
      fprintf(stderr, "Did you mean '%s'?\n", search->suggestion);
    if (search->total_tracks > 0) {
      for (track = search->tracks; track; count++, track = track->next) {
        char album_uri[37] = "spotify:album:";
        char track_uri[37] = "spotify:track:";
        struct album_browse *album = despotify_get_album(ds, track->album_id);
        despotify_id2uri(track->album_id, album_uri + strlen(album_uri));
        despotify_id2uri(track->track_id, track_uri + strlen(track_uri));
        printf("%36s\t%s - %s [%d, %d tracks]\n", album_uri, album->artist,
            album->name, album->year, album->num_tracks);
        printf("%36s\t%02d: %s\n\n", track_uri, track->tracknumber,
            track->title);
      }
      if (count == search->total_tracks)
        fprintf(stderr, "%d %s found\n", search->total_tracks,
            search->total_tracks == 1 ? "track": "tracks");
      else
        fprintf(stderr, "%d tracks found; returned first %d matches\n",
            search->total_tracks, count);
    } else {
      fprintf(stderr, "No tracks found\n");
    }
    return EXIT_SUCCESS;
  }
  return EXIT_FAILURE;
}

int find_match(int argc, char **argv) {
  char match[1024];

  if (!strcmp(argv[0], "track") || !strcmp(argv[0], "tracks")) {
    if (++argv, --argc < 1)
      return EXIT_SUCCESS;
    snprintf(match, sizeof(match), "%s", argv[0]);
    for (size_t i = 1; i < argc; i++)
      snprintf(match + strlen(match), sizeof(match) - strlen(match), " %s", argv[i]);
    return find_tracks(match);
  }

  if (!strcmp(argv[0], "album") || !strcmp(argv[0], "albums"))
    if (++argv, --argc < 1)
      return EXIT_SUCCESS;
  snprintf(match, sizeof(match), "%s", argv[0]);
  for (size_t i = 1; i < argc; i++)
    snprintf(match + strlen(match), sizeof(match) - strlen(match), " %s", argv[i]);
  return find_albums(match);
}
