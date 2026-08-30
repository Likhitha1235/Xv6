// #include "types.h"
// #include "param.h"
// #include "spinlock.h"
// #include "defs.h"
// #include "mbox.h"

// static struct {
//     struct mailbox box[MAX_MAIL_BOXES];
// } mboxes;

// // initialize all mailboxes at boot
// void
// mboxinit(void){
//     for(int i = 0; i < MAX_MAIL_BOXES; i++){
//         initlock(&mboxes.box[i].lock, "mbox");
//         mboxes.box[i].used = 0;
//         mboxes.box[i].key = 0;
//         mboxes.box[i].head = 0;
//         mboxes.box[i].tail = 0;
//         mboxes.box[i].count = 0;
//         mboxes.box[i].closed = 0;
//     }
// }


// int
// mboxcreate(int key){
//     if(key <= 0) return -2; // invalid key

    
//     for(int i = 0; i < MAX_MAIL_BOXES; i++){
//         acquire(&mboxes.box[i].lock);
//         if(mboxes.box[i].used && mboxes.box[i].key == key){
//             release(&mboxes.box[i].lock);
//             return i; 
//         }
//         release(&mboxes.box[i].lock);
//     }

//     for(int i = 0; i < MAX_MAIL_BOXES; i++){
//         acquire(&mboxes.box[i].lock);
//         if(!mboxes.box[i].used){
//             mboxes.box[i].used = 1;
//             mboxes.box[i].key = key;
//             mboxes.box[i].head = 0;
//             mboxes.box[i].tail = 0;
//             mboxes.box[i].count = 0;
//             mboxes.box[i].closed = 0;
//             release(&mboxes.box[i].lock);
//             return i;
//         }
//         release(&mboxes.box[i].lock);
//     }

//     return -1; 
// }


// int
// mboxsend(int key, int msg){
//     struct mailbox *m = 0;

//     // find mailbox by key
//     int ii = -1;
//     for(int i = 0; i < MAX_MAIL_BOXES; i++){
//         acquire(&mboxes.box[i].lock);
//         if(mboxes.box[i].used && mboxes.box[i].key == key){
//             m = &mboxes.box[i];
//             ii = i;
//             // release(&mboxes.box[i].lock);
//             break; 
//         } else {
//             release(&mboxes.box[i].lock);
//         }
//     }
//     if(m == 0) return -1;

//     acquire(&m->lock);
//     release(&mboxes.box[ii].lock);

//     while(m->count == MAX_MAILS_PER_BOX){
//         sleep(m, &m->lock); 
//         acquire(&m->lock);
//     }

//     // insert message
//     m->buf[m->tail] = msg;
//     m->tail = (m->tail + 1) % MAX_MAILS_PER_BOX;
//     m->count++;

//     wakeup(m);      
//     release(&m->lock);
//     return 0;
// }


// int
// mboxrecv(int key, int *msg){
//     if(key <= 0 || msg == 0) return -1;

//     struct mailbox *m = 0;

//     int ii = -1;
//     for(int i = 0; i < MAX_MAIL_BOXES; i++){
//         acquire(&mboxes.box[i].lock);
//         if(mboxes.box[i].used && mboxes.box[i].key == key){
//             m = &mboxes.box[i];
//             ii = i;
//             break; 
//         } else {
//             release(&mboxes.box[i].lock);
//         }
//     }
//     if(m == 0) return -1;

//     acquire(&m->lock);
//     release(mboxes.box[ii].lock);

//     while(m->count == 0){
//         sleep(m, &m->lock); 
//         acquire(&m->lock);
//     }

    
//     *msg = m->buf[m->head];
//     m->head = (m->head + 1) % MAX_MAILS_PER_BOX;
//     m->count--;

//     wakeup(m);      
//     release(&m->lock);
//     return 0;
// }

#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "riscv.h"
#include "proc.h"
#include "mailbox.h"
#include "defs.h"

static struct {
    struct mailbox box[MAX_MAIL_BOXES];
} mboxes;

// initialize all mailboxes at boot
void
mboxinit(void)
{
    for(int i = 0; i < MAX_MAIL_BOXES; i++){
        initlock(&mboxes.box[i].lock, "mbox");
        mboxes.box[i].used = 0;
        mboxes.box[i].key = 0;
        mboxes.box[i].head = 0;
        mboxes.box[i].tail = 0;
        mboxes.box[i].count = 0;
        mboxes.box[i].closed = 0;
    }
}

int
mboxcreate(int key)
{
    if(key <= 0) return -2; // invalid key

    // First check if mailbox with this key already exists
    for(int i = 0; i < MAX_MAIL_BOXES; i++){
        acquire(&mboxes.box[i].lock);
        if(mboxes.box[i].used && mboxes.box[i].key == key){
            release(&mboxes.box[i].lock);
            return -1; // Mailbox already exists
        }
        release(&mboxes.box[i].lock);
    }

    // Find an unused mailbox slot
    for(int i = 0; i < MAX_MAIL_BOXES; i++){
        acquire(&mboxes.box[i].lock);
        if(!mboxes.box[i].used){
            mboxes.box[i].used = 1;
            mboxes.box[i].key = key;
            mboxes.box[i].head = 0;
            mboxes.box[i].tail = 0;
            mboxes.box[i].count = 0;
            mboxes.box[i].closed = 0;
            release(&mboxes.box[i].lock);
            return i; // Return mailbox index
        }
        release(&mboxes.box[i].lock);
    }

    return -1; // No free mailbox slots
}

int
mboxsend(int key, int msg)
{
    struct mailbox *m = 0;
    int found = 0;

    // find mailbox by key
    for(int i = 0; i < MAX_MAIL_BOXES; i++){
        acquire(&mboxes.box[i].lock);
        if(mboxes.box[i].used && mboxes.box[i].key == key){
            m = &mboxes.box[i];
            found = 1;
            break;
        }
        release(&mboxes.box[i].lock);
    }
    
    if(!found) return -1;

    // Wait until there's space in the mailbox
    while(m->count == MAX_MAILS_PER_BOX){
        sleep(m, &m->lock);
    }

    // insert message
    m->buf[m->tail] = msg;
    m->tail = (m->tail + 1) % MAX_MAILS_PER_BOX;
    m->count++;

    wakeup(m); // Wake up any waiting receivers
    release(&m->lock);
    return 0;
}

int
mboxrecv(int key, int *msg)
{
    if(key <= 0 || msg == 0) return -1;

    struct mailbox *m = 0;
    int found = 0;

    // find mailbox by key
    for(int i = 0; i < MAX_MAIL_BOXES; i++){
        acquire(&mboxes.box[i].lock);
        if(mboxes.box[i].used && mboxes.box[i].key == key){
            m = &mboxes.box[i];
            found = 1;
            break;
        }
        release(&mboxes.box[i].lock);
    }
    
    if(!found) return -1;

    // Wait until there's a message to receive
    while(m->count == 0){
        sleep(m, &m->lock);
    }

    // retrieve message
    *msg = m->buf[m->head];
    m->head = (m->head + 1) % MAX_MAILS_PER_BOX;
    m->count--;

    wakeup(m); // Wake up any waiting senders
    release(&m->lock);
    return 0;
}
