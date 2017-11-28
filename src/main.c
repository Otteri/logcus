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


int run = 1;
//static int *using_logcus;
/*    Rough program structure:
 * ------------------------------------------------
 *
 */
 void sigint_handler(int sig) {
  printf("Caught signal %d\n", sig);
  printf("killing main process: %d.\n", getpid());
  close_logcus();
  exit(0);
 }

int main(int argc, char *argv[]) {
  (void) argc;
  (void) argv;
  printf("Hello!\n");
  init_logcus();

  while(run) {
    printf("main is running...\n");
    signal(SIGINT, sigint_handler);
    logcus("string sent from main.c program\n");
    sleep(2);
  }
  close_logcus();
  printf("...main terminating...\n");
  return 0;
}
