#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "spot.h"
#include "despotify/despotify.h"
#include "despotify/sndqueue.h"

static int callback(void *source, int bytes, void *private, int offset) {
  fwrite(source, 1, bytes, (FILE *) private);
  return bytes;
}

void fetch(struct track *track, FILE *out) {
  FILE *msg = out == stdout ? stderr : stdout;
  char buffer[65536], uri[37];
  size_t length, retry, total;

  despotify_track_to_uri(track, uri);
  track->next = NULL;

start:
  length = total = 0;
  despotify_play(ds, track, false);
  while (ds->track != NULL || length > 0) {
    for (retry = 0; !ds || !ds->fifo || !ds->fifo->start; retry++)
      if (out != stdout && retry > 100) {
        fprintf(msg, "%38s\tstalled at %zu bytes\n", uri, total);
        while (!login(NULL, NULL)) {
          fprintf(msg, "%38s re-authentication failed: retrying in 5 seconds\n", uri);
          sleep(5);
        }
        rewind(out);
        goto start;
      } else {
        const struct timespec delay = { 0, 100000000 };
        nanosleep(&delay, NULL);
      }
    snd_fill_fifo(ds);
    length = snd_consume_data(ds, sizeof(buffer), out, callback);
    if (length > 0) {
      total += length;
      if (isatty(fileno(msg)))
        fprintf(msg, "%38s\treceived %zu bytes\r", uri, total);
    }
  }

  fprintf(msg, "%38s\tcompleted %zu bytes\n", uri, total);
  return;
}
