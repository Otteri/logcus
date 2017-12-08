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
#include <syslog.h>
#include <time.h>
#include <math.h>


int run = 1;

 void sigint_handler(int sig) {
  printf("Caught signal %d\n", sig);
  printf("killing main process: %d.\n", getpid());
  close_logcus();
  exit(0);
 }



int test_reaction_ability() {
 /**
  * Starts from one seconds. Loops until logcus
  * fails, thus printing the minium sleep time which
  * equals minium delay time between logcus calls.
  *
  * This test isn't good. First of all, the system cannot
  * guarantee nanosecond accuracy even though it can output
  * nanoseconds. This test doesn't show when the logsystem
  * falls apart, it just proves that the logcus can work 
  * quite fast.
  */
  int ret = 0;
  struct timespec ts;
  ts.tv_sec = 0;
  ts.tv_nsec = 1000000000; //1s
  long ms = round(ts.tv_nsec / 1.0e6); // Convert nanoseconds to milliseconds

  // 100ms equals 100000000 nanoseconds
  for(; ts.tv_nsec > 100 && ret >= 0; ) { // 1000ns = 0.001ms

    if(ts.tv_nsec > 100000000 && ts.tv_nsec <= 1000000000) {
      ts.tv_nsec -= 100000000;
    } else if(ts.tv_nsec > 10000000 && ts.tv_nsec <= 100000000) {
      ts.tv_nsec -= 10000000;
    } else if(ts.tv_nsec > 1000000 && ts.tv_nsec <= 10000000) {
      ts.tv_nsec -= 1000000;
    } else if(ts.tv_nsec > 100000 && ts.tv_nsec <= 1000000) {
      ts.tv_nsec -= 100000;
    } else if(ts.tv_nsec > 10000 && ts.tv_nsec <= 100000) {
      ts.tv_nsec -= 10000;
    } else if(ts.tv_nsec > 1000 && ts.tv_nsec <= 10000) {
      ts.tv_nsec -= 1000;
    } else if(ts.tv_nsec > 100 && ts.tv_nsec <= 1000) {
      ts.tv_nsec -= 100;
    } 

    ret = logcus("Log message sent from test1.c program\n"); 
    ms = round(ts.tv_nsec / 1.0e6);
    printf("test_reaction_ability running. [ret: %d] [react: %03ld ms] [react: %.9ld ns]\n", ret, ms, ts.tv_nsec);
    nanosleep(&ts, NULL);
  }
  printf("Reaction time: %.9ld ns\n", ts.tv_nsec);
  return 0;
}


int test_process(int process_number){
  if (open_logcus() < 0) {
    exit(2);
  }
  while(1) {
    printf("process: %d\n", process_number);
    error("wut the fuck%s | %d\n", "homo",10);
    logcus("Log message sent from test_process_%d\n", process_number);
    sleep(3);
  }
  return 0;
}

int test_simultaneous_process_response() {
  /**

   **/
  int is_ready = 0;
  int n;
  int i = 0;
  printf("How many simultaneous processes you want to create?\n");
  scanf("%d", &n);
  pid_t pids[n];
  for( ; i < n+1; i++){
    if((pids[i] = fork()) < 0) {
      perror("forking error\n");
      abort();
    }
    else if (pids[i] == 0) {
      test_process(i);
      exit(0);
    }
  }
  // Test completed, ask processes to exit.
  if(i == n) {
    for(int i = 1; i < n+1; i++){
      kill(pids[i], SIGINT);
    }
  }


  /*
  int n;
  printf("How many simultaneous processes you want to create?\n");
  scanf("%d", &n);
  logcus("Starting test\n");
  for(int i = 1; i < n+1; i++){
   
    printf("simultaneous processes currently: %d\n", i);
    switch (fork()) {
      case 0:  break;//logcus("Log message sent from test1.c program\n");  // Child
      case -1: return -1; 
      default: test_process(i);  // Parent
    }
    sleep(2);
  }*/

  return 0;
}

int test_average_latency() {
  return 0;
}

int test_log_output() {
  /*Open syslog daemon and log.txt and compare results visually.
    I didn't want to have exact same format in my log, so its 
    better just to check correcpondece instead of writing 
    'assertqeual tests'.
  */
  openlog("my_daemon", LOG_PID, LOG_DAEMON);
  for(int i = 0; i <= 5; i++) {
    printf("test_log_output running (%d/5) [pid: %d]\n", i,getpid());
    logcus("Log message sent from test1.c program\n");
    syslog(LOG_INFO, "Log message sent from test1.c program\n");
    sleep(2);
  }
  return 0;
}

int test_millisecond_accuracy() {
  /**
   * This isn't very accurate test, but should be enough
   * to demonstrate the millisecond precise logging ability.
   **/
  return 0;
}



int main(int argc, char *argv[]) {
  (void) argc;
  (void) argv;
  int select;
  signal(SIGINT, sigint_handler);

  if (open_logcus() < 0) {
    exit(2);
  }

  printf("Please select a test by typing its number [1-4].\n");
  printf("1. test_reaction_ability\n");
  printf("2. test_simultaneous_process_response\n");
  printf("3. test_average_latency\n");
  printf("4. test_log_output\n");
  printf("5. test_millisecond_accuracy\n");
  scanf("%d", &select);

  printf("\nStarting selected test\n\n");
  switch(select) {
    case 1: test_reaction_ability(); break;
    case 2: test_simultaneous_process_response(); break;
    case 3: test_average_latency(); break;
    case 4: test_log_output(); break;
    case 5: test_millisecond_accuracy(); break;
    default: printf("Selection error, no such a test available.");
  }

  close_logcus();
  printf("No errors. Shutting down.\n");
  return 0;
}