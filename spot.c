#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "spot.h"
#include "despotify/despotify.h"

struct despotify_session *ds = NULL;

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

static void finish(void) {
  if (ds)
    despotify_exit(ds);
  despotify_cleanup();
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

static int usage(char *cmd) {
  fprintf(stderr, "\
Usage: %s COMMAND [ARGS]...\n\
Commands:\n\
  cat URI [URI]...\n\
  find [albums | tracks] SEARCH\n\
  get URI [URI]...\n\
  list URI [URI]...\n\
", cmd);
  return EXIT_FAILURE;
}

int main(int argc, char **argv) {
  char *user, *pass;

  if (argc < 3)
    return usage(argv[0]);

  user = getenv("SPOTIFY_USERNAME");
  pass = getenv("SPOTIFY_PASSWORD");
  if (!user || !pass)
    error(EXIT_FAILURE, 0, "Please set SPOTIFY_USERNAME and SPOTIFY_PASSWORD");

  setbuf(stdout, NULL);
  setbuf(stderr, NULL);
  atexit(finish);

  if (!login(user, pass))
    error(EXIT_FAILURE, 0, "Authentication failed");

  if (!strcmp(argv[1], "find") || !strcmp(argv[1], "search"))
    return find_match(argc - 2, argv + 2);
  else if (!strcmp(argv[1], "cat"))
    return cat_uri(argc - 2, argv + 2);
  else if (!strcmp(argv[1], "get"))
    return get_uri(argc - 2, argv + 2);
  else if (!strcmp(argv[1], "list") || !strcmp(argv[1], "ls"))
    return list_uri(argc - 2, argv + 2);

  return usage(argv[0]);
}
