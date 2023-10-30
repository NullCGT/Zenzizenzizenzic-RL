/**
 * @file message.c
 * @author Kestrel (kestrelg@kestrelscry.com)
 * @brief Functionality necessary for ingame messages.
 * @version 1.0
 * @date 2022-05-27
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#include "register.h"
#include "windows.h"
#include "message.h"
#include "render.h"

int log_string(const char *, int, va_list);
void wrap_string(char *, int);

/**
 * @brief Free an individual message.
 * 
 * @param message The message to be freed.
 */
void free_msg(struct msg *message) {
    free(message->msg);
    free(message);
    return;
}

/**
 * @brief Free all messages in the linked list of messages. Uses console
 * output.
 * 
 * @param cur_msg Head of the linked list to be freed.
 * @return int Messages freed.
 */
int free_message_list(struct msg *cur_msg) {
    struct msg *prev_msg = cur_msg;
    int count = 0;
    while (cur_msg != (struct msg *) 0) {
        cur_msg = cur_msg->next;
        free_msg(prev_msg);
        prev_msg = cur_msg;
        count++;
    }
    return count;
}

/**
 * @brief Iterates through a string and inserts newlines in order
 to wrap the text in an aesthetically pleasing manner.
 * 
 * @param buf The string to be wrapped. Mutated by this function.
 * @param max_width The width at which to wrap.
 */
void wrap_string(char *buf, int max_width) {
    int last_space = 0;
    int w = 0;
    int i = 0;

    while (1) {
        if (buf[i] == '\0') return;
        if (buf[i] == ' ') last_space = i;
        if (w >= max_width - 1) {
            buf[last_space] = '\n';
            w = -1;
        }
        w++;
        i++;
    }
}

/**
 * @brief Replace the line breaks in a string with spaces.
 * 
 * @param buf The string to be unwrapped. Mutated by this function call.
 * @return char* The unwrapped string.
 */
char *unwrap_string(char *buf) {
    int i =0;
    while (buf[i] != '\0') {
        if (buf[i] == '\n')
            buf[i] = ' ';
        i++;
    }
    return buf;
}

/**
 * @brief Output a string to the log.
 * 
 * @param format Format.
 * @param attr Attributes to apply. Should be constrained to color.
 * @param arg Arguments for the format.
 * @return int Returns zero.
 */
int log_string(const char *format, int attr, va_list arg) {
    char *msgbuf = malloc(MAX_MSG_LEN * sizeof(char));
    int i = 0;
    struct msg *cur_msg;
    struct msg *prev_msg;

    vsnprintf(msgbuf, MAX_MSG_LEN * sizeof(char), format, arg);
    msgbuf[MAX_MSG_LEN - 1] = '\0';

    struct msg *new_msg = malloc(sizeof(struct msg));
    wrap_string(msgbuf, term.msg_w);
    new_msg->msg = msgbuf;
    new_msg->turn = g.turns;
    new_msg->attr = attr;
    new_msg->next = NULL;
    new_msg->prev = NULL;
    /* Insert message at the beginning of the message list. */
    if (g.msg_list == (struct msg *) 0) {
        g.msg_list = new_msg;
        g.msg_last = new_msg;
    } else {
        new_msg->next = g.msg_list;
        new_msg->next->prev = new_msg;
        g.msg_list = new_msg;
    }
    /* Loop through list and delete messages that exceed MAX_BACKSCROLL. */
    /* TODO: Theoretically, we could keep a bitfield flag that denotes whether the
       message log has been filled. If it has, we skip iterating through the linked
       list and delete the last message. If not, we iterate. Messy, but would save
       a lot of overhead on every call to log_string(). */
    cur_msg = g.msg_list;
    while (cur_msg != NULL) {
        if (i >= MAX_BACKSCROLL) {
            prev_msg->next = NULL;
            prev_msg = cur_msg;
            cur_msg = cur_msg->next;
            free_msg(prev_msg);
            i++;
            continue;
        }
        i++;
        prev_msg = cur_msg;
        g.msg_last = cur_msg;
        cur_msg = cur_msg->next;
    }
    /* Handle rendering */
    f.update_msg = 1;
    return 0;
}

/**
 * @brief Output a message to the log with standard formatting.
 * 
 * @param format The format.
 * @param ... Formatting arguments.
 * @return int The result of log_string.
 */
int logm(const char *format, ...) {
    int ret;
    va_list arg;

    va_start(arg, format);
    ret = log_string(format, WHITE, arg);
    va_end(arg);
    return ret;
}

/**
 * @brief Output a message to the log with a color attribute.
 * 
 * @param format The format.
 * @param ... Formatting arguments.
 * @return int The result of log_string.
 */
int logma(int attr, const char *format, ...) {
    int ret;
    va_list arg;

    va_start(arg, format);
    ret = log_string(format, attr, arg);
    va_end(arg);
    return ret;
}

/* TODO: Potentially exploitable? */
/**
 * @brief Output a warning message to the log.
 * 
 * @param format The format.
 * @param ... Formatting arguments.
 * @return int The result of log_string.
 */
int logm_warning(const char *format, ...) {
    int ret;
    va_list arg;
    char buf[MAX_MSG_LEN] = "Warning: ";
    strcat(buf, format);

    va_start(arg, format);
    ret = log_string(buf, MAGENTA, arg);
    va_end(arg);
    return ret;
}

/**
 * @brief Return "a" or "an", depending on which would be grammatically
 correct when preceeding the string passed in.
 * 
 * @param str The string to consider.
 * @return const char* "a" or "an"
 */
const char *an(const char *str) {
    if (str && vowel(str[0]))
        return "an";
    return "a";
}

/**
 * @brief Prompt the user to answer a yes/no message in the message log.
 * 
 * @param prompt The prompt that the user sees.
 * @param def_choice The default option for the prompt.
 * @return int Return 1 if yes, 0 if no.
 */
int yn_prompt(const char *prompt, int def_choice) {
    int keycode;
    logma(BLUE, "%s (%s)", prompt, def_choice ? "Yn" : "yN");
    render_all();
    while ((keycode = handle_keys())) {
        if (keycode == 'y' || keycode == 'Y')
            return 1;
        else if (keycode == 'n' || keycode == 'N')
            return 0;
        else if (keycode == 27 || keycode == '\n')
            return def_choice;
    }
    return def_choice;
}