#define _XOPEN_SOURCE 700 //getline
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h> //sleep
#include <string.h>
#include "logcus.h" //currently linked in makefile
#include <signal.h>

//tmp includes
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include <fcntl.h> //open()
#include <sys/stat.h> //O_WRONLY
#include <fcntl.h>  //O_
#include <unistd.h>
#include <time.h>

int run = 1;

 void sigint_handler(int sig) {
  printf("Caught signal %d\n", sig);
  printf("killing main process: %d.\n", getpid());
  close_logcus();
  exit(0);
 }

int main(int argc, char *argv[]) {
  (void) argc;
  (void) argv;
  struct timespec ts;
  ts.tv_sec = 0;
  ts.tv_nsec = 100000000;

  printf("My pid %d, my ppid %d, my gpid %d\n",getpid(),getppid(),getpgrp());
	if (open_logcus() < 0) {
		perror("daemon");
		exit(2);
	}


  while(run) {
    printf("main is running... [press ^C to stop]\n");
    signal(SIGINT, sigint_handler);
    logcus("string sent from main.c program\n");
    sleep(1);
    //usleep(3*1000);
    nanosleep(&ts, NULL);
  }
  close_logcus();
  printf("...main terminating...\n");
  return 0;
}
