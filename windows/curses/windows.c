/**
 * @file windows.c
 * @author Kestrel (kestrelg@kestrelscry.com)
 * @brief SCreen and window-related functions for the ncurses window port.
 * @version 1.0
 * @date 2022-05-27
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#define NCURSES_WIDECHAR 1
#include <curses.h>
#include <stdlib.h>
#include <stdio.h>
#include <locale.h>
#include <string.h>

#include "register.h"
#include "windows.h"
#include "render.h"
#include "message.h"
#include "action.h"
#include "map.h"
#include "invent.h"
#include "menu.h"
#include "ai.h"
#include "version.h"
#include "save.h"
#include "combat.h"

void setup_locale(void);
void setup_colors(void);
void popup_warning(const char *);
void print_hitdescs(WINDOW *, int, int, short, unsigned char);
void print_stance(struct actor *, WINDOW *, int, int);
void render_bar(WINDOW*, int, int, int, int, int, int);
int handle_mouse(void);
void curses_display_sb(WINDOW *);
void display_sb_nearby(WINDOW *, int *);
void display_sb_controls(WINDOW *, int *j);
void display_sb_stats(WINDOW *, int *, struct actor *);

#define MAX_FILE_LEN 200

struct curse_color {
    int r, g, b;
};

#define CHANGE_COLORS 0

struct zz_win map_win;
WINDOW *msg_win;
struct zz_win bars_win;
struct zz_win msgbox_win;
struct zz_win sb_win_left;
struct zz_win sb_win_right;

/* SCREEN FUNCTIONS */

void wcolor_on(WINDOW *win, unsigned char color) {
    wattron(win, COLOR_PAIR(color));
    if (color >= BRIGHT_COLOR)
        wattron(win, A_BOLD);
}

void wcolor_off(WINDOW *win, unsigned char color) {
    wattroff(win, COLOR_PAIR(color));
    if (color >= BRIGHT_COLOR)
        wattroff(win, A_BOLD);
}

void title_screen(void) {
    WINDOW *background;
    struct zz_win background_container;
    struct menu *selector;
    char buf[64];
    int selected = -1;

    background_container = create_win(term.h, term.w, 0, 0);
    background = background_container.win;
    for (int x = 0; x < term.w; x++) {
        for (int y = 0; y < term.h; y++) {
            mvwaddch(background, y, x, 'z');
        }
    }
    box(background, 0, 0);

    snprintf(buf, sizeof(buf), "Zenzizenzizenzic v%d.%d.%d-%s", 
             VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH, RELEASE_STATE);
    selector = menu_new(buf, 1, 1, 35, 8);

    menu_add_item(selector, 'p', "Play");
    menu_add_item(selector, 'd', "View Last Character");
    menu_add_item(selector, 'r', "Records");
    menu_add_item(selector, 'h', "Help");
    menu_add_item(selector, 'q', "Quit");

    while (1) {
        selected = menu_do_choice(selector, 0);
        switch (selected) {
            case 'p':
                menu_destroy(selector);
                cleanup_win(background_container);
                return;
            case 'd':
                display_file_text("dumplog.txt");
                break;
            case 'r':
                popup_warning("The high score list has not yet been implemented.");
                break;
            case 'h':
                display_file_text("data/text/help.txt");
                break;
            case 'q':
                menu_destroy(selector);
                cleanup_win(background_container);
                cleanup_screen();
                exit(0);
        }
    }
}

/**
 * @brief Perform the first-time setup for the game's GUI.
 * 
 */
void setup_gui(void) {
    map_win = create_win(term.mapwin_h, term.mapwin_w, term.mapwin_y, term.mapwin_x);
    bars_win = create_win(4, term.msg_w, 0, 0);
    msgbox_win = create_win(term.msg_h, term.msg_w, term.msg_y, 0);
    msg_win = newpad(term.h, term.msg_w);
    new_panel(msg_win);
    sb_win_left = create_win(term.sb_h, term.mapwin_x, term.mapwin_y, 0);
    sb_win_right = create_win(term.sb_h, term.sb_w, term.mapwin_y, term.sb_x);
    f.update_map = 1;
    draw_msg_window(0);
    draw_lifebars();
    wrefresh(map_win.win);
    update_panels();
    doupdate();
}

/**
 * @brief Set the locale of the terminal for the purposes of consistency, bug
reproducibility, and drawing special characters. The previous locale
is saved and reset upon game exit.
 * 
 */
void setup_locale(void) {
    char *old_locale;
    old_locale = setlocale(LC_ALL, NULL);
    term.saved_locale = strdup(old_locale);
    if (term.saved_locale == NULL)
        return;
    setlocale(LC_ALL, "en_US.UTF-8");
    return;
}

/**
 * @brief Set up the the scren of the game. In addition to creating the main window,
this initializes the screen, turns off echoing, and does the basic setup
needed for curses to do its job.
 * 
 */
void setup_screen(void) {
    int h, w;
    putenv("ESCDELAY=25");
    initscr();
    curs_set(0);
    getmaxyx(stdscr, h, w);
    if ((h >= MIN_TERM_H) && (w >= MIN_TERM_W)) {
        setup_term_dimensions(h, w, 1, 1);
    } else {
        cleanup_screen();
        printf("Terminal must be at least %dx%d.\n", MIN_TERM_W, MIN_TERM_H);
        exit(0);
    }
    if (has_colors()) {
        start_color();
        setup_colors();
    }
    setup_locale();
    noecho();
    raw();
    keypad(stdscr, 1);
    mousemask(ALL_MOUSE_EVENTS | REPORT_MOUSE_POSITION, NULL);
    mouseinterval(0);
    refresh();
}

/**
 * @brief Set up the color pairs necessary for rendering the game in color. This function
is only called if color is supported by the terminal.
 * 
 */
void setup_colors(void) {
    /* Set colors to desired shades if able. */
    #if CHANGE_COLORS
    /* TODO: Alter this so that it pulls from the global w_colors array. */
    if (can_change_color()) {
        for (int i = COLOR_BLACK; i <= COLOR_WHITE; i++) {
            if (i >= COLORS) break;
            init_color(i, colors[i].r, colors[i].g, colors[i].b);
        }
        
    }
    #endif
    /* Initialize color pairs */
    for (int i = COLOR_BLACK; i < MAX_COLOR; i++) {
        if (i < BRIGHT_COLOR)
            init_pair(i, i, COLOR_BLACK);
        else
            init_pair(i, i - BRIGHT_COLOR, COLOR_BLACK);
    }
    return;
}

/**
 * @brief The counterpart to setup_screen(). Called at the end of the program, and
used to clean up curses artifacts.
 * 
 */
void cleanup_screen(void) {
    endwin();
    return;
}

/* WINDOW MANAGEMENT FUNCTIONS */

/**
 * @brief Create a new window. Wrapper for curses function newwin.
 * 
 * @param h height.
 * @param w width.
 * @param y y coordinate.
 * @param x x coordinate.
 * @return The created window and panel struct.
 */
struct zz_win create_win(int h, int w, int y, int x) {
    WINDOW* win;
    PANEL* panel;
    struct zz_win ret;

    win = newwin(h, w, y, x);
    panel = new_panel(win);
    ret.win = win;
    ret.panel = panel;
    return ret;
}

/**
 * @brief Clean up a window by erasing, refreshing, and deleting it.
 * 
 * @param win Window to be cleaned up.
 */
void cleanup_win(struct zz_win win) {
    werase(win.win);
    del_panel(win.panel);
    delwin(win.win);
    update_panels();
    doupdate();
    return;
}

/**
 * @brief Display a warning message popup.
 * 
 * @param text The warning message.
 */
void popup_warning(const char *text) {
    WINDOW *new_win;
    PANEL *panel;
    int keycode;

    new_win = newwin(3, strlen(text) + 2, 1, 1);
    panel = new_panel(new_win);
    wcolor_on(new_win, RED);
    box(new_win, 0, 0);
    wcolor_off(new_win, RED);
    mvwprintw(new_win, 1, 1, "%s", text);
    update_panels();
    doupdate();

    while ((keycode = handle_keys())) {
        if (keycode == 27)
            break;
    }
    
    del_panel(panel);
    delwin(new_win);
    doupdate();
}

/**
 * @brief A text entry prompt.
 * 
 * @param prompt What to prompt the player with.
 * @param buf The buffer to write to.
 * @param bufsiz The size of the buffer to write to.
 */
void text_entry(const char *prompt, char *buf, int bufsiz) {
    WINDOW *new_win;
    struct zz_win new_win_container;
    int keycode;
    int index = 0;
    bufsiz = bufsiz - 1;

    new_win_container = create_win(term.mapwin_h, term.mapwin_w, term.mapwin_y, term.mapwin_x);
    new_win = new_win_container.win;

    while (buf[index] != '\0') {
        index++;
    }
    wcolor_on(new_win, MAGENTA);
    box(new_win, 0, 0);
    wcolor_off(new_win, MAGENTA);
    mvwprintw(new_win, 1, 1, "%s", prompt);
    mvwprintw(new_win, 3, 1, "%s", buf);
    update_panels();
    doupdate();

    while ((keycode = handle_keys())) {
        if (keycode >= 32 && keycode <= 'z' && index < bufsiz) {
            buf[index++] = keycode;
        } else if (keycode == '\b') {
            index--;
            buf[index] = '\0';
        } else if (keycode == '\n') {
            break;
        } else if (keycode == 27) {
            buf[0] = '\0';
            break;
        }
        if (index < 0) index = 0;
        if (index > bufsiz) index = bufsiz - 1;
        werase(new_win);
        wcolor_on(new_win, MAGENTA);
        box(new_win, 0, 0);
        wcolor_off(new_win, MAGENTA);
        mvwprintw(new_win, 1, 1, "%s", prompt);
        mvwprintw(new_win, 3, 1, "%s", buf);
        wrefresh(new_win);
    }
    cleanup_win(new_win_container);
}

/**
 * @brief Display the text of a file in a scrollable pad.
 * 
 * @param fname Filename to be displayed.
 */
void display_file_text(const char *fname) {
    FILE *fp;
    WINDOW *new_win;
    PANEL *newpanel;
    int i = 1;
    int j = 0;
    int key = 0;
    char *line = NULL;
    size_t len = 0;

    /* Write the file to the window */
    fp = fopen(fname, "r");
    if (fp == NULL)
        return;
    new_win = newpad(MAX_FILE_LEN, max(term.w, MAPW));
    newpanel = new_panel(new_win);
    while (getline(&line, &len, fp) != -1) {
        mvwprintw(new_win, i++, 1, "%s", line);
    }
    free(line);
    fclose(fp);
    /* Handle player input */
    f.mode_map = 0;
    while (1) {
        prefresh(new_win, j, 0, 0, 0, term.h - 1, term.w - 1);

        key = handle_keys();
        switch (key) {
            case 27:
                werase(new_win);
                prefresh(new_win, j, 0, 0, 0, term.h - 1, term.w - 1);
                del_panel(newpanel);
                delwin(new_win);
                f.update_map = 1;
                f.update_msg = 1;
                f.mode_map = 1;
                update_panels();
                doupdate();
                return;
            case KEY_UP:
            case 'k':
                j -= 1;
                break;
            case KEY_DOWN:
            case 'j':
                j += 1;
                break;
        }
        j = min(max(0, j), max(0, i - term.h));
    }
}

/* Windowport code. Displays both sidebars. */
void display_sb(void) {
    curses_display_sb(sb_win_left.win);
    curses_display_sb(sb_win_right.win);
}

/**
 * @brief Display a sidebar window
 * 
 * @param sb_win The window in question.
 */
void curses_display_sb(WINDOW *sb_win) {
    int j = 1;

    werase(sb_win);
    box(sb_win, 0, 0);
    wattron(sb_win, A_STANDOUT);
    if (sb_win == sb_win_right.win)
        mvwprintw(sb_win, 0, 1, "Team %s", g.userbuf); 
    else
        mvwprintw(sb_win, 0, 1, "Opponent");
    wattroff(sb_win, A_STANDOUT);

    if (term.hudmode == HUD_MODE_CHAR && sb_win == sb_win_right.win) {
        display_sb_stats(sb_win, &j, g.player);
        j++;
        if (!g.depth)
            mvwprintw(sb_win, j++, 1, "FL: Lobby T: %d", g.turns);
        else if (g.depth != g.max_depth)
            mvwprintw(sb_win, j++, 1, "FL: %d (max %d) T:%d", g.depth, g.max_depth, g.turns);
        else
            mvwprintw(sb_win, j++, 1, "FL: %d T: %d", g.depth, g.turns);
    } 
    if (term.hudmode == HUD_MODE_CHAR && sb_win == sb_win_left.win && g.target != NULL) {
        display_sb_stats(sb_win, &j, g.target);
    } 
    if (term.hudmode == HUD_MODE_HELP && sb_win == sb_win_left.win) {
        display_sb_nearby(sb_win, &j);
    } 
    if (term.hudmode == HUD_MODE_HELP && sb_win == sb_win_right.win) {
        display_sb_controls(sb_win, &j);
    }
    wrefresh(sb_win);
}

/**
 * @brief Print controls to a sidebar window.
 * 
 * @param sb_win The window to print to.
 * @param j A pointer to the y value to print at.
 */
void display_sb_controls(WINDOW *sb_win, int *j) {
    char *action;
    int actmax = (g.debug) ? A_WISH : A_MAGICMAP;
    for (int i = A_REST; i < actmax; i++) {
        action = stringify_action(i);
        mvwprintw(sb_win, (*j)++, 1, "%s", action);
        free(action);
    }
}

/**
 * @brief Print the nearby actor list to the sidebar.
 * 
 * @param j A pointer to the y value to print at.
 */
void display_sb_nearby(WINDOW *sb_win, int *j) {
    char buf[128];
    struct actor *cur_npc = g.player;

    wattron(sb_win, A_UNDERLINE);
    memset(buf, 0, 128);
    snprintf(buf, sizeof(buf), "Nearby");
    mvwprintw(sb_win, (*j)++, 1, "%s", buf);
    wattroff(sb_win, A_UNDERLINE);
    while (cur_npc != NULL) {
        if (is_visible(cur_npc->x, cur_npc->y) && cur_npc != g.player) {
            memset(buf, 0, 128);
            snprintf(buf, sizeof(buf), "%c", cur_npc->chr);
            wcolor_on(sb_win, cur_npc->color);
            mvwprintw(sb_win, *j, 1, "%s", buf);
            wcolor_off(sb_win, cur_npc->color);
            
            if (is_aware(cur_npc, g.player)) {
                wcolor_on(sb_win, BRIGHT_YELLOW);
                mvwprintw(sb_win, *j, 2, "!");
                wcolor_off(sb_win, BRIGHT_YELLOW);
            }

            memset(buf, 0, 128);
            snprintf(buf, sizeof(buf), "%s (%d, %d)", actor_name(cur_npc, 0), cur_npc->x, cur_npc->y);
            mvwprintw(sb_win, (*j)++, 4, "%s", buf);
        }
        cur_npc = cur_npc->next;
    }
}

/**
 * @brief Display the stats in a given sidebar
 * 
 * @param win The sidebar window
 * @param i A pointer to the y value to print at.
 * @param actor The actor to display stats for.
 */
/* TODO: Remove unecessary buffer usage. */
void display_sb_stats(WINDOW *win, int *i, struct actor *actor) {
    char buf[128];
    struct attack *cur_attack = get_active_attack(actor, g.active_attack_index);
    int k = 0;

    memset(buf, 0, 128);
    wattron(win, A_UNDERLINE);
    if (actor == g.player)
        snprintf(buf, sizeof(buf), "%s", actor_name(actor, NAME_CAP));
    else
        snprintf(buf, sizeof(buf), "%s (target)", actor_name(actor, NAME_CAP));
    mvwprintw(win, *i, 1, "%s", buf);
    *i+= 1;
    wattroff(win, A_UNDERLINE);
    mvwprintw(win, *i, 1, "EV: %d%% AC: %d%% EN: %d\t", actor->evasion, actor->accuracy, actor->energy);
    *i += 1;

    /* TODO: Clean up this epic kludge */
    for (int index = 0; index < MAX_ATTK * 2; index++) {
        cur_attack = get_active_attack(actor, index);
        if ((EWEP(actor) && (index > MAX_ATTK || is_noatk(EWEP(actor)->attacks[index]))) ||
            (EOFF(actor) && !EWEP(actor) && (index >= MAX_ATTK || is_noatk(EOFF(actor)->attacks[index]))) ||
            (!(EWEP(actor) || EOFF(actor)) && (index >= MAX_ATTK || is_noatk(actor->attacks[index])))) {
            break;
        } else if (EOFF(actor) && EWEP(actor) && (index < MAX_ATTK ? is_noatk(EWEP(actor)->attacks[index]) : is_noatk(EOFF(actor)->attacks[index]))) {
            k++;
            continue;
        }
        if (cur_attack && cur_attack->dam) {
            memset(buf, 0, 128);
            snprintf(buf, sizeof(buf), "%s%d %s [%d%%]=>%d", 
                    (actor == g.player && (g.active_attack_index % MAX_ATTK == k)) ? "*" : " ", k + 1,
                    g.active_attacker == g.player ? "Unarmed" : actor_name(g.active_attacker, NAME_CAP),  /* Bugged line */
                    cur_attack->accuracy, cur_attack->dam);
            mvwprintw(win, *i, 1, "%s", buf);
            print_hitdescs(win, (*i)++, 1 + strlen(buf), cur_attack->hitdescs, 0);
            k++;
        }
    }

    if (actor != g.player) {
        *i += 1;
        if (is_aware(actor, g.player)) {
            wcolor_on(win, BRIGHT_YELLOW);
            mvwprintw(win, *i, 1, "Tracking");
            wcolor_off(win, BRIGHT_YELLOW);
        } else {
            mvwprintw(win, *i, 1, "Unaware");
        }
        *i += 1;
        if (g.debug) {
            print_stance(actor, win, *i, 1);
        }
        
    }
}

void print_hitdescs(WINDOW *win, int y, int x, short hitdescs, unsigned char blanks) {
    char buf[16]; /* For a more verbose form later. */

    for (int i = 0; i < MAX_HITDESC; i++) {
        if (hitdescs & hitdescs_arr[i].val) {
            memset(buf, 0, 16);
            wcolor_on(win, hitdescs_arr[i].color);
            snprintf(buf, sizeof(buf), "%c", hitdescs_arr[i].str[0]);
            mvwprintw(win, y, x++, "%s", buf);
            wcolor_off(win, hitdescs_arr[i].color);
        } else if (blanks) {
            mvwprintw(win, y, x++, "_");
        }
    }
}

/**
 * @brief Render a bar.
 * 
 * @param win window to render on.
 * @param cur current value.
 * @param max max value.
 * @param x x coordinate.
 * @param y y coordinate.
 * @param width width.
 * @param reverse reverse the direction that  the bar extends
 */
void render_bar(WINDOW *win, int cur, int max, int x, int y,
                int width, int reverse) {
    int pips = (int) (width * cur / max);
    char buf[16] = {'\0'};
    snprintf(buf, sizeof(buf), "%d", cur);
    wattron(win, A_REVERSE);
    for (int i = 0; i < width; i++) {
        if (i <= pips) {
            mvwaddch(win, y, reverse ? (x + width - i) : x + i, ' ');
        }
    }
    if (reverse)
        mvwprintw(win, y, x + width - strlen(buf), "%s", buf);
    else
        mvwprintw(win, y, x + 1, "%s", buf);
    wattroff(win, A_REVERSE);
}

/**
 * @brief Action that fullscrens the message window.
 * 
 * @return int Cost of fullscreening.
 */
int fullscreen_action(void) {
    draw_msg_window(1);
    return 0;
}

/**
 * @brief Print the current stance of an actor to the window.
 * 
 * @param actor
 * @param win 
 * @param y 
 * @param x 
 */
void print_stance(struct actor *actor, WINDOW *win, int y, int x) {
    switch(actor->stance) {
        case STANCE_CROUCH:
            mvwprintw(win, y, x, "Crouch");
            break;
        case STANCE_STAND:
            mvwprintw(win, y, x, "Stand");
            break;
        case STANCE_TECH:
            mvwprintw(win, y, x, "Tech");
            break;
        case STANCE_STUN:
            wcolor_on(win, BRIGHT_RED);
            mvwprintw(win, y, x, "STUN");
            wcolor_off(win, BRIGHT_RED);
            break;
        default:
            mvwprintw(win, y, x, "UNKNOWN");
            break;
    }
}

void draw_lifebars(void) {
    char buf[4] = {'\0'};
    WINDOW *bars_win_p = bars_win.win;

    werase(bars_win_p);
    box(bars_win_p, 0, 0);
    /* Health Bar */
    wcolor_on(bars_win_p, BRIGHT_GREEN);
    render_bar(bars_win_p, g.player->hp, g.player->hpmax, term.msg_w / 2 + 4, 1,
                    term.msg_w / 2 - 5, 0);
    if (g.target)
        render_bar(bars_win_p, g.target->hp, g.target->hpmax, 1, 1,
                    term.msg_w / 2 - 6, 1);       
    wcolor_off(bars_win_p, BRIGHT_GREEN);
    /* Center Text */
    wcolor_on(bars_win_p, BRIGHT_YELLOW);
    print_stance(g.player, bars_win_p, 2, term.msg_w / 2 - 3);
    wcolor_off(bars_win_p, BRIGHT_YELLOW);
    snprintf(buf, sizeof(buf), "%d", g.player->energy);
    mvwprintw(bars_win_p, 1, term.msg_w / 2 - 1, "%s", buf);
    /* Energy Bar */
    wcolor_on(bars_win_p, BRIGHT_BLUE);
    render_bar(bars_win_p, g.player->hp, g.player->hpmax, term.msg_w / 2 + 4, 2,
                    term.msg_w / 2 - 5, 0);
    if (g.target)
        render_bar(bars_win_p, g.target->hp, g.target->hpmax, 1, 2,
                    term.msg_w / 2 - 6, 1);    
    wcolor_off(bars_win_p, BRIGHT_BLUE);
}

/**
 * @brief Draw the message window.
 * 
 * @param full wehther it is being drawn fullscreen.
 */
void draw_msg_window(int full) {
    int i = 0;
    struct msg *cur_msg;

    werase(msg_win);
    cur_msg = g.msg_list;
    while (cur_msg != NULL) {
        wcolor_on(msg_win, cur_msg->attr);
        waddstr(msg_win, cur_msg->msg);
        wcolor_off(msg_win, cur_msg->attr);
        waddch(msg_win, '\n');
        cur_msg = cur_msg->next;
        i++;
    }
    box(msgbox_win.win, 0, 0);
    if (full) {
        prefresh(msg_win, 0, 0, 0, 0, term.h, term.w);
    } else {
        prefresh(msg_win, 0, 0, term.msg_y + 1, 1, term.msg_y + term.msg_h - 2, term.msg_w - 2);
    }
    f.update_msg = 0;

    if (full) {
        getch();
        prefresh(msg_win, 0, 0, 1, 1, term.h - 2, term.w - 2);
        f.update_map = 1;
        f.update_msg = 1;
    }
}

/**
 * @brief Render a map tile.
 * 
 * @param x x coordinate to render at.
 * @param y y coordinate to render at.
 * @param mx x coordinate of the tile on the map.
 * @param my y coordinate of the tile on the map.
 * @param attr attributes to render with.
 * @return int result of map_putch.
 */
int map_put_tile(int x, int y, int mx, int my, int attr) {
    return map_putch(x, y, g.levmap[mx][my].pt->chr, attr);
}

/**
 * @brief Render an actor.
 * 
 * @param x x coordinate to render at.
 * @param y y coordinate to render at.
 * @param actor actor to be rendered.
 * @param attr attributes to render with.
 * @return int result of map_putch.
 */
int map_put_actor(int x, int y, struct actor *actor, int attr) {
    int ret;
    if (actor == g.target)
        wattron(map_win.win, A_UNDERLINE);
    if (actor == g.player)
        wattron(map_win.win, A_REVERSE);
    ret = map_putch(x, y, actor->chr, attr);
    if (actor == g.target)
        wattroff(map_win.win, A_UNDERLINE);
    if (actor == g.player)
        wattroff(map_win.win, A_REVERSE);
    return ret;
}

/**
 * @brief Output a character to the map window. Wrapper for mvwaddch().
 * 
 * @param x x coordinate to render at.
 * @param y y coordinate to render at.
 * @param chr character to render.
 * @param attr attributes to render with.
 * @return int Result of mvwaddch().
 */
int map_putch(int x, int y, int chr, int attr) {
    int ret;
    wcolor_on(map_win.win, attr);
    ret = mvwaddch(map_win.win, y, x, chr); 
    wcolor_off(map_win.win, attr);
    return ret;
}

/**
 * @brief Outputs a character to the map window. Since curses only supports
 a small set of colors, we cast to an integer and modulo.
 * 
 * @param x x coordinate to render at.
 * @param y y coordinate to render at.
 * @param chr character to render.
 * @param attr attributes to render with.
 * @return int Result of map_putch().
 */
int map_putch_truecolor(int x, int y, int chr, unsigned color) {
    int final_color = color % MAX_COLOR;
    return map_putch(x, y, chr, final_color);
}

/**
 * @brief Erase the map window.
 * 
 */
void clear_map(void) {
    werase(map_win.win);
}

/**
 * @brief Refresh the map window. Moves the window cursor
 to the player's location.
 * 
 */
void refresh_map(void) {
    wmove(map_win.win, g.player->y - g.cy, g.player->x - g.cx);
    wrefresh(map_win.win);
}

/**
 * @brief Handle mouse inputs.
 * 
 * @return Return an action.
 */
int handle_mouse(void) {
    int x, y;   /* Mouse cell */
    int gx, gy; /* Map cell */
    MEVENT event;

    if (getmouse(&event) != OK)
        return 0;
    
    x = event.x;
    y = event.y;
    gx = x + g.cx - term.mapwin_x;
    gy = y - g.cy - term.mapwin_y;
    
    #if 0
    if (event.bstate & BUTTON1_PRESSED) {
        if (y <= term.msg_h - 1 && x <= term.msg_w) {
            return A_FULLSCREEN;
        }
    }
    #endif

    if (f.mode_look) {
        g.cursor_x = gx;
        g.cursor_y = gy;
    }

    /* Left click to travel. */
    if ((event.bstate & BUTTON1_PRESSED)
        && !f.mode_look
        && in_bounds(gx, gy)
        && !is_blocked(gx, gy)
        && is_explored(gx, gy)) {
        g.goal_x = gx;
        g.goal_y = gy;
        f.mode_run = 1;
        do_heatmaps(heatmaps[HM_GOAL].field, 0);
        return 0;
    }
    /* Right click to examine. */
    if (event.bstate & BUTTON3_PRESSED) {
        look_at(gx, gy);
    }

    return 0;
}

/**
 * @brief Handle key inputs. Blocking.
 * 
 * @return int Return the action taken.
 */
int handle_keys(void) {
    int keycode = getch();
    /* This is a bit more complicated than other input systems,
       since curses picks up character codes, rather than
       keyboard strikes. */
    switch(keycode) {
        case KEY_MOUSE:
            return handle_mouse();
        case KEY_UP:
            return 'k';
        case KEY_DOWN:
            return 'j';
        case KEY_LEFT:
            return 'h';
        case KEY_RIGHT:
            return 'l';
        case KEY_HOME:
            return 'y';
        case KEY_END:
            return 'b';
        case KEY_NPAGE:
            return 'n';
        case KEY_PPAGE:
            return 'u';
        case KEY_BACKSPACE:
            return '\b';
        default:
            return keycode;
    }
    return 0;
}
