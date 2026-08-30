// #include "kernel/types.h"
// #include "kernel/stat.h"
// #include "kernel/fcntl.h"
// #include "user/user.h"

// #define SIZE (300*1024) // 300 KB to exceed old limit

// int main() {
//     int fd = open("bigfile", O_CREATE | O_RDWR);
//     char buf[512];
//     for(int i=0;i<512;i++) buf[i]=i%256;

//     for(int i=0;i<SIZE/512;i++)
//         write(fd, buf, 512);

//     close(fd);

//     fd = open("bigfile", O_RDONLY);
//     for(int i=0;i<SIZE/512;i++){
//         read(fd, buf, 512);
//         for(int j=0;j<512;j++){
//             if(buf[j] != j%256) {
//                 printf("Data mismatch!\n");
//                 exit(0);
//             }
//         }
//     }
//     printf("Test passed!\n");
//     close(fd);
//     exit(0);
// }

#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fcntl.h"

int main(){

    if(fork() == 0){
      printf("i am child\n");
    }else {
        wait(0);
        printf("i am parent\n");
    }
}