//kernel/mailbox.h
#ifndef MAIL_BOX
#define MAIL_BOX

#include "types.h"
#include "spinlock.h"

#define MAX_MAIL_BOXES 64
#define MAX_MAILS_PER_BOX 16

struct mailbox {
    struct spinlock lock;
    int used;
    int key;
    int buf[MAX_MAILS_PER_BOX];
    int head, tail, count;
    int closed;
};

void mboxinit(void);
int mboxcreate(int key);
int mboxsend(int key, int msg);
int mboxrecv(int key, int *msg);

#endif