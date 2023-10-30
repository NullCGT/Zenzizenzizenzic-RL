/**
 * @file menu.c
 * @author Kestrel (kestrelg@kestrelscry.com)
 * @brief Menu-related code for the ncurses windowport.
 * @version 1.0
 * @date 2022-05-26
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#include <curses.h>
#include <stdlib.h>
#include <string.h>

#include "register.h"
#include "windows.h"
#include "menu.h"

/**
 * @brief Initialize a new menu and give it focus.
 * 
 * @param title The title of the new menu.
 * @return struct menu* The menu that has been created.
 */
struct menu *menu_new(const char *title, int x, int y, int w, int h) {
    struct menu *new_menu = malloc(sizeof(struct menu));
    new_menu->title = title;
    new_menu->max = 0;
    new_menu->selected = 0;
    new_menu->items = NULL;
    new_menu->win = create_win(h, w, y, x);
    f.mode_map = 0;
    update_panels();
    doupdate();
    return new_menu;
}

/**
 * @brief Add an item to an existing menu.
 * 
 * @param menu The menu the item is to be added to.
 * @param index The letter associated with the item.
 * @param text The text associated with the menu item. 
 */
void menu_add_item(struct menu *menu, unsigned char index, const char *text) {
    struct menu_item *cur = menu->items;
    struct menu_item *prev = menu->items;

    struct menu_item *new_item = malloc(sizeof(struct menu_item));
    new_item->index = index;
    new_item->next = NULL;
    strcpy(new_item->text, text);

    while (cur != NULL) {
        prev = cur;
        cur = cur->next;
    }
    if (prev) prev->next = new_item;
    else menu->items = new_item;

    menu->max += 1;
}

/**
 * @brief Render a menu and all associated items.
 * 
 * @param menu The menu to be rendered.
 */
void display_menu(struct menu *menu) {
    struct menu_item *cur = menu->items;
    WINDOW *menu_win_p = menu->win.win;
    char itembuf[128];
    int index = 0;

    werase(menu_win_p);
    wcolor_on(menu_win_p, YELLOW);
    box(menu_win_p, 0, 0);
    if (menu->title) {
        wattron(menu_win_p, A_STANDOUT);
        mvwprintw(menu_win_p, 0, 1, menu->title);
        wattroff(menu_win_p, A_STANDOUT);
    }
    wcolor_off(menu_win_p, YELLOW);
    
    while (cur != NULL) {
        memset(itembuf, 0, 128);
        snprintf(itembuf, sizeof(itembuf), " %c) %s", cur->index, cur->text);
        if (index == menu->selected) {
            wattron(menu_win_p, A_BOLD);
            wattron(menu_win_p, A_UNDERLINE);
        }
        mvwprintw(menu_win_p, index + 2, 1, itembuf);
        wattroff(menu_win_p, A_BOLD);
        wattroff(menu_win_p, A_UNDERLINE);
        cur = cur->next;
        index++;
    }
    update_panels();
    doupdate();
}

/**
 * @brief Menu driver. Gets blocking input from the user in order for
them to navigate the menu and select different options.
 * 
 * @param menu The menu that is taking input.
 * @param can_quit Defines whether the menu can be exited manually.
 * @return signed char The index that was selected. Returns -1 if the menu is
exited from and nothing is chosen. The caller must handle a response of -1.
 */
signed char menu_do_choice(struct menu *menu, int can_quit) {
    struct menu_item *cur_item = menu->items;
    int x, y;
    MEVENT event;

    while (1) {
        display_menu(menu);
        doupdate();
        int input = getch();

        if (input == 27 && can_quit) {
            return -1;
        } else if (input == KEY_UP || input == '8') {
            menu->selected = max(0, menu->selected - 1);
        } else if (input == KEY_DOWN || input == '2') {
            menu->selected = min(menu->max - 1, menu->selected + 1);
        } else if (input == KEY_ENTER || input == '\n' || input == '\r') {
            for (int i = 0; i < menu->selected; i++) {
                cur_item = cur_item->next;
            }
            return cur_item->index;
        } else if (input >= 'a' && input <= 'z') {
            return input;
        } else if (input == KEY_MOUSE) {
            if (getmouse(&event) != OK)
                continue;
            getbegyx(menu->win.win, y, x);
            x = event.x - x;
            y = event.y - y - 2;
            if (y < 0 || y >= menu->max)
                continue;
            else
                menu->selected = y;
            if (event.bstate & BUTTON1_PRESSED) {
                (void) x;
                for (int i = 0; i < menu->selected; i++) {
                    cur_item = cur_item->next;
                }
                return cur_item->index;
            }
        }
    }
}

/**
 * @brief Destroy a menu and free all memory associated with it.
 * 
 * @param menu The menu to be destroyed.
 */
void menu_destroy(struct menu *menu) {
    struct menu_item *cur = menu->items;
    struct menu_item *prev = menu->items;

    while (cur != NULL) {
        cur = cur->next;
        free(prev);
        prev = cur;
    }
    cleanup_win(menu->win);
    free(menu);
    f.update_msg = 1;
    f.mode_map = 1;
}