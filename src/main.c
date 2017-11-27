#include <stdio.h>
#include <stdlib.h>
#include <unistd.h> //sleep
#include <string.h>
#include "logcus.h" //currently linked in makefile
#include <signal.h>

int run = 1;
/*    Rough program structure:
 * ------------------------------------------------
 *
 */
 void sigint_handler(int sig) {
   printf("Caught signal %d\n", sig);
   printf("killing main process: %d.\n", getpid());
   run = 0;
   exit(0);
 }

int main(int argc, char *argv[]) {
  (void) argc;
  (void) argv;
  //char string[] = "Hello World!\n";

  printf("Hello!\n");
  init_logcus();

  while(run) {
    printf("main is running...\n");
    signal(SIGINT, sigint_handler);
    logcus("string sent from main.c program\n");
    check_logcus();
    //write(input, string, (strlen(string)+1));
    sleep(2);
  }
  close_logcus();
  printf("...main terminating...\n");
  return 0;
}
