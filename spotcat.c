#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "despotify/despotify.h"
#include "despotify/sndqueue.h"

static struct despotify_session *ds = NULL;

void error(int status, int errnum, char *format, ...) {
  va_list args;
  va_start(args, format);
  vfprintf(stderr, format, args);
  va_end(args);
  if (errnum != 0)
    fprintf(stderr, ": %s\n", strerror(errnum));
  else
    fputc('\n', stderr);
  if (status != 0)
    exit(status);
}

bool login(char *user, char *pass) {
  static char *u = NULL, *p = NULL;
  u = user ? user : u;
  p = pass ? pass : p;

  if (ds)
    despotify_exit(ds);
  if (!(ds = despotify_init_client(NULL, NULL, true, false)))
    error(EXIT_FAILURE, 0, "Failed to initialize despotify");
  return despotify_authenticate(ds, u, p);
}

void finish(void) {
  if (ds)
    despotify_exit(ds);
  despotify_cleanup();
}

int callback(void *source, int bytes, void *private, int offset) {
  fwrite(source, 1, bytes, stdout);
  return bytes;
}

void fetch(struct track *t) {
  char buf[65536], uri[37];
  size_t length = 0, total = 0;

  fprintf(stderr, "Track %02d: %s (%d:%02d at %d kbit/s)\n", t->tracknumber,
      t->title, t->length/60000, (t->length % 60000)/1000,
      t->file_bitrate/1000);

  despotify_track_to_uri(t, uri);
  t->next = NULL;

  if (!t->playable) {
    fprintf(stderr, "%38s\tnot playable: skipping\n", uri);
    return;
  }

  despotify_play(ds, t, false);
  while (ds->track != NULL || length > 0) {
    for (int i = 0; !ds || !ds->fifo || !ds->fifo->start; i++)
      if (i > 100) {
        fprintf(stderr, "%38s\tstalled at %ld bytes\n", uri, total);
        exit(1);
      } else {
        const struct timespec delay = { 0, 100000000 };
        nanosleep(&delay, NULL);
      }
    snd_fill_fifo(ds);
    length = snd_consume_data(ds, sizeof(buf), NULL, callback);
    if (length > 0) {
      total += length;
      if (isatty(STDERR_FILENO))
        fprintf(stderr, "%38s\treceived %ld bytes\r", uri, total);
    }
  }

  fprintf(stderr, "%38s\tdownloaded %ld bytes\n", uri, total);
}

int get_uri(int argc, char **argv) {
  for (size_t i = 0; i < argc; i++) {
    struct link *l = despotify_link_from_uri(argv[i]);
    struct album_browse *a;
    struct track *t, *n;

    switch (l->type) {
      case LINK_TYPE_ALBUM:
        if (!(a = despotify_link_get_album(ds, l))) {
          error(0, 0, "Album '%s' not found", argv[i]);
          continue;
        }
        fprintf(stderr, "Album: %s - %s\n", a->artist, a->name);
        for (t = a->tracks; t; t = n) {
          n = t->next;
          fetch(t);
        }
        despotify_free_album_browse(a);
        break;

      case LINK_TYPE_TRACK:
        if (!(t = despotify_link_get_track(ds, l))) {
          error(0, 0, "Track '%s' not found", argv[i]);
          continue;
        }
        fetch(t);
        despotify_free_track(t);
        break;

      default:
        error(0, 0, "Link '%s' not supported", argv[i]);
    }
    despotify_free_link(l);
  }
  return EXIT_SUCCESS;
}

int main(int argc, char **argv) {
  setbuf(stdout, NULL);
  setbuf(stderr, NULL);
  atexit(finish);

  if (argc < 3) {
    fprintf(stderr, "Usage: %s USERNAME PASSWORD URI [URI]...\n", argv[0]);
    return EXIT_FAILURE;
  }

  if (!login(argv[1], argv[2]))
    error(EXIT_FAILURE, 0, "Authentication failed");
  return get_uri(argc - 3, argv + 3);
}
