#define _XOPEN_SOURCE 700 //getline
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <assert.h> //assert
#include <fcntl.h> //open()
#include <sys/stat.h> //O_WRONLY
#include <signal.h>
#include <errno.h>


int fd;
char *myfifo = "/tmp/myfifo";
int is_running = 1;
char buffer[100]; //this to config?

/* Notes. Only one daemon. If daemon already exists, do not create a new one
 *
 */
int logcus(char *msg) {
	// TODO: First check if daemon exists.
	fd = open(myfifo, O_WRONLY);
	write(fd, msg, (strlen(msg)+1));
	return 0;
}


int close_logcus() {
	is_running = 0;
	printf("closing logcus\n");
	fd = open(myfifo, O_WRONLY);
	write(fd, "terminate", 10);
	return(EXIT_SUCCESS);
}


int check_logcus() {
/* Checks current status of daemon process.
 * pid.tmp should exists only if daemon is alive.
 * However, it is possible that unclean exits hasn't wiped the file, so
 * it is better to check that process with the id really does exists
 */
	FILE * pid_file = NULL;
	char * line = NULL;
	size_t len = 0;

 	pid_file = fopen ("/tmp/pid.txt", "r");
	if(pid_file == NULL) {
		printf("ERROR! cannot open pid.tmp");
	}
	getline(&line, &len, pid_file);
	int pid = atoi(line);
	printf("%s", line);
	if(kill(pid, 0) == 0){
		//process exists
		printf("process exists\n");
	} else if(errno == ESRCH) {
		printf("daemon does not exist\n");
	} else {
		printf("error!\n");
	}

	return(EXIT_SUCCESS);
}



/* Init forks a child process. The parent returns and continues executing
 * its own code, if child was created succesfully. The child forks another child
 * (lapsenlapsi) and then gets killed, so the grandchild gets detached thus
 * making a new daemon process.
 */

int init_logcus() {
	FILE *fp= NULL;
	pid_t process_id1 = 0;  // child - (will be killed)
	pid_t process_id2 = 0; // grandchild -> daemon
	pid_t sid = 0;

	process_id1 = fork(); // Create a child process
	if (process_id1 < 0) {
		printf("fork failed!\n");
		exit(EXIT_FAILURE);
	}
	else if (process_id1 == 0) {
		/* ---CHILD PROCESS--- */
		process_id2 = fork(); // create new child (grandchild)
		if (process_id2 < 0) {
			printf("fork failed!\n");
			exit(EXIT_FAILURE);
		}
		if (process_id2 > 0) {
			/* Orphanate/detach grandchild by killing its parent.*/
			printf("This process is becoming a daemon: %d \n", process_id2); //daemon PID
			FILE *file_open = fopen("/tmp/pid.txt", "w+");
			fprintf(file_open, "%d\n", process_id2);
			fclose(file_open);
			exit(EXIT_SUCCESS);
		}
		/* ---GRANDCHILD---*/
		umask(0); // unmask the file mode - new permissions
		sid = setsid(); // set new session
		if(sid < 0) {
			printf("Setting new session failed\n");
			exit(EXIT_FAILURE);
		}
		// Change the current working directory to root.
		if((chdir("/")) < 0){
			exit(EXIT_FAILURE);
		}
		// Close file descriptors (stdin, stdout and stderr).
		int x;
		for (x = sysconf(_SC_OPEN_MAX); x>=0; x--) {
			close (x);
		}


		// TODO: error handling
		mkfifo(myfifo, 0666);

		// Open a log file in write mode.
		fd = open(myfifo, O_RDONLY);
		if(fd == -1){
			exit(EXIT_FAILURE);
		}
		fp = fopen ("/tmp/Log.txt", "w+");

		/* DAEMON INITALIZED START MAINLOOP */
		while (strcmp(buffer,"terminate") != 0)	{
			//Dont block context switches, let the process sleep for some time
			fprintf(fp, "Daemon running\n");
			sleep(1);
			int nbytes = read(fd, buffer, sizeof(buffer));
			if(nbytes > 0) {
				fprintf(fp, "%s\n", buffer);
			}
			fflush(fp);
			// Implement and call some function that does core work for this daemon.
		}
		fprintf(fp, "closing!\n");
		fclose(fp);
		remove("/tmp/pid.tmp");
		close(fd); //close fifo
		return(EXIT_SUCCESS);
	}
	else {
		/*---PARENT--- */
		return (EXIT_SUCCESS); // continue executing the program
	}

	// Program shouldn't get here.
	return (EXIT_FAILURE);
}
