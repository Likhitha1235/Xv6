#define SBRK_ERROR ((char *)-1)

struct stat;

// system calls
int fork(void);
int exit(int) __attribute__((noreturn));
int wait(int*);
int pipe(int*);
int write(int, const void*, int);
int read(int, void*, int);
int close(int);
int kill(int);
int exec(const char*, char**);
int open(const char*, int);
int mknod(const char*, short, short);
int unlink(const char*);
int fstat(int fd, struct stat*);
int link(const char*, const char*);
int mkdir(const char*);
int chdir(const char*);
int dup(int);
int getpid(void);
char* sys_sbrk(int,int);
int pause(int);
int uptime(void);

int shm_create(int);
uint64 shm_get(int);
int shm_close(int);

int mboxcreate(int);
int mboxsend(int,int);
int mboxrecv(int,int* );
// ulib.c
int stat(const char*, struct stat*);
char* strcpy(char*, const char*);
void *memmove(void*, const void*, int);
char* strchr(const char*, char c);
int strcmp(const char*, const char*);
char* gets(char*, int max);
uint strlen(const char*);
void* memset(void*, int, uint);
int atoi(const char*);
int memcmp(const void *, const void *, uint);
void *memcpy(void *, const void *, uint);
char* sbrk(int);
char* sbrklazy(int);
long strtol(const char *s, char **endptr, int base);

// printf.c
void fprintf(int, const char*, ...) __attribute__ ((format (printf, 2, 3)));
void printf(const char*, ...) __attribute__ ((format (printf, 1, 2)));

// umalloc.c
void* malloc(uint);
void free(void*);



// Task 2 Begin
// Statistics for demand paging
struct pagestat {
  int num_pagefaults;
  int num_swapins;
  int num_swapouts;
};
// Task 2 End

// Task 2 Begin
int getpagestat(int pid, struct pagestat *st);
int dumpmru(void);
// Task 2 End

// Add system call declarations:
int symlink(const char *target, const char *path);


// Add structure definition:
