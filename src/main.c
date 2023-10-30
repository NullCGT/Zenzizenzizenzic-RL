/**
 * @file main.c
 * @author Kestrel (kestrelg@kestrelscry.com)
 * @brief The main loop and functions necessary for running the game.
 * @version 1.0
 * @date 2022-05-27
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#include <stdlib.h>
#include <locale.h>
#include <signal.h>
#include <stdio.h>
#include <execinfo.h>
#include <unistd.h>
#include <string.h>

#include "ai.h"
#include "invent.h"
#include "map.h"
#include "register.h"
#include "windows.h"
#include "message.h"
#include "render.h"
#include "mapgen.h"
#include "random.h"
#include "action.h"
#include "save.h"
#include "spawn.h"
#include "parser.h"
#include "version.h"

void handle_exit(void);
void handle_sigwinch(int);
void new_game(void);
void parse_args(int, const char *[]);

/**
 * @brief Called whenever the program exits. Cleans up the screen and
 frees used memory.
 * 
 */
void handle_exit(void) {
    int freed;
    if (g.debug)
        printf("Freeing message list...\n");
    freed = free_message_list(g.msg_list);
    if (g.debug) {
        printf("Freed %d messages.\n", freed);
        printf("Freeing actor list...\n");
    }
    freed = free_actor_list(g.player);
    if (g.debug)
        printf("Freed %d actors.\n", freed);
    if (term.saved_locale != NULL) {
        if (g.debug) printf("Restoring locale...\n");
        setlocale (LC_ALL, term.saved_locale);
        free(term.saved_locale);
        if (g.debug) printf("Locale restored.\n");
    }
    printf("Team %s will return...\n", g.userbuf);
    return;
}

/**
 * @brief Handle instances of the terminal being resized. Currently saves and quits.
 * 
 * @param sig Signal.
 */
void handle_sigwinch(int sig) {
    (void) sig;
    if (g.turns)
        save_game();
    cleanup_screen();
    exit(0);
    return;
}


/**
 * @brief Handle segmentation faults and dump a meaningful backtrace.
 This is pulled directly from a stack overflow post by user Todd Gamblin.
   https://stackoverflow.com/questions/77005/how-to-automatically-generate-a-stacktrace-when-my-program-crashes
 * 
 * @param sig Signal.
 */
void handle_sigsegv(int sig) {
    void *array[16];
    int size = backtrace(array, 16);
    (void) sig;
    cleanup_screen();
    fprintf(stderr, "Error: signal %d:\n", sig);
    backtrace_symbols_fd(array, size, STDERR_FILENO);
    printf("\nWell, that's embarassing. The game appears to have suplexed itself.");
    printf("\nPlease report any bugs at %s.\n", REPO_URL);
    exit(1);
}

/**
 * @brief Set up a new game.
 * 
 */
void new_game(void) {
    json_to_monster_list("data/creature/creatures.json");
    json_to_item_list("data/item/weapons.json");
    if (f.mode_explore || g.debug) {
        logm_warning("The high score list is disabled due to the game mode.");
    }
    /* Spawn player */
    if (g.player == NULL) {
        g.player = spawn_named_creature("zenzi", 0, 0);
        g.player->unique = 1;
        g.active_attacker = g.player;
    }
    /* Make level */
    make_level();
    /* Put player in a random spot */
    push_actor(g.player, g.up_x, g.up_y);
    /* Once we are all done, set up the gui. */
    setup_gui();
    logma(CYAN, "Welcome, Team %s! Let's rock!", g.userbuf);
}

/**
 * @brief Parse the given arguments
 * 
 * @param argc Number of arguments
 * @param argv Argument array
 */
void parse_args(int argc, const char *argv[]) {
    for (int i = 1; i < argc; i++) {
        if (!strncmp(argv[i], "-X", 2)) {
            g.explore = 1;
        } else if (!strncmp(argv[i], "-D", 2)) {
            g.debug = 1;
        } else if (!strncmp(argv[i], "-u", 2)) {
            snprintf(g.userbuf, sizeof(g.userbuf), "%s", argv[i] + 2);
        }
    }
}

/**
 * @brief Main function
 * 
 * @param argc Number of arguments
 * @param argv Argument array
 * @return int 0
 */
int main(int argc, const char *argv[]) {
    struct actor *cur_actor;
    char buf[MAX_USERSZ + 4] = { '\0' };

    /* handle exits and resizes */
    atexit(handle_exit);
    signal(SIGWINCH, handle_sigwinch);
    signal(SIGSEGV, handle_sigsegv);

    // Parse args
    parse_args(argc, argv);
    if (g.userbuf[0] == '\0')
        getlogin_r(g.userbuf, sizeof(g.userbuf));
    
    /* Build savefile name */
    snprintf(buf, sizeof(buf), "%s.sav", g.userbuf);

    // Seed the rng
    rndseed_t();

    // Set up the screen
    setup_screen();
    title_screen();
    if (file_exists(buf)) {
        load_game(buf);
        logma(CYAN, "Welcome back, Team %s! It's go time!", g.userbuf);
    } else {
        new_game();
    }
    
    /* Main Loop */
    cur_actor = g.player;
    render_all();
    while (1) {
        while (cur_actor != NULL) {
            take_turn(cur_actor);
            cur_actor = cur_actor->next;
            if (cur_actor == NULL)
                cur_actor = g.player;
        }
    }
    cleanup_screen();
    exit(0);
    return 0;
}