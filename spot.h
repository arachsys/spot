#ifndef SPOT_H
#define SPOT_H

#include "despotify/despotify.h"

extern struct despotify_session *ds;

void error(int status, int errnum, char *format, ...);
void fetch(struct track *track, FILE *out);
bool login(char *user, char *pass);

int find_match(int argc, char **argv);
int cat_uri(int argc, char **argv);
int get_uri(int argc, char **argv);
int list_uri(int argc, char **argv);

#endif
