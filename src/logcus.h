#ifndef LOGCUS_H
#define LOGCUS_H
#define _XOPEN_SOURCE 700 // ftruncate()
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <assert.h>
#include <math.h>
#include <time.h>
#include <pthread.h>
#include <stdarg.h>

typedef struct {
	pthread_mutex_t *lock;
	char * message;
} logcus_struct;


char * concat(const char*, const char*);
char * get_timestamp(void);
char * to_string(int n);

void closeall(void);
int create_daemon(void);
void errexit(const char *str);
void errreport(const char *str);
void process(void);
unsigned * create_shared_variable(const char *name);
unsigned * open_shared_variable(const char *name);

int logcus(char *, ...);
int open_logcus(void);
int close_logcus(void);

void *entry_function();
#endif
