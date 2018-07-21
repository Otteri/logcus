 #include "test.h"

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

///////////////////////////////////////////////////////////////////////////////
// Function tests logcus' reaction time by sending log messages
// with decreasing intervals. Function starts from one second
// and loops until logcus crashes or message interval meets 1000ns.
// It makes no sense to test under 1000ns response, because the results
// wouldn't be very accurate due to implementation of this function and
// because the standards/hardware cannot guarntee nanosecond accuracy.
// --------------------------------------------------------------------
// It seems that the logcus goes through this test without problems,
// so basically the test just prove that the logcus can work quite
// fast. The function peak is around 6 messages in 0.01 ms, so
// 1000ns should be quite good limit. The limit in for loop can be
// changed to another.
int test_reaction_ability() {
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

// Opens logcus and repeadetly spams the same log message.
// Process stuck in this function has to be terminated with signal.
// Used in test_simultaneous_process_response
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


///////////////////////////////////////////////////////////////////////////////
// This test forks new processes, which use logcus for sending
// log messages, thus stressing testing the capabilities of logcus.
//
// Notice that the test stresses system with n+1 processes, because
// logcus is opened also in main.
//
// Function implementation:
// Child's pid value cannot be 0 when accessing from parent.
// For this reason, we can use pid array's last item as a flag.
// We set the value to 0 manually, and when the value changes to
// actual pid, we know that all processes are created and parent 
// can contininue to next part in code.
//
// Currently there is a small problem with the function. It doesn't 
// guarntee that the childs have enough time to open the logcus and 
// send messages, since the function only checks that all childs are 
// created before moving to termination part.
int test_simultaneous_process_response() {
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
      printf("Created a process. Pid: %d\n", getpid());
      test_process(getpid());
    }
    // Let's give little time for child to open logcus
    // without this, all messages wont get logged (not sure why,
    // tested with syslog also and it left them also out).
    nanosleep((const struct timespec[]){{0, 100000}}, NULL);
  }
  
  // Just wait
  while(pids[n] == 0) {
    sleep(1);
  }

  // Test completed; all childs alive; do a clean exit.
  for(int i = 0; i < n+1; i++){
    kill(pids[i], SIGTERM);
  }
  return 0;
}


// Open syslog daemon and log.txt and compare results visually.
// I didn't want to have exact same format in my log, so its 
// better just to check the correspondece visually instead of 
// writing tests here (...and I'm feeling sleepy now).
int test_log_output() {
  openlog("my_daemon", LOG_PID, LOG_DAEMON);
  for(int i = 0; i <= 5; i++) {
    printf("test_log_output running (%d/5) [pid: %d]\n", i,getpid());
    logcus("Log message sent from test1.c program\n");
    syslog(LOG_INFO, "Log message sent from test1.c program\n");
    sleep(2);
  }
  return 0;
}


// Provides a command-line interface for running different
// tests from this file. './test' runs this function.
int main(int argc, char *argv[]) {
  (void) argc;
  (void) argv;
  int select;
  signal(SIGINT, sigint_handler);
  signal(SIGTERM, sigterm_handler);

  if (open_logcus() < 0) {
    exit(2);
  }

  printf("PLEASE SELECT A TEST BY TYPING ITS NUMBER [1-3].\n");
  printf("1. test_reaction_ability\n");
  printf("2. test_simultaneous_process_response\n");
  printf("3. test_log_output\n");
  scanf("%d", &select);

  printf("\nStarting selected test\n\n");
  switch(select) {
    case 1: test_reaction_ability(); break;
    case 2: test_simultaneous_process_response(); break;
    case 3: test_log_output(); break;
    default: printf("Selection error, no such a test available.");
  }

  close_logcus();
  printf("No errors. Shutting down.\n");
  return 0;
}