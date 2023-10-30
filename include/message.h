#ifndef MESSAGE_H
#define MESSAGE_H

struct msg {
    char *msg;
    int turn;
    int attr;
    struct msg *next;
    struct msg *prev;
};

#define MAX_MSG_LEN 256
#define MAX_BACKSCROLL 25

/* Function Prototypes */
void free_msg(struct msg *);
int free_message_list(struct msg *);
char *unwrap_string(char *);
int logm(const char *, ...);
int logma(int, const char *, ...);
int logm_warning(const char *, ...);
const char *an(const char *);
int yn_prompt(const char *, int);

#endif