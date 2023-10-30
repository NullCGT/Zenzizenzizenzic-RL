#ifndef INVENT_H
#define INVENT_H

#define MAX_INVENT_SIZE 26

enum slot_enum {
    SLOT_HEAD,
    SLOT_BACK,
    SLOT_TORSO,
    SLOT_LEGS,
    SLOT_WEP,
    SLOT_OFF,
    SLOT_FEET
};

#define NO_SLOT -1
#define MAX_SLOTS SLOT_FEET + 1

struct item {
    struct actor *parent;
    /* Equip slots */
    signed char slot;     /* Equip slot */
    signed char pref_slot; /* Preferred slot */
    short poss_slot; /* Possible slot */
    /* Inventory */
    int quan;               /* Quantity */
    int letter;       /* Previous letter used if dropped */
    /* bitfields */
};

struct equip {
    struct actor *parent;
    struct actor *slots[MAX_SLOTS];
};

struct slot_type {
    int id;
    short field;
    const char *slot_name;
    const char *slot_desc;
    const char *on_msg;
    const char *off_msg;
};

extern struct slot_type slot_types[MAX_SLOTS];

#define EHEAD(actor) (actor->equip ? actor->equip->slots[SLOT_HEAD] : NULL)
#define EBACK(actor) (actor->equip ? actor->equip->slots[SLOT_BACK] : NULL)
#define ETORSO(actor) (actor->equip ? actor->equip->slots[SLOT_TORSO] : NULL)
#define ELEGS(actor) (actor->equip ? actor->equip->slots[SLOT_LEGS] : NULL)
#define EWEP(actor) (actor->equip ? actor->equip->slots[SLOT_WEP] : NULL)
#define EOFF(actor) (actor->equip ? actor->equip->slots[SLOT_OFF] : NULL)
#define EFEET(actor) (actor->equip ? actor->equip->slots[SLOT_FEET] : NULL)

#define is_hat(actor) (actor->item && actor->item->pref_slot == SLOT_WEP)
#define is_cloak(actor) (actor->item && actor->item->pref_slot == SLOT_BACK)
#define is_shirt(actor) (actor->item && actor->item->pref_slot == SLOT_TORSO)
#define is_pants(actor) (actor->item && actor->item->pref_slot == SLOT_LEGS)
#define is_weapon(actor) (actor->item && actor->item->pref_slot == SLOT_WEP)
#define is_shield(actor) (actor->item && actor->item->pref_slot == SLOT_OFF)
#define is_shoes(actor) (actor->item && actor->item->pref_slot == SLOT_FEET)

#define is_equipped(actor) (actor->item && actor->item->slot != NO_SLOT)

/* Function Prototypes */
struct item *init_item(struct actor *);
struct equip *init_equip(struct actor *);
int add_to_invent(struct actor *, struct actor *);
int display_invent(void);

#endif