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
#include "logcus.h"

#define DAEMON_FIFO "/tmp/daemon_fifo"
#define CUSTOM_LOG  NULL // Write custom log path here inside quatation marks.
char CWD[1024]; 				 // Working directory of logcus initalizer.
pthread_mutex_t LOCK;

 /*===================================================*\
  |"Library" provides the following functions for user |
  |                                                    |
  |                 open_logcus(void)                  |
  |                 logcus(char*, ...)                 |
  |                 close_logcus(void)                 |
 \*===================================================*/



char * concat(const char *s1, const char *s2){
 /**
  * Function concatenates two strings togehter.
  * Caller function must free the allocated memory.
  **/
  char *result = malloc(strlen(s1)+strlen(s2)+2);
  strcpy(result, s1);
  strcat(result, s2);
  return result;
}

char * to_string(int n) {
 /**
  * Converts a integer to a string.
  * (For normal ints, 12-chars should be enough).
  **/
  static char str[12];
  sprintf(str, "%d", n);
  return str;
}

char * get_timestamp (void) {
 /**
  * Function returns timestamp in millisecond precision. It does
  * not add a newline at the end of string. Caller function must
  * free the memory mallocated in this function.
  **/
  time_t 	now;
  struct tm ts;
  long ms;
  struct timespec spec;
  char buf[80];

  char* timestamp = malloc(25*sizeof(char));

  time(&now);
  clock_gettime(CLOCK_REALTIME, &spec);
 	ms = round(spec.tv_nsec / 1.0e6); // Convert nanoseconds to milliseconds
  ts = *localtime(&now);
  strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &ts);

  sprintf(timestamp, "%s.%03ld", buf, ms);
  return timestamp;
}


void closeall(void) {
  /* Closes all FDs. Handy in daemon creation.*/
  int fd = 0;
  int fdlimit = sysconf(_SC_OPEN_MAX);
  while (fd < fdlimit)
  close(fd++);
}



unsigned * create_shared_variable(const char *name) {
  /**
  * Creates a variable that can be shared between processes.
  * On my system the variables are stored to /dev/shm/
  * the variable needs to be freed by calling shm_unlink().
  **/
  int fd;
  unsigned* addr;

  fd = shm_open(name, O_RDWR | O_CREAT, 0666);
  if(fd == -1) {
  return NULL;
  }
  if(ftruncate(fd, sizeof(*addr)) == -1) {
  return NULL;
  }
  addr = mmap(0, sizeof(*addr), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
  if(addr == MAP_FAILED) {
  return NULL;
  }
  return addr;
}

unsigned * open_shared_variable(const char *name) {
  /**
  * Opens a memory shared variable with read and write permissions.
  * @Return: -1 on error (e.g. invalid 'name'), otherwise the address
  * of the variable.
  **/
  int fd;
  unsigned* addr;

  fd = shm_open(name, O_RDWR, 0666);
  if(fd == -1) {
  return NULL;
  }
  struct stat sb;
  fstat(fd, &sb);

  addr = mmap(0, sizeof(*addr), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
  if(addr == MAP_FAILED) {
  return NULL;
  }
  return addr;
}



/**
 * Create_daemon():
 * Forks a daemon process and leaves the original caller process alive.
 * Fucntion's skeleton is borrowed from lecture materials.
 * @Return: -1 on failure, 0 for child process and child's pid for parent.
 *
 *  |
 * main
 *  |
 *  |......c1              fork()
 *  |      .
 *  |      .......c2       fork()
 *  |      .      |
 *  |      .      |
 *  |      .      |
 *  ⌄      .      ⌄
 *
 * Daemon (c2) is created by killing c1, thus detaching c2.
 **/

int create_daemon(void) {
  switch(fork()) {
  case 0: // Child. start making daemon.
  // Fork again to sever ties to parent.
  switch (fork())  {
  case 0:  break;     // Child       (continue)
  case -1: return -1; // Fork failed  (return)
  default: _exit(0);  // Parent exits (detach)
  }

  // Get a new session.
  assert(setsid() > 0);               // shouldn't fail

  if(getcwd(CWD, sizeof(CWD)) == NULL){
  printf("Error! Cannot get current working directory\n");
  }

  // Get to the root directory, just in case the current working
  // directory needs to be unmounted at some point.
  chdir("/");

  // Close all open files.
  closeall();

  // open /dev/null. As all fds are closed, the following open
  // will make stdin=/dev/null
  open("/dev/null",O_RDWR);
  dup(0);   // Copy /dev/null also as stdout stream
  dup(0);   // Copy /dev/null also as stderr stream
  return 0; // Now we have a daemon process, we can return.

  case -1: return -1; 					// Fork failed!
  default: return(getpid());  	// Parent. Let it live.
  }
}


/* Reports a terminal error (and quits). */
void errexit(const char *str) {
  syslog(LOG_INFO, "%s failed: %s (%d)", str, strerror(errno),errno);
  exit(1);
}


void process(void) {
 /**
  * Daemon executes this function and does the work defined in here.
  * Daemon's job is to write given messages to a log file, which lies
  * in CUSTOM_LOG path. The messages are forwarded for the daemon through
  * a FIFO pipe. This ensures that the messages are logged correctly from
  * multiple sources, as long as the messages doesn't exceed the buffer size,
  * thus breaking the atomicity of the pipe.
  *
  * Daemon stays active until all processes that has opened logcus by calling
  * open_logcus(), also close the logcus by calling close_logcus().
  **/
  char str[1024];
  FILE *fp;
  int nbytes, fd;

  openlog("my_daemon", LOG_PID, LOG_DAEMON);
  umask(0); // this could be in create_daemon


  // Open or create the log file
  if(CUSTOM_LOG == NULL) {
  char *full_path = concat(CWD, "/log.txt");
  syslog(LOG_INFO,"full_path: %s\n", full_path);
  fp = fopen(full_path, "a+"); //w+ for debug
  free(full_path);
  } else {
  fp = fopen(CUSTOM_LOG, "a+");
  }

  // Create FIFO connection
  if ((mkfifo(DAEMON_FIFO, 0666) == -1) && (errno != EEXIST)) {
   errexit("Failed to create a FIFO");
  }
  if ((fd = open(DAEMON_FIFO, O_RDONLY)) == -1) {
  errexit("Failed to open FIFO");
  }

  // Start processing
  unsigned * processes_using_logcus = open_shared_variable("/processes_using_logcus");
  while(*processes_using_logcus > 0) {
  syslog(LOG_INFO, "daemon is active (loop). pul: %d\n", *processes_using_logcus);
  nbytes = read(fd,str,sizeof(str));
  if (nbytes > 0) {
  str[nbytes]='\0';
  syslog(LOG_INFO, "Someone sent me a job to do: %s", str);
  fprintf(fp, "%s", str);
  fflush(fp);
  }
  }
  // Close and remove temporary contents - do a clean exit.
  syslog(LOG_INFO, "Daemon is exiting\n");
  remove(DAEMON_FIFO);
  shm_unlink("/processes_using_logcus");
  close(fd);
  fclose(fp);
  return;
}


void *entry_function(void *args) {
  logcus_struct *args_ptr = args;

  pthread_mutex_lock(args_ptr->lock);


  unsigned * processes_using_logcus = open_shared_variable("/processes_using_logcus");
  if(processes_using_logcus == NULL) {
  errexit("Error! No daemon initalized. Have you called open_logcus?");
  }
  // No errors detected (yet) - write event to the log
  char full_message[strlen(args_ptr->message)+33 * sizeof(char)]; // timestamp + pid + '\0' = 33
  char *pid = to_string(getpid());
  char *timestamp = get_timestamp();
  if((sprintf(full_message, "%s [%s]: %s\n", timestamp, pid, args_ptr->message)) == -1) {
  fprintf(stderr, "Error! Cannot form full log message. (Message might be over the buffer).");
  free(timestamp);
  return (void *)-1;
  }
  free(timestamp);
  int fd = open(DAEMON_FIFO, O_WRONLY);
  write(fd, full_message, strlen(full_message)+1);

  pthread_mutex_unlock(args_ptr->lock);
  return 0;
}

int logcus(char *message, ...) {
 /**
  * First checks that daemon process is running by checking that shared variable
  * (processes_using_logcus) exists. (The variable is created when daemon is initalized).
  * After verifying that daemon exists, the message can be passed through the FIFO to be
  * written in log file.
  **/

  pthread_t queuer;
  char tmp[1024];

  va_list arguments;
  va_start(arguments, message);
  vsprintf(tmp, message, arguments);
  va_end(arguments);

  logcus_struct *args = malloc(sizeof(logcus_struct)); // move this? - don't want t
  args->lock = &LOCK;
  args->message = malloc(strlen(tmp)+1);
  strcpy(args->message, tmp);

  if(pthread_create(&queuer, NULL, entry_function, args)) {
  free(args);
  fprintf(stderr, "ERROR! Cannot create queuer thread\n");
  return -1;
  }

  if(pthread_join(queuer, NULL)) {
  fprintf(stderr, "ERROR! Cannot join threads\n");
  return -1;
  }
  free(args->message);
  free(args);
  return 0;
}

int open_logcus(void) {
  /**
   * Function checks if daemon process exists. If it does not, then we create one.
   * We always increment the memory shared variable by one to address that more
   * processes are using the logcus.
   **/
  unsigned *processes_using_logcus = open_shared_variable("/processes_using_logcus");
  // Check if logcus needs to be initalized (first call), and possibly initalize.
  if(processes_using_logcus == NULL || *processes_using_logcus == 0) {
  //*processes_using_logcus = 0; // just to be sure
  if(pthread_mutex_init(&LOCK, NULL) != 0) {
  fprintf(stderr, "\n mutex init failed\n");
  return -1;
  }
  processes_using_logcus = create_shared_variable("/processes_using_logcus");
  switch(create_daemon()) {
  case 0:
  process();      		// Let the daemon do its work
  exit(0);        		// We don't want to leave daemon hanging
  case -1: return -1; 	// Error occured
  default: break;	 			// Original process can continue
  }
  }
  processes_using_logcus = open_shared_variable("/processes_using_logcus");
  *processes_using_logcus += 1;
  return 0;
}

int close_logcus(void) {
  /* Function decreases shared memory variable by one, thus 'signaling'
   * that less processes are using logcus. If shared variable goes below
   * one, then we can close connections and files and shut down the logcus.
   * @Return: -1 on error, 1 when last process closes and 0 on normal close
   */
  unsigned * processes_using_logcus = open_shared_variable("/processes_using_logcus");
  if(processes_using_logcus == NULL) {
  fprintf(stderr, "ERROR! Cannot open variable - unable to do a clean exit");
  return -1;
  }
  *processes_using_logcus -= 1; // TODO: fix race condition

  if(*processes_using_logcus < 1) {
  printf("\nCompletely shutting down service.\n");
  shm_unlink("/processes_using_logcus");
  munmap(processes_using_logcus, sizeof(processes_using_logcus));
  return 1;
  }
  //printf("active processes after close: %d\n", *processes_using_logcus);
  //*processes_using_logcus = 0;
  return 0;
}

#endif
