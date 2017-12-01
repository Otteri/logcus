#define _XOPEN_SOURCE 700 //ftruncate()
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


#define DAEMON_FIFO "/tmp/daemon_fifo"
#define CUSTOM_LOG  NULL
char CWD[1024]; //logcus initalizer working directory


char * concat(const char *s1, const char *s2){
  char *result = malloc(strlen(s1)+strlen(s2)+2);
  strcpy(result, s1);
  strcat(result, s2);
  return result;
}

char * to_string(int n) {
  // for normal ints, 12-chars should be enough.
  static char str[12];
  sprintf(str, "%d", n);
  return str;
}

char * get_timestamp (void) {
 /**
  * Function returns timestamp in millisecond precision. It does
  * not add a newline at the end of string. Calling function must
  * free the memory.
  */
	time_t 	now;
	struct tm ts;
  long ms;
  struct timespec spec;
  char buf[80];

  //char timestamp[80];
  char* timestamp = malloc(25*sizeof(char));

  time(&now);
  clock_gettime(CLOCK_REALTIME, &spec);
 	ms = round(spec.tv_nsec / 1.0e6); // Convert nanoseconds to milliseconds
  ts = *localtime(&now);
  strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &ts);
  
  sprintf(timestamp, "%s.%03ld", buf, ms);
  printf("sprintf: %s\n", timestamp);
  return timestamp;
}


/* closeall() -- close all FDs >= a specified value */
void closeall(void) {
	int fd = 0;
	int fdlimit = sysconf(_SC_OPEN_MAX);
	while (fd < fdlimit)
		close(fd++);
}

/**
 * Daemonize the process - detach process from user and disappear into the
 * background.
 *
 * Returns -1 on failure, but you can't do much except exit in
 * that case since we may already have forked.  This is based on the BSD
 * version, so the caller is responsible for things like the umask, etc.
 * It's believed to work on all Posix systems. 
 *
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
 * We can create daemon by killing c1, thus detaching c2.
 *
 */

unsigned * create_shared_variable(const char *name) {

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


int create_daemon(void) {
	/* Creates a daemon process and let's the calling process live
	 * 
	 * @return: returns parend pid for parent and 0 for daemon.
	 */
	// TODO ADD one more fork? - disable later TTY access

	// Create new process which becomes daemon.
	switch(fork()) {
		case 0: // Child. start making daemon.
			
			// Fork again to sever ties to parent.
			switch (fork())  {
				case 0:  break;     /* We're the child, ok. */
				case -1: return -1; /* Fork failed! */
				default: _exit(0);  /* Parent exits. */
			}

			printf("My pid %d, my ppid %d, my gpid %d\n",getpid(),getppid(),getpgrp());

			// Get a new session.
			assert(setsid() > 0);               /* shouldn't fail */

			printf("Daemon my pid %d, my ppid %d, my gpid %d\n",getpid(),getppid(),getpgrp());

			if(getcwd(CWD, sizeof(CWD)) == NULL){
				printf("Error! Cannot get current working directory\n");
			}

			/* Get to the root directory, just in case the current working
			 * directory needs to be unmounted at some point. */
			chdir("/");

			/* Close all open files. */
			closeall();

			/* open /dev/null. As all fds are closed, the following open
			 * will make stdin=/dev/null  */
			open("/dev/null",O_RDWR);
			dup(0);  /* Copy /dev/null also as stdout stream */
			dup(0);  /* Copy /dev/null also as stderr stream */

			// Now we're a daemon, a lonely avenger fighting for the cause.
			return 0;

		case -1: return -1; 					// Fork failed!
		default: return(getpid());  	// Parent. Let it live.
	}
}


/* No STDOUT anymore, we use syslog to report things */

/**
 * Reports a terminal error (and quits). */
void errexit(const char *str) {
	syslog(LOG_INFO, "%s failed: %s (%d)", str, strerror(errno),errno);
	exit(1);
}

/**
 * Sends a generic error message, not fatal. */
void errreport(const char *str) {
	syslog(LOG_INFO, "%s failed: %s (%d)", str, strerror(errno),errno);
}

/* This is the daemon's main work -- listen for messages and then do something */
void process(void) {
	char str[1024];
	FILE *fp;
	int nbytes, fd;

	openlog("my_daemon", LOG_PID, LOG_DAEMON);
	umask(0); //here?
	

	// Open/create log file
	if(CUSTOM_LOG == NULL) {
		char *full_path = concat(CWD, "/log.txt");
		syslog(LOG_INFO,"full_path: %s\n", full_path);
		fp = fopen(full_path, "w+");
		free(full_path);
	} else {
		fp = fopen(CUSTOM_LOG, "w+");
	}

	// Create FIFO connection
	if ((mkfifo(DAEMON_FIFO, 0666) == -1) && (errno != EEXIST)) {
		 errexit("Failed to create a FIFO");
	}
	if ((fd = open(DAEMON_FIFO, O_RDONLY)) == -1) {
		errexit("Failed to open FIFO");
	}


	syslog(LOG_INFO,"-DAEMON- My pid %d, my ppid %d, my gpid %d\n",getpid(),getppid(),getpgrp());
	
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
	syslog(LOG_INFO, "daemon is out of loop! -Closing\n");
	remove(DAEMON_FIFO);
	close(fd);
	fclose(fp);
	return;
}

int logcus(char *message) {
 /**
	* First checks that daemon process is running by checking that shared variable
	* "processes_using_logcus" exists. (The variable is created when daemon is initalized).
	* After verifying that daemon exists, the message can be passed through FIFO to be
	* written in log file.
	*/
	char full_message[1024];
	unsigned * processes_using_logcus = open_shared_variable("/processes_using_logcus");
	if(processes_using_logcus == NULL) {
		fprintf(stderr, "Error! No daemon initalized. Have you called open_logcus?");
		return -1;
	}

	char *pid = to_string(getpid());
	char *timestamp = get_timestamp();
	if((sprintf(full_message, "%s [%s]: %s\n", timestamp, pid, message)) == -1) {
		fprintf(stderr, "Error! Cannot form full log message. (Message might be over max 1024 bytes).");
		free(timestamp);
		return -1;
	}
	free(timestamp);

	printf("-processes %d\n", getppid());
	int fd = open(DAEMON_FIFO, O_WRONLY);
	write(fd, full_message, strlen(full_message)+1);
	printf("log msg: %s\n", full_message);

	return 0;
}

int open_logcus(void) {
	/* Function checks if daemon process exists. If it does not, then we create one.
   * Then we increment the memory shared variable by one to address that
   * more processes are using the logcus.
	 */

	unsigned *processes_using_logcus = open_shared_variable("/processes_using_logcus");
	// Check if logcus needs to be initalized (first call), and possibly initalize.
	if(processes_using_logcus == NULL || *processes_using_logcus == 0) {
		processes_using_logcus = create_shared_variable("/processes_using_logcus");
		switch(create_daemon()) {
			case 0:
				process();      		// Let the daemon do its work
				exit(0);        		// We don't want to leave daemon hanging
			case -1: return -1; 	// Error occured
			default: break;	 			// Original process can continue
		}
	}
	printf("active processes when opening: %d\n", *processes_using_logcus);
	*processes_using_logcus = 0; // just to be sure
	// Always increment process counter by one, when this function is called.
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
		printf("ERROR! Cannot open variable - unable to do a clean exit");
		return -1;
	}
	*processes_using_logcus -= 1; // TODO: fix race condition
	
	if(*processes_using_logcus < 1) {
		printf("shut down\n");
		shm_unlink("/processes_using_logcus");
		munmap(processes_using_logcus, sizeof(processes_using_logcus));
		return 1;
	}
	printf("active processes after close: %d\n", *processes_using_logcus);
	//*processes_using_logcus = 0;
	return 0;
}

