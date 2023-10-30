/**
 * @file hiscore.c
 * @author Kestrel (kestrelg@kestrelscry.com)
 * @brief 
 * @version 1.0
 * @date 2023-10-30
 * 
 * @copyright Copyright (c) 2023
 * 
 */

#include <curses.h>
#include <stdlib.h>

#include "register.h"
#include "windows.h"
#include "parser.h"

void display_hiscore_list(void) {
    cJSON *hiscore_json;
    cJSON *scores_json;
    cJSON *entry;
    char entry_buf[256];
    
    WINDOW *new_win;
    PANEL *newpanel;
    int i = 1;
    int j = 0;
    int key = 0;

    /* Write the file to the window */
    fp = fopen("hiscores.json", "r");
    if (fp == NULL)
        return;

    cJSON *hiscore_from_json = json_from_file("hiscores.json");
    new_win = newpad(MAX_FILE_LEN, max(term.w, MAPW));
    newpanel = new_panel(new_win);

    /* Parse the json into something readable */
    scores_json = cJSON_GetObjectItemCaseSensitive(test_json, "scores");
    cJSON_ArrayForEach(entry, scores_json) {
        memset(buf, 0, 256);
        snprintf(buf, 256, "Team %s [%d] (%s) scored %d.\n%s")
        mvwprintw(new_win, i++, 1, line);
    }


    /* Handle player input */
    /* TODO: Pull this out into a function since it's the same as file display */
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