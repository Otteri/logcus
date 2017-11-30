#define _XOPEN_SOURCE 700 //ftruncate()
#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <assert.h>

#define DAEMON_FIFO "/tmp/daemon_fifo"
#define FIFO_PERMS (S_IRWXU | S_IWGRP| S_IWOTH)









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
 *
 * main
 *  |
 *  |......c1              fork()
 *  |      .
 *  |      .......c2       fork()
 *  |      .      |
 *  |      .      |
 *  |      .      |
 *
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
			syslog(LOG_INFO, "created daemon");
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
	int len, fd;

	openlog("my_daemon", LOG_PID, LOG_DAEMON);
	umask(0);
	
	if ((mkfifo(DAEMON_FIFO, FIFO_PERMS) == -1) && (errno != EEXIST)) {
		 errexit("Failed to create a FIFO");
	}
	
	if(errno == EEXIST) {
		struct stat stat_buf;
		  
		if(stat(DAEMON_FIFO,&stat_buf) != 0)
			errexit("stat");
		if(S_ISFIFO(stat_buf.st_mode))
			syslog(LOG_INFO,"File %s exists, and is a FIFO, not problem...",DAEMON_FIFO);
		else
			errexit("File exists, and is not a FIFO, exiting...");
	}

	/*if ((fd = open(DAEMON_FIFO, O_RDONLY)) == -1) 
		errexit("Failed to open FIFO");*/

	syslog(LOG_INFO,"-DAEMON- My pid %d, my ppid %d, my gpid %d\n",getpid(),getppid(),getpgrp());
	unsigned * processes_using_logcus = open_shared_variable("/processes_using_logcus");

	while(*processes_using_logcus > 0) {
		syslog(LOG_INFO, "daemon is active (loop). pul: %d\n", *processes_using_logcus);
		/*len = read(fd,str,1024);
		if (len>0) {
			str[len]='\0';
			syslog(LOG_INFO, "Someone sent me a job to do: %s", str);
		}*/
	}
	syslog(LOG_INFO, "daemon is out of loop! -Closing\n");
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
		*processes_using_logcus = 0; //just to be safe
		switch(create_daemon()) {
			case 0:
				process();      		// Let the daemon do its work
				exit(0);        		// We don't want to leave daemon hanging
			case -1: return -1; 	// Error occured
			default: break;	 			// Original process can continue
		}
	}

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

