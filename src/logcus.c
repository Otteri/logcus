#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>

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



		// Open a log file in write mode.
		fp = fopen ("Log.txt", "w+");
		/* DAEMON INITALIZED START MAINLOOP */
		while (1)	{
			//Dont block context switches, let the process sleep for some time
			sleep(1);
			fprintf(fp, "Logging info...\n");
			fflush(fp);
			// Implement and call some function that does core work for this daemon.
		}
		fclose(fp);
		return(EXIT_SUCCESS);
	}
	else {
		/*---PARENT--- */
		return(EXIT_SUCCESS); // continue executing the program
	}

	// Program shouldn't get here.
	return (EXIT_FAILURE);
}
