#ifndef PARSER_H
#define PARSER_H

/* Include so that we can parse wfc from json. */
#include "wfc.h"

/* Function Prototypes */
struct wfc_image parse_wfc_json(char *infile);
struct actor *actor_from_file(const char *);
void json_to_monster_list(const char *);
void json_to_item_list(const char *);

#endif