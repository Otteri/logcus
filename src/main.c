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


 void sigint_handler(int sig) {
  logcus("Caught signal: %d. Killing process\n", sig);
  close_logcus();
  exit(0);
 }

int main(int argc, char *argv[]) {
  (void) argc;
  (void) argv;
  signal(SIGINT, sigint_handler);
	if (open_logcus() < 0) {
		perror("daemon");
		exit(2);
	}

  while(1) {
    printf("main is running... [press ^C to stop]\n");
    logcus("string sent from %s program\n", "main.c");
    sleep(1);
  }
  close_logcus();
  return 0;
}
