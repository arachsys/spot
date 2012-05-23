#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "spot.h"
#include "despotify/despotify.h"

int list_uri(int argc, char **argv) {
  char album_uri[37] = "spotify:album:", track_uri[37];
  struct album_browse *album;
  struct link *link;
  struct track *track;

  for (size_t i = 0; i < argc; i++) {
    link = despotify_link_from_uri(argv[i]);
    switch (link->type) {
      case LINK_TYPE_ALBUM:
        if (!(album = despotify_link_get_album(ds, link))) {
          error(0, 0, "Album '%s' not found", argv[i]);
          continue;
        }

        printf("%36s\t%s - %s [%d, %d tracks]\n", argv[i], album->artist,
            album->name, album->year, album->num_tracks);
        for (track = album->tracks; track; track = track->next) {
          despotify_track_to_uri(track, track_uri);
          printf("%36s\t%02d: %s\n", track_uri, track->tracknumber,
              track->title);
        }
        if (i + 1 < argc)
          putchar('\n');
        break;

      case LINK_TYPE_TRACK:
        if (!(track = despotify_link_get_track(ds, link))
              || !(album = despotify_get_album(ds, track->album_id))) {
          error(0, 0, "Track '%s' not found", argv[i]);
          continue;
        }

        despotify_id2uri(track->album_id, album_uri + strlen(album_uri));
        printf("%36s\t%s - %s [%d, %d tracks]\n", album_uri, album->artist,
            album->name, album->year, album->num_tracks);
        printf("%36s\t%02d: %s\n", argv[i], track->tracknumber, track->title);
        if (i + 1 < argc)
          putchar('\n');
        break;

      default:
        error(0, 0, "Link '%s' is not of a supported type", argv[i]);
    }
  }
  return EXIT_SUCCESS;
}
