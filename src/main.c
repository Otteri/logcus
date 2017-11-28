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
   //run = 0;
   exit(0);
 }

/*
 unsigned * open_shared() {
  // Create new obj
  int shared_fd = shm_open("/users", O_RDWR, 0666);
  if(shared_fd == -1) {
    fprintf(stderr, "Main Cannot create shared object: using_logcus. %s\n",
    strerror(errno));
    //return;
  }
  struct stat sb;
  fstat(shared_fd, &sb);
  //off_t length = sb.st_size;
  unsigned *addr = mmap(0, sizeof(*addr), PROT_READ | PROT_WRITE, MAP_SHARED, shared_fd, 0);
  if(addr == MAP_FAILED) {
    fprintf(stderr, "mmap failed: %s\n", strerror(errno));
    //return;
  }
  return addr;
}*/

int main(int argc, char *argv[]) {
  (void) argc;
  (void) argv;
  //char string[] = "Hello World!\n";

  printf("Hello!\n");
  init_logcus();


  sleep(3);
  /*
  // Create new obj
  int shared_fd = shm_open("/users", O_RDWR, 0666);
  if(shared_fd == -1){
    fprintf(stderr, "Main Cannot create shared object: using_logcus. %s\n",
    strerror(errno));
    return(EXIT_FAILURE);
  }
  struct stat sb;
  fstat(shared_fd, &sb);
  //off_t length = sb.st_size;
  unsigned *addr = mmap(0, sizeof(*addr), PROT_READ | PROT_WRITE, MAP_SHARED, shared_fd, 0);
  if(addr == MAP_FAILED) {
    fprintf(stderr, "mmap failed: %s\n", strerror(errno));
    return EXIT_FAILURE;
  }
 */
  //unsigned * addr = open_shared();

  while(run) {
    printf("main is running...\n");
    signal(SIGINT, sigint_handler);
    logcus("string sent from main.c program\n");
    check_logcus();
    //write(input, string, (strlen(string)+1));
    //fprintf(stdout, "ADDR: %d\n", *addr);
    sleep(2);
    /*using_logcus = mmap(NULL, sizeof *using_logcus, PROT_READ | PROT_WRITE, MAP_SHARED, -1, 0);
		printf("Using: %d", *using_logcus);*/

  }
  close_logcus();
  printf("...main terminating...\n");
  return 0;
}
