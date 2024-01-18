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
#include <argp.h>

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
error_t parse_args(int, char *, struct argp_state *);

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
    if (g.practice || g.debug) {
        logm("The high score list is disabled due to the game mode.");
    }
    /* Spawn player */
    if (g.player == NULL) {
        g.player = spawn_named_creature("zenzi", 0, 0);
        if (!g.player) panik("Failed to spawn player?");
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

static char doc[] = SHORT_DESC;
static struct argp_option options[] = {
    { "team",     't', "TEAMNAME", 0, "Set a default name for the player team.", 0},
    { "version",  'v', 0, 0, "Display version information.", 0},
    { "debug",    'd', 0, 0, "Activates debug mode. Debug mode enables debug commands and makes losing optional. Disables the high score list.", 0},
    { "practice", 'p', 0, 0, "Activates practice mode. Practice mode makes losing optional. Disables the high score list.", 0},
    {0}
};

struct arguments
{
    char *args[2];
    char *team;
    int debug, practice;
};
static struct argp argp = { options, parse_args, 0, doc, 0, 0, 0 };

error_t parse_args(int key, char *arg, struct argp_state *state) {
    struct arguments *arguments = state->input;

    switch (key) {
        case 'd':
            g.debug = 1;
            break;
        case 'p':
            g.practice = 1;
            break;
        case 'v':
            printf("v%d.%d.%d-%s (%s)\n", VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH, RELEASE_STATE, RELEASE_TYPE);
            exit(0);
            break;
        case 't':
            snprintf(g.userbuf, sizeof(g.userbuf), "%s", arg);
            break;
        case ARGP_KEY_ARG:
            if (state->arg_num >= 5)
                argp_usage(state);
            arguments->args[state->arg_num] = arg;
            break;
        default:
            return ARGP_ERR_UNKNOWN;
            exit(1);
            break;
    }
    return 0;
}

/**
 * @brief Main function
 * 
 * @param argc Number of arguments
 * @param argv Argument array
 * @return int 0
 */
int main(int argc, char **argv) {
    struct actor *cur_actor;
    char buf[MAX_USERSZ + 4] = { '\0' };

    // Parse args
    struct arguments arguments;
    arguments.debug = 0;
    arguments.practice = 0;
    arguments.team = '\0';
    argp_parse(&argp, argc, argv, 0, 0, &arguments);
    if (g.userbuf[0] == '\0')
        getlogin_r(g.userbuf, sizeof(g.userbuf));
    if (g.userbuf[0] == ' ')
        snprintf(g.userbuf, sizeof(g.userbuf), "Lion");
    if (g.userbuf[0] > 'Z')
        g.userbuf[0] = g.userbuf[0] - 32;

    /* handle exits and resizes */
    atexit(handle_exit);
    signal(SIGWINCH, handle_sigwinch);
    signal(SIGSEGV, handle_sigsegv);
    
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