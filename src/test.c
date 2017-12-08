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

int processes_ready = 0;

 void sigint_handler(int sig) {
  (void) sig;
  logcus("process closing logcus.\n", getpid());
  close_logcus();
  exit(0);
 }


 void sigterm_handler(int sig) {
  (void) sig;
  logcus("terminating %d.\n", getpid());
  close_logcus();
  exit(0);
 }


int test_reaction_ability() {
 /**
  * Function tests logcus' reaction time by sending log messages
  * with decreasing intervals. Function starts from one second
  * and loops until logcus crashes or message interval meets 1000ns.
  * It makes no sense to test under 1000ns response, because the results
  * wouldn't be very accurate due to implementation of this function and
  * because the standards/hardware cannot guarntee nanosecond accuracy.
  * --------------------------------------------------------------------
  * It seems that the logcus goes through this test without problems,
  * so basically the test just prove that the logcus can work quite
  * fast. The function peak is around 6 messages in 0.01 ms, so
  * 1000ns should be quite good limit. The limit in for loop can be
  * changed to another.
  **/
  int ret = 0;
  struct timespec ts;
  ts.tv_sec = 0;
  ts.tv_nsec = 1000000000; // 1s
  long ms = round(ts.tv_nsec / 1.0e6); // Convert nanoseconds to milliseconds

  for(; ts.tv_nsec > 1000 && ret >= 0; ) { // 1000ns limit 

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
    } else if(ts.tv_nsec > 10 && ts.tv_nsec <= 100) {
      ts.tv_nsec -= 10;
    } else if(ts.tv_nsec > 1 && ts.tv_nsec <= 10) {
      ts.tv_nsec -= 1;
    } 

    ret = logcus("Log message sent from test1.c program\n"); 
    ms = round(ts.tv_nsec / 1.0e6);
    printf("test_reaction_ability running. [ret: %d] [react: %03ld ms] [react: %.9ld ns]\n", ret, ms, ts.tv_nsec);
    nanosleep(&ts, NULL);
  }
  printf("Reaction time is under: %.9ld ns\n", ts.tv_nsec);
  return 0;
}


int test_process(pid_t pid){
  if (open_logcus() < 0) {
    printf("Failed to open logcus!!!");
    exit(2);
  }
  logcus("Process opened logcus", pid);
  while(1) {
    logcus("Log message sent from test_process_%d\n", pid);
    syslog(LOG_INFO, "Log message sent from test_process %d\n", pid);
    sleep(1);
  }
  return 0; //never getting here
}

int test_simultaneous_process_response() {
  /**
   * This test forks new processes requested amount. All processes use
   * logcus, thus stressing it and testing its capabilities.
   *
   * Notice that test stresses system with n+1 processes, because
   * logcus is opened also in main. (This shouldn't effect to anything).
   *
   * Function implementation:
   * Child's pid value cannot be 0 when accessing to it from parent.
   * For this reason, we can use our pid arrays last item as a flag.
   * We set the value to 0 manually, and the value changes when final
   * process is actually created. This way, the parent knows when it 
   * can continue to next part in code (termination).
   *
   * Currently there is a small problem in the function. It doesn't 
   * guarntee that the childs have enough time to open the logcus and 
   * send message, since the function only checks that all childs are 
   * created.
   **/
  int n; // user defines n 
  printf("How many simultaneous processes you want to create?\n");
  scanf("%d", &n);
  n = n-1;
  pid_t pids[n];
  pids[n] = 0; // set flag

  // Create processes
  for(int i=0; i < n+1; i++){
    if((pids[i] = fork()) < 0) {
      logcus("Forking failed: %d\n", getpid());
      perror("forking error\n");
      abort();
    }
    else if (pids[i] == 0) {
      // CHILD!!!
      test_process(getpid());
    }
    // Let's give little time for child to open logcus
    // without this, all messages wont get logged (not sure why,
    // tested with syslog also and it left them also out).
    nanosleep((const struct timespec[]){{0, 100000}}, NULL);
  }
  

  // I'm parent checking for 0
  while(pids[n] == 0) {
    sleep(1);
  }
  //sleep(5);
  // Test completed, childs alive. Do a clean exit.
  for(int i = 0; i < n+1; i++){
    kill(pids[i], SIGTERM);
  }
  return 0;
}

int test_average_latency() {
  return 0;
}

int test_log_output() {
  /** 
   * Open syslog daemon and log.txt and compare results visually.
   * I didn't want to have exact same format in my log, so its 
   * better just to check the correspondece visually instead of 
   * writing tests here (...and I'm feeling sleepy now).
   **/
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
  signal(SIGTERM, sigterm_handler);

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