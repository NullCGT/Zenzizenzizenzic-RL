/**
 * @file gameover.c
 * @author Kestrel (kestrelg@kestrelscry.com)
 * @brief Functions called when the game has ended, whether due to a loss or a win.
 * @version 1.0
 * @date 2022-05-26
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "register.h"
#include "message.h"
#include "windows.h"
#include "gameover.h"

int write_dumplog(const char *, int);
void dump_target(FILE *);
void dump_levmap(FILE *);
void dump_messages(FILE *);
void dump_inventory(FILE *);

/**
 * @brief The action by which the player manually quits the game.
 * 
 * @return int The cost in actions of quitting the game (always 0).
 */
int do_quit(void) {
    if (!yn_prompt("Give up? THIS CANNOT BE UNDONE.", 0)) {
        logm("That's right, there's still hope!");
        return 0;
    }
    logm("Failure...");
    g.target = NULL;
    end_game(0);
    return 0;
}

/**
 * @brief End the game.
 * 
 * @param winner Whether the game was won or lost.
 1 if the game was won.
 0 if the game was lost.
 */
void end_game(int winner) {
    if (!write_dumplog("dumplog.txt", winner)
        && yn_prompt("View the game summary?", 1)) {
        display_file_text("dumplog.txt");
    }
    cleanup_screen();
    exit(0);
}

/**
 * @brief Write a dumplog to a file.
 * 
 * @param fname The name of the file to dump the log to.
 * @param winner Whether the game was lost.
 1 if the game was won.
 0 if the game was lost.
 * @return int Whether the file was successfully opened.
 1 if we could not acquire a file pointer.
 0 if the file was opened successfully.
 */
int write_dumplog(const char *fname, int winner) {
    FILE *fp;
    fp = fopen(fname, "w");
    if (!fp) {
        return 1;
    }

    fputs("== Final Statics ==\n", fp);
    fprintf(fp, "The active fighter was %s, fighting for Team %s.\n", actor_name(g.player, 0), g.userbuf);
    if (winner) {
        fprintf(fp, "Team %s won on turn %d.\n", g.userbuf, g.turns);
    } else if (g.target) {
        fprintf(fp, "%s was KO'd by %s on turn %d.\n", actor_name(g.player, NAME_CAP), actor_name(g.target, NAME_A), g.turns);
    } else {
        fprintf(fp, "Team %s gave up on turn %d.\n", g.userbuf, g.turns);
    }
    fprintf(fp, "The team scored %d points.\n", g.score);
    fprintf(fp, "%s had %d health, with a maximum of %d.\n", actor_name(g.player, NAME_CAP), g.player->hp, g.player->hpmax);
    fprintf(fp, "The team was on level %d of the facility.\n", g.depth);
    if (g.depth <= g.max_depth) {
        fprintf(fp, "The team was in unknown territory.\n");
    } else {
        fprintf(fp, "The team journeyed even deeper previously, and had reached level %d.\n", g.max_depth);
    }

    if (g.target) {
        dump_target(fp);
    }

    dump_messages(fp);
    dump_inventory(fp);
    dump_levmap(fp);
    fclose(fp);
    return 0;
}

/**
 * @brief Dump information about the creature that killed the player.
 * 
 * @param fp The dumplog file.
 */
void dump_target(FILE *fp) {
    struct actor *cur = g.target->invent;

    fputs("\n== Target Statistics ==\n", fp);
    fprintf(fp, "Name: %s\n", actor_name(g.target, NAME_CAP));
    fprintf(fp, "HP: (%d/%d)\n", g.target->hp, g.target->hpmax);

    if (cur != NULL) {
        fprintf(fp, "Possessions:\n");
    }
    while (cur != NULL) {
        fprintf(fp, " %s\n", actor_name(cur, NAME_CAP | NAME_A));
        cur = cur->next;
    }
}

/**
 * @brief Dump a textual representation of the current level.
 * 
 * @param fp The dumplog file.
 */
void dump_levmap(FILE *fp) {
    fprintf(fp, "\n== Level %d ==\n", g.depth);
    for (int y = 0; y < MAPH; y++) {
        for (int x = 0; x < MAPW; x++) {
            if (g.levmap[x][y].actor)
                fputc(g.levmap[x][y].actor->chr, fp);
            else if (g.levmap[x][y].item_actor)
                fputc(g.levmap[x][y].item_actor->chr, fp);
            else
                fputc(g.levmap[x][y].pt->chr, fp);
        }
        fputc('\n', fp);
    }
}

/**
 * @brief Dump all messages contained in the game's memory.
 * 
 * @param fp The dumplog file.
 */
void dump_messages(FILE *fp) {
    struct msg *cur_msg = g.msg_last;
    fputs("\n== Last Recorded Messages ==\n", fp);
    while (cur_msg != NULL) {
        fprintf(fp, "(%d) %s\n", cur_msg->turn, cur_msg->msg);
        cur_msg = cur_msg->prev;
    }
}

/**
 * @brief Dump a record of all items in the player's inventory.
 * 
 * @param fp The dumplog file.
 */
void dump_inventory(FILE *fp) {
    struct actor *cur = g.player->invent;

    fputs("\n== Inventory ==\n", fp);
    if (!cur) {
        fprintf(fp, "A bit of dust.\n");
        return;
    }
    while (cur != NULL) {
        fprintf(fp, "%s\n", actor_name(cur, NAME_CAP | NAME_A | NAME_EX | NAME_EQ));
        cur = cur->next;
    }

}