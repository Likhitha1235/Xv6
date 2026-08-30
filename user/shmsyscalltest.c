// #include "kernel/types.h"
// #include "kernel/stat.h"
// #include "user/user.h"

// // prototypes for your syscalls
// int shm_create(int key);
// uint64 shm_get(int key);
// int shm_close(int key);

// #define SHM_KEY 1234

// int
// main(int argc, char *argv[])
// {
//     printf("shmtest: starting\n");

//     // 1. Parent creates a shared region
//     int ret = shm_create(SHM_KEY);

//     if(ret < 0){
//         printf("shmtest: shm_create failed\n");
//         exit(1);
//     }
//     printf("shmtest: created shm with key=%d\n", SHM_KEY);

//     // 2. Parent maps the region
    
//     char *shm_parent = (char*) shm_get(SHM_KEY);
    
//     if(shm_parent == 0){
        
//         printf("shmtest: parent shm_get failed\n");
//         exit(1);
    
//     }
    
//     //printf("shmtest: parent mapped shm at %p\n", shm_parent);

//     // 3. Parent writes to shared memory
//     strcpy(shm_parent, "hello from parent");//

//     // 4. Fork a child
//     int pid = fork();
//     if(pid < 0){
//         printf("shmtest: fork failed\n");
//         exit(1);
//     }

//     if(pid == 0){
//         // ---- CHILD ----
//         //sleep(10); // let parent write first

//         char *shm_child = (char*) shm_get(SHM_KEY);
//         if(shm_child == 0){
//             printf("child: shm_get failed\n");
//             exit(1);
//         }
//         printf("child: mapped shm at %p\n", shm_child);

//         // read what parent wrote
//         printf("child: read from shm: %s\n", shm_child);

//         // modify shared region
//        strcpy(shm_child, "hello back from child");

//         // exit child
//         shm_close(SHM_KEY);
//         exit(0);
//     } else {
//         // ---- PARENT ----
//         wait(0); // wait for child to finish
//         //printf("shmtest: parent mapped shm at %p\n", shm_parent);

//         printf("parent: after child wrote, shm contains: %s\n", shm_parent);

//         // cleanup
//         shm_close(SHM_KEY);
//     }

//     printf("shmtest: done\n");
//     exit(0);
// }

#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

#define KEY 1234

int
main(int argc, char *argv[])
{
    int pid;
    int mid;

    // create mailbox
    mid = mboxcreate(KEY);
    if(mid < 0){
        printf("mboxcreate failed: %d\n", mid);
        exit(1);
    }
    printf("Mailbox created with index %d and key %d\n", mid, KEY);

    pid = fork();
    if(pid < 0){
        printf("fork failed\n");
        exit(1);
    }

    if(pid == 0){
        // child = receiver
        //sleep(10); // wait a bit to let parent send first
        int msg;
        if(mboxrecv(KEY, &msg) == 0){
            printf("Child received message: %d\n", msg);
        } else {
            printf("Child failed to receive\n");
        }
        exit(0);
    } else {
        // parent = sender
        
        int msg = 42;
        //printf("Parent sending message: %d\n", msg);
        if(mboxsend(KEY, msg) == 0){
            wait(0);
            printf("Parent sent message successfully\n");
        } else {
            wait(0);
            printf("Parent failed to send\n");
        }
    }

    exit(0);
}
