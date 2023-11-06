/**
 * @file parser.c
 * @author Kestrel (kestrelg@kestrelscry.com)
 * @brief Functionality for parsing xml files.
 * @version 1.0
 * @date 2022-05-27
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <cjson/cJSON.h>

#include "message.h"
#include "parser.h"
#include "spawn.h"
#include "actor.h"
#include "ai.h"
#include "register.h"
#include "invent.h"
#include "random.h"
#include "spawn.h"

struct cJSON* json_from_file(const char *);
struct actor *actor_from_json(cJSON *);
struct ai *ai_from_json(struct ai *, cJSON *);
struct item *item_from_json(struct item *, cJSON *);
struct actor *actor_primitives_from_json(struct actor *, cJSON *);
struct actor *attacks_from_json(struct actor *, cJSON *);
void hitdescs_from_json(unsigned short *, cJSON *);
void color_from_json(unsigned char *, cJSON *);
void mod_slots(struct item *);

/**
 * @brief Read JSON from a file.
 * 
 * @param fname The name of the file to be read.
 * @return struct cJSON* A pointer to the cJSON struct. Returns NULL if 
 parsing was not possible.
 */
struct cJSON* json_from_file(const char *fname) {
    long len;
    FILE *fp;
    char *buf = NULL;
    cJSON *json = NULL;
    /* Read the file. */
    fp = fopen(fname, "rb");
    if (!fp) {
        logm_warning("Error: Could not find file: %s", fname);
        return NULL;
    }
    fseek(fp, 0, SEEK_END);
    len = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    buf = (char *) malloc(len + 1);
    fread(buf, 1, len, fp);
    buf[len] = '\0';
    fclose(fp);
    /* Parse the file into JSON. */
    json = cJSON_Parse(buf);
    if (!json) {
        logm_warning("Error: Could not parse JSON in file: %s", fname);
        return NULL;
    }
    free(buf);
    return json;
}
/**
 * @brief Parse an actor from a json object
 * 
 * @param actor_json The json object containing the actor
 * @return struct actor* The dynamically allocataed actor
 */
struct actor *actor_from_json(cJSON *actor_json) {
    cJSON *field = NULL;
    struct actor *actor;

    if (!actor_json)
        return NULL;
    actor = (struct actor *) malloc(sizeof(struct actor));
    if (!actor)
        return NULL;
    *actor = (struct actor) { 0 };
    
    /* Parse Fields */
    actor_primitives_from_json(actor, actor_json);
    field = cJSON_GetObjectItemCaseSensitive(actor_json, "attacks");
    attacks_from_json(actor, field);
    field = cJSON_GetObjectItemCaseSensitive(actor_json, "color");
    color_from_json(&(actor->color), field);

    /* Parse Components */
    field = cJSON_GetObjectItemCaseSensitive(actor_json, "ai");
    if (field) {
        init_ai(actor);
        ai_from_json(actor->ai, field);
    }
    field = cJSON_GetObjectItemCaseSensitive(actor_json, "item");
    if (field) {
        init_item(actor);
        item_from_json(actor->item, field);
    }
    field = cJSON_GetObjectItemCaseSensitive(actor_json, "equip");
    if (field) {
        init_equip(actor);
    }
    // Currently just hard enforce unique off any unique tag.
    field = cJSON_GetObjectItemCaseSensitive(actor_json, "unique");
    if (field)
        actor->unique = field->valueint;

    /* Fixups */
    if (actor->item)
        mod_slots(actor->item);
    
    /* Caller is responsible for freeing. */
    return actor;
}

void json_to_monster_list(const char *fname) {
    // comb through every file in the folder via looking through the master file
    int i = 0;
    cJSON *all_json = json_from_file(fname);
    cJSON *actor_json;
    struct actor *new_actor;

    if (!all_json) {
        panik("Could not read creatures: %s\n", fname);
        return;
    }

    // read each one into the actor array. must be freed at a later point.
    cJSON_ArrayForEach(actor_json, all_json) {
        if (g.total_monsters >= MAX_MONSTERS) {
            logm_warning("MAX_MONSTERS exceeded. Termination of game is recommended.");
            break;
        }
        new_actor = actor_from_json(actor_json);
        new_actor->id = g.total_monsters;
        g.monsters[i] = new_actor;
        i++;
        g.total_monsters++;
    }
    //cJSON_Delete(all_json);
    cJSON_Delete(actor_json);
}

void json_to_item_list(const char *fname) {
    int i = 0;
    cJSON *all_json = json_from_file(fname);
    cJSON *actor_json;
    struct actor *new_actor;

    if (!all_json) {
        panik("Could not read items: %s\n", fname);
        return;
    }

    // read each one into the actor array. must be freed at a later point.
    cJSON_ArrayForEach(actor_json, all_json) {
        if (g.total_items >= MAX_ITEMS) {
            logm_warning("MAX_ITEMS exceeded. Termination of game is recommended.");
            break;
        }
        new_actor = actor_from_json(actor_json);
        new_actor->id = g.total_items;
        g.items[i] = new_actor;
        i++;
        g.total_items++;
    }
    //cJSON_Delete(all_json);
    cJSON_Delete(actor_json);
}

/**
 * @brief Parse an ai struct from JSON.
 * 
 * @param ai A pointer to the ai struct.
 * @param ai_json A pointer to the JSON to be parsed.
 * @return struct ai* A pointer to the ai struct.
 */
struct ai *ai_from_json(struct ai *ai, cJSON *ai_json) {
    cJSON* field = NULL;
    field = cJSON_GetObjectItemCaseSensitive(ai_json, "seekdef");
    ai->seekdef = field->valueint;
    return ai;
}

/**
 * @brief Parse an item struct from JSON.
 * 
 * @param item A pointer to the item struct.
 * @param item_json A pointer to the JSON to be parsed.
 * @return struct item* A pointer to the item struct.
 */
struct item *item_from_json(struct item *item, cJSON *item_json) {
    cJSON* field = NULL;
    field = cJSON_GetObjectItemCaseSensitive(item_json, "pref_slot");
    if (field) {
        for (int i = 0; i < MAX_SLOTS; i++) {
            if (!strcmp(field->valuestring, slot_types[i].slot_name)) {
                item->pref_slot = slot_types[i].id;
            }
        }
    }
    return item;
}

/**
 * @brief Parse the primitives necessary for an actor from JSON.
 * 
 * @param actor A pointer to the actor struct to be modified
 * @param actor_json A pointer to the JSON to be parsed.
 * @return struct actor* A pointer to the modified actor struct.
 */
struct actor *actor_primitives_from_json(struct actor *actor, cJSON *actor_json) {
    cJSON* field = NULL;
    cJSON* field2 = NULL;
    field = cJSON_GetObjectItemCaseSensitive(actor_json, "name");
    field2 = cJSON_GetObjectItemCaseSensitive(actor_json, "appearance");
    if (field2)
        init_permname(actor, field->valuestring, field2->valuestring);
    else
        init_permname(actor, field->valuestring, NULL);
    field = cJSON_GetObjectItemCaseSensitive(actor_json, "chr");
    actor->chr = field->valuestring[0];
    field = cJSON_GetObjectItemCaseSensitive(actor_json, "hp");
    actor->hp = field->valueint;
    actor->hpmax = actor->hp;
    field = cJSON_GetObjectItemCaseSensitive(actor_json, "speed");
    actor->speed = field->valueint;

    field = cJSON_GetObjectItemCaseSensitive(actor_json, "evasion");
    if (!field)
        actor->evasion = 0;
    else
        actor->evasion = field->valueint;
    field = cJSON_GetObjectItemCaseSensitive(actor_json, "accuracy");
    if (!field)
        actor->accuracy = 0;
    else
        actor->accuracy = field->valueint;
    return actor;
}

/**
 * @brief Parse an array of attacks from JSON and assign them to an actor.
 * 
 * @param actor A pointer to the actor to be modified.
 * @param attacks_json A pointer to the JSON.
 * @return struct actor* A pointer to the modified actor.
 */
struct actor *attacks_from_json(struct actor *actor, cJSON *attacks_json) {
    cJSON* attack_json = NULL;
    cJSON* field = NULL;
    int i = 0;

    cJSON_ArrayForEach(attack_json, attacks_json) {
        if (i > MAX_ATTK) break;
        field = cJSON_GetObjectItemCaseSensitive(attack_json, "damage");
        actor->attacks[i].dam = field->valueint;
        field = cJSON_GetObjectItemCaseSensitive(attack_json, "kb");
        if (field)
            actor->attacks[i].kb = field->valueint;
        field = cJSON_GetObjectItemCaseSensitive(attack_json, "accuracy");
        actor->attacks[i].accuracy = field->valueint;
        field = cJSON_GetObjectItemCaseSensitive(attack_json, "types");
        hitdescs_from_json(&(actor->attacks[i].hitdescs), field);
        i++;
    }
    while (i < MAX_ATTK) {
        actor->attacks[i] = (struct attack) { 0 };
        i++;
    }
    return actor;
}

/**
 * @brief Parse damage type bitflags from JSON.
 * 
 * @param field A pointer to the field to be modified.
 * @param types_json A pointer to the JSON.
 */
void hitdescs_from_json(unsigned short *field, cJSON *types_json) {
    cJSON* hitdesc_json;

    cJSON_ArrayForEach(hitdesc_json, types_json) {
        for (int j = 0; j < MAX_HITDESC; j++) {
            if (!strcmp(hitdescs_arr[j].str, hitdesc_json->valuestring)) {
                *field |= hitdescs_arr[j].val;
            }
        }
    }
}


/**
 * @brief Parse color information from JSON.
 * 
 * @param color A pointer to the color to be modified.
 * @param color_json A pointer to the JSON.
 */
void color_from_json(unsigned char *color, cJSON *color_json) {
    for (int i = 0; i < MAX_COLOR; i++) {
        if (!strcmp(w_colors[i].str, color_json->valuestring)) {
            *color = w_colors[i].cnum;
        }
    }
}


/**
 * @brief Set up the item's preferred slots based on its type.
 * 
 * @param item The item to be mutated.
 */
void mod_slots(struct item *item) {
    if (!item)
        return;
    /* The preferred slot is always possible, as are hand slots. */
    item->poss_slot |= slot_types[item->pref_slot].field;
    item->poss_slot |= slot_types[SLOT_WEP].field;
    item->poss_slot |= slot_types[SLOT_OFF].field;
    /* All shields can be worn on the back for minor protection. */
    if (is_shield(item->parent) || is_weapon(item->parent))
        item->poss_slot |= slot_types[SLOT_BACK].field;
    /* Pants can be hats, if you try hard enough. */
    if (is_pants(item->parent))
        item->poss_slot |= slot_types[SLOT_HEAD].field;
    /* And the shirt off your back... */
    if (is_shirt(item->parent))
        item->poss_slot |= slot_types[SLOT_BACK].field;
}

/**
 * @brief Parse a wfc image for use in wfc generation from a json file.
 * 
 * @param infile The file to be parsed.
 * @return struct wfc_image The wfc image. 
 */
struct wfc_image parse_wfc_json(char *infile) {
    int width, height, offset = 0;
    unsigned char *wfcbuf = malloc(257 * sizeof(char));
    cJSON *wfc_json = json_from_file(infile);
    cJSON *test_json = NULL;
    cJSON *line = NULL;
    cJSON* field = NULL;
    struct wfc_image blank_image = {
        .data = wfcbuf,
        .component_cnt = 1,
        .width = 0,
        .height = 0
    };

    if (!wfc_json)
        return blank_image;
    test_json = cJSON_GetObjectItemCaseSensitive(wfc_json, "standard");
    
    field = cJSON_GetObjectItemCaseSensitive(test_json, "width");
    width = field->valueint;
    field = cJSON_GetObjectItemCaseSensitive(test_json, "height");
    height = field->valueint;
    field = cJSON_GetObjectItemCaseSensitive(test_json, "map");
    cJSON_ArrayForEach(line, field) {
        memcpy(wfcbuf + offset, (unsigned char *) line->valuestring, sizeof(line->valuestring));
        offset += width;
    }
    wfcbuf[height * width] = '\0';

    struct wfc_image image = {
        .data = wfcbuf,
        .component_cnt = 1,
        .width = width,
        .height = height
    };
    cJSON_Delete(test_json);
    return image;
}