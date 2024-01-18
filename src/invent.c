/**
 * @file invent.c
 * @author Kestrel (kestrelg@kestrelscry.com)
 * @brief Inventory and item-related functionality.
 * @version 1.0
 * @date 2022-05-27
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#include <stdlib.h>
#include <string.h>

#include "actor.h"
#include "invent.h"
#include "windows.h"
#include "message.h"
#include "menu.h"
#include "register.h"
#include "spawn.h"
#include "ai.h"
#include "combat.h"

void clean_item_slots(struct actor *, struct actor *);
struct actor *win_pick_invent(void);
int win_use_item(struct actor *);
int win_extequip_item(struct actor *);
int equip_item(struct actor *, struct actor *, int);
int drop_item(struct actor *, struct actor *);
int throw_item(struct actor *, struct actor *);
int takeoff_item(struct actor *, struct actor *);

struct slot_type slot_types[MAX_SLOTS] = {
    { SLOT_HEAD,  0x0001,  "head",    "(on head)",  "puts on",  "takes off" },
    { SLOT_BACK,  0x0002,  "back",    "(on back)",  "puts on",  "shrugs off" },
    { SLOT_TORSO, 0x0004,  "torso",   "(on torso)", "dons",     "takes off" },
    { SLOT_LEGS,  0x0008,  "legs",    "(on legs)",  "puts on",  "takes off" },
    { SLOT_WEP,   0x0010,  "weapon",  "(wielded)",  "wields",   "stops wielding" },
    { SLOT_OFF,   0x0020,  "offhand", "(offhand)",  "equips",   "stops using" },
    { SLOT_FEET,  0x0040,  "feet",    "(on feet)",  "pulls on", "takes off" }
};

/**
 * @brief Allocate memory for the item component of a given actor
 * 
 * @param actor The parent of the new item component.
 * @return A pointer to the newly-created item struct.
 */
struct item *init_item(struct actor *actor) {
    struct item *new_item = (struct item *) malloc(sizeof(struct item));
    *new_item = (struct item) { 0 };
    new_item->parent = actor;
    new_item->letter = 'a';
    new_item->quan = 1;
    new_item->slot = NO_SLOT;
    actor->item = new_item;
    return actor->item;
}

/**
 * @brief Allocate memory for the equip component of a given actor
 * 
 * @param actor The parent of the new equip component.
 * @return A pointer to the newly-created equip struct.
 */
struct equip *init_equip(struct actor *actor) {
    struct equip *new_equip = (struct equip *) malloc(sizeof(struct equip));
    *new_equip = (struct equip) { 0 };
    new_equip->parent = actor;
    actor->equip = new_equip;
    return new_equip;
}

/**
 * @brief Add an item to a given inventory. Letters are "sticky," in that
if you drop an item, it should have the same letter when you
pick it back up. If there is a collision, the letter is reassigned.
Returns 1 if item was successfully added.
Returns 0 on failure.
 * 
 * @param creature The creature whose inventory is being appended to.
 * @param item The actor being added to the inventory.
 * @return int Success.
0 if unsuccessful.
1 if successful.
 */
int add_to_invent(struct actor *creature, struct actor *item) {
    struct actor *cur = creature->invent;
    struct actor *prev = creature->invent;
    int found_spot = 0;

    char letters[26] = "abcdefghijklmnopqrstuvwyz";
    
    /* Transcend list, removing used letters. */
    while (cur != NULL) {
        prev = cur;
        letters[cur->item->letter - 'a'] = 0;
        cur = cur->next;
    }
    /* Reassign letter if necessary. */
    if (!letters[item->item->letter - 'a']) {
        for (int i = 0; i < 26; i++) {
            if (letters[i]) {
                item->item->letter = letters[i];
                found_spot = 1;
                break;
            }
        }
        if (!found_spot) return 0;
    }
    /* Insert the item */
    if (prev != NULL) {
        prev->next = item;
    } else {
        creature->invent = item;
    }
    return 1;
}

/**
 * @brief Internal function that resets an item's slot information and
resets the actor wielding that item's equip information.
 * 
 * @param actor the actor wielding the item.
 * @param item the item actor.
 */
void clean_item_slots(struct actor *actor, struct actor *item) {
    actor->equip->slots[item->item->slot] = NULL;
    item->item->slot = NO_SLOT;
    if (actor == g.player) {
        g.active_attack_index = 0;
        g.active_attacker = g.player;
    }
}

/**
 * @brief Perform an action with an item picked from the inventory.
 * 
 * @return The costi n energy of the performed action. 
 */
int display_invent(void) {
    int ret = 0;
    struct actor *item;
    item = win_pick_invent();
    if (item)
        ret = win_use_item(item);
    return ret;
}

/**
 * @brief Remove an item from the inventory of an actor. The caller is
 expected to handle the item afterward and prevent it from being orphaned.
 * 
 * @param holding_actor The actor holding the item.
 * @param held_actor The actor representing the item.
 * @return int Rteturn 1 if called in error, 0 otherwise.
 */
int remove_from_invent(struct actor *holding_actor, struct actor *held_actor) {
    struct actor *cur = holding_actor->invent;
    struct actor *prev = NULL;
    int found_item = 0;

    /* Remove the item from the inventory. */
    while (cur != NULL) {
        if (cur == held_actor) {
            found_item = 1;
            break;
        }
        prev = cur;
        cur = cur->next;
    }
    if (!found_item) {
        logm_warning("Error: Attempting to remove an item from an inventory it is not present in?");
        return 1;
    }
    if (prev != NULL) prev->next = cur->next;
    else {
        holding_actor->invent = cur->next;
    }
    clean_item_slots(holding_actor, held_actor);
    /* Caller MUST take care of adding item to master and pushing, otherwise it will be orphaned. */
    return 0;
}

/**
 * @brief Set up the inventory menu and choose an item.
 * 
 * @return struct actor* The item chosen from the inventory menu.
 */
struct actor *win_pick_invent(void) {
    struct menu *selector;
    struct actor *cur = g.player->invent;
    int selected = -1;

    if (cur == NULL) {
        logm("%s is not carrying anything.", actor_name(g.player, NAME_CAP));
        return NULL;
    }

    selector = menu_new("Inventory", term.mapwin_x, term.mapwin_y, term.mapwin_w, term.mapwin_h);
    while (cur != NULL) {
        menu_add_item(selector, cur->item->letter, actor_name(cur, NAME_EQ | NAME_EX));
        cur = cur->next;
    }
    while (1) {
        selected = menu_do_choice(selector, 1);
        if (selected == -1) {
            cur = NULL;
            break;
        }
        cur = g.player->invent;
        while (cur != NULL) {
            if (cur->item->letter == selected) {
                menu_destroy(selector);
                return cur;
            }
            cur = cur->next;
        }
    }
    menu_destroy(selector);
    return cur;
}

/**
 * @brief Display the item use menu.
 * 
 * @param item The item to be used.
 * @return int The energy cost of the action performed with the
 item.
 */
int win_use_item(struct actor *item) {
    struct menu *selector;
    int selected = -1;
    int equipped = is_equipped(item);

    selector = menu_new(actor_name(item, NAME_A | NAME_CAP), term.mapwin_x, term.mapwin_y, term.mapwin_w, term.mapwin_h);
    menu_add_item(selector, 'd', "drop");
    menu_add_item(selector, 'w', is_weapon(item) ? "wield" : "wear");
    if (equipped)
        menu_add_item(selector, 'r', "remove");
    menu_add_item(selector, 'e', "extended equip");
    menu_add_item(selector, 'n', "rename");
    menu_add_item(selector, 't', "throw");

    while (1) {
        selected = menu_do_choice(selector, 1);
        switch (selected) {
            case 'd':
                menu_destroy(selector);
                return drop_item(g.player, item);
            case 'w':
                menu_destroy(selector);
                return equip_item(g.player, item, item->item->pref_slot);
            case 'r':
                if (equipped) {
                    menu_destroy(selector);
                    return takeoff_item(g.player, item);
                } else {
                    break;
                }
            case 'e':
                selected = win_extequip_item(item);
                if (selected != -1)
                    menu_destroy(selector);
                return selected;
            case 'n':
                text_entry("What should this item be named?", item->name->given_name, MAXNAMESIZ);
                return 0;
            case 't':
                menu_destroy(selector);
                return throw_item(g.player, item);
            case -1:
                menu_destroy(selector);
                return 0;
        }
    }
    menu_destroy(selector);
    return 0;
}

/**
 * @brief Display the menu for equipping an item in an odd slot.
 * 
 * @param item The actor representing the item to be equipped.
 * @return int The cost in energy of equipping the item, or -1 if it could not be done.
 */
int win_extequip_item(struct actor *item) {
    struct menu *selector;
    int selected = -1;
    int letter = 'a';

    selector = menu_new("Which slot?", 0, 0, term.mapwin_w, term.mapwin_h);

    for (int i = 0; i < MAX_SLOTS; i++) {
        menu_add_item(selector, letter++, slot_types[i].slot_name);
    }
    while (1) {
        selected = menu_do_choice(selector, 1);
        if (selected >= 'a' && selected <= 'z') {
            menu_destroy(selector);
            return equip_item(g.player, item, selected - 'a');
        } else if (selected == -1) {
            menu_destroy(selector);
            return -1;
        }
    }
}

/**
 * @brief Equip an item.
 * 
 * @param actor The actor doing the equipping.
 * @param item The actor representing the item to be equipped.
 * @param inslot The slot the item is to be equipped to.
 * @return int The cost of equipping an item in energy.
 */
int equip_item(struct actor *actor, struct actor *item, int inslot) {
    if (!actor->equip) {
        logm("%s cannot equip items%s",
             actor_name(actor, NAME_THE | NAME_CAP), 
             in_danger(actor) ? "!" : ".");
        return 0;
    }
    if (actor->equip->slots[inslot] == item) {
        logm("%s is already %s %s!",
             actor_name(actor, NAME_THE | NAME_CAP), 
             is_weapon(item) ? "wielding" : "wearing", actor_name(item, NAME_THE));
        return 0;
    }
    if (!(slot_types[inslot].field & item->item->poss_slot)) {
        /* TODO: Vary this message based on intelligence score. */
        logm("Equipping %s on the %s? %s",
             actor_name(item, NAME_THE), slot_types[inslot].slot_name,
             in_danger(actor) ? "That's ridiculous!" : "Be serious.");
        return 0;
    }

    if (actor->equip->slots[inslot]) {
        takeoff_item(actor, actor->equip->slots[inslot]);
    }
    if (is_equipped(item)) {
        takeoff_item(actor, actor->equip->slots[item->item->slot]);
    }
    actor->equip->slots[inslot] = item;
    item->item->slot = inslot;

    if (is_weapon(item) && item != g.active_attacker
        && (inslot == SLOT_WEP || inslot == SLOT_OFF)){
        g.active_attacker = item;
        g.active_attack_index = (inslot == SLOT_WEP ? MAX_ATTK : (MAX_ATTK * 2));
    }

    logm("%s %s %s.", actor_name(actor, NAME_THE | NAME_CAP),
                      in_danger(actor) ? "whips out" : slot_types[item->item->slot].on_msg, 
                      actor_name(item, NAME_A));
    return TURN_FULL;
}

/**
 * @brief Drop an item.
 * 
 * @param actor The actor dropping the item.
 * @param item The actor representing the item.
 * @return int The cost of dropping an item in energy.
 */
int drop_item(struct actor *actor, struct actor *item) {
    int x = actor->x;
    int y = actor->y;

    if (item->item->slot != NO_SLOT && item->item->slot != SLOT_OFF && item->item->slot != SLOT_WEP) {
        logm("%s tries to drop %s, but is still wearing it.",
              actor_name(actor, NAME_CAP | NAME_THE), actor_name(item, NAME_THE));
        return 0;
    }
    if (nearest_pushable_cell(item, &x, &y)) {
        logm("There is not enough room here to drop %s.", actor_name(item, NAME_THE));
        return 0;
    }

    logm("%s drops %s.", actor_name(actor, NAME_CAP | NAME_THE), actor_name(item, NAME_THE));
    remove_from_invent(actor, item);
    add_actor_to_main(item);
    push_actor(item, x, y);
    return 50;
}

/**
 * @brief Throw an item. This is seperate from actor knockback because items
 * need to be able to pass freely over other items.
 * Whenever an item bounces off a wall, it gains a damage multiplier.
 * 
 * @param actor The actor throwing the item.
 * @param item The actor being thrown.
 * @return int The cost of throwing an item in energy.
 */
int throw_item(struct actor *actor, struct actor *item) {
    /* TODO: Move getting directions outside of the function so that monsters
       can throw items */
    struct coord new_dir;
    struct actor *target;
    int nx, ny;
    int dmul = 1;
    int velocity = 5;

    new_dir = get_direction("throw");
    remove_from_invent(actor, item);
    nx = actor->x + new_dir.x;
    ny = actor->y + new_dir.y;
    logm("%s throws %s.", actor_name(actor, NAME_CAP | NAME_THE), actor_name(item, NAME_THE));
    
    while (velocity) {
        if (!in_bounds(nx, ny)) {
            break;
        }
        if (is_blocked(nx, ny)) {
           /* Projectile rebounds. Because why not? Rebounding a projectile increases its damage multiplier. */
           /* TODO: Damage the thrown item for each rebound in order to prevent extended juggles off a single item. */
            logm("%s bounces off the %s.", actor_name(item, NAME_CAP | NAME_THE), 
                                           g.levmap[nx][ny].pt->name);
            if (nx != item->x)
                new_dir.x = new_dir.x * -1;
            if (ny != item->y)
                new_dir.y = new_dir.y * -1;
            dmul += 1;
        }
        target = MON_AT(nx, ny);
        if (target) {
            do_attack(item, target, dmul);
            break;
        }
        item->x = nx;
        item->y = ny;
        nx += new_dir.x;
        ny += new_dir.y;
        velocity--;
    }
    add_actor_to_main(item);
    push_actor(item, nx, ny);
    return TURN_FULL;
}

int takeoff_item(struct actor *actor, struct actor *item) {
    if (!actor->equip) {
        logm("%s cannot remove items.", actor_name(actor, NAME_CAP | NAME_THE));
        return 0;
    }
    if (item->item->slot == NO_SLOT) {
        if (actor == g.player) {
            logm("Already done.");
        }
        return 0;
    }
    logm("%s %s%s %s.", 
            actor_name(actor, NAME_CAP | NAME_THE),
            in_danger(actor) ? "hastily " : "", 
            slot_types[item->item->slot].off_msg, actor_name(item, NAME_A));
    clean_item_slots(actor, item);
    return TURN_FULL;
}
