#define _XOPEN_SOURCE 700 //getline
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <string.h>
#include <assert.h> //assert
#include <fcntl.h> //open()
#include <sys/stat.h> //O_WRONLY
#include <fcntl.h>  //O_
#include <signal.h>
#include <errno.h>

int fd;
char *myfifo = "/tmp/myfifo";
int is_running = 1;
char buffer[1024]; //this to config?
//static int *using_logcus = 0; //+1 init <and -1 close. if 0 do not run daemon.
int shared_fd;
unsigned* addr; //tells how many processes are using daemon TODO: ADD LOCK - currently race condition
char *log_path = NULL;


char * concat(const char *s1, const char *s2){
  char *result = malloc(strlen(s1)+strlen(s2)+2);
  strcpy(result, s1);
  strcat(result, s2);
  return result;
}

int shared_add() {
	char *name ="/shared_logcus_count";
	int shared_fd = shm_open(name, O_RDWR | O_CREAT, 0644);
	assert(shared_fd > 0);
	struct stat sb;
	fstat(shared_fd, &sb);
	off_t length = sb.st_size;
	char *ptr = (char *) mmap(NULL, length, PROT_READ|PROT_WRITE, MAP_SHARED, shared_fd, 0);
	ptr[0] = '1';
	return 1;
}

int check_logcus() {
/* TODO: Checks current status of daemon process???.
 * Function checks if daemon process is alive by reading
 * the pid from temporary text file and testing the pid validity
 * with kill. If the temporary file cannot be read, it means that
 * daemon doesn't exist, because file is avlable only when daemon
 * is running;
 *
 * @return: 1 when daemon exists, -1 when it doesn't exist.
 */
	FILE * pid_file = NULL;
	char * line = NULL;
	size_t len = 0;

 	pid_file = fopen("/tmp/pid.txt", "r");
	if(pid_file == NULL) {
		return -1;
	}
	getline(&line, &len, pid_file);
	int pid = atoi(line);
	printf("%s", line);
	if(kill(pid, 0) == 0) {
		//process exists
		printf("process exists\n");
		return 1;
	} else if(errno == ESRCH) {
		printf("daemon does not exist\n");
		return -1;
	}
	// error case
	return(-1);
}



/* Notes. Only one daemon. If daemon already exists, do not create a new one
 *
 */
int logcus(char *msg) {
	// TODO: First check if daemon exists.
	int ret = check_logcus();
	if(ret == 1) {
		fd = open(myfifo, O_WRONLY);
		write(fd, msg, (strlen(msg)+1));
		return 0;
	}
	printf("ERROR! Cannot write. Daemon is not currently running\n");
	return -1;
}


unsigned * create_shared() {
	// This function only creates THE shared variable.
	// wanted to lighten the load from init_function
	// CREATING ADDR
	//TODO: Function for setting follofing objects???

	// Create new obj
	shared_fd = shm_open("/users", O_RDWR | O_CREAT, 0666);
	if(shared_fd == -1){
		fprintf(stderr, "Cannot create shared object: using_logcus. %s\n",
		strerror(errno));
		return(addr = NULL);
	}
	// Set size of memory obj
	if(ftruncate(shared_fd, sizeof(*addr)) == -1){
		fprintf(stderr, "ftruncate: %s\n", strerror(errno));
		return(addr = NULL);
	}
	// Map the memory obj
	addr = mmap(0, sizeof(*addr), PROT_READ | PROT_WRITE, MAP_SHARED, shared_fd, 0);
	if(addr == MAP_FAILED) {
		fprintf(stderr, "mmap failed: %s\n", strerror(errno));
		return(addr = NULL);
	}
	return addr;
}


unsigned * open_shared() {
 // Open the shared obj
 int shared_fd = shm_open("/users", O_RDWR, 0666);
 if(shared_fd == -1) {
	 fprintf(stderr, "Cannot create shared object: using_logcus. %s\n",
	 strerror(errno));
 }
 struct stat sb;
 fstat(shared_fd, &sb);
 unsigned *addr = mmap(0, sizeof(*addr), PROT_READ | PROT_WRITE, MAP_SHARED, shared_fd, 0);
 if(addr == MAP_FAILED) {
	 fprintf(stderr, "mmap failed: %s\n", strerror(errno));
 }
 return addr;
}


int close_logcus() {
	unsigned * addr = open_shared();
	*addr = *addr - 1;
	printf("Reamining processes process count: %d\n", *addr);
	return(EXIT_SUCCESS);
}


/* Init forks a child process. The parent returns and continues executing
 * its own code, if child was created succesfully. The child forks another child
 * (lapsenlapsi) and then gets killed, so the grandchild gets detached thus
 * making a new daemon process.
 */

int init_logcus() {

	// Check if logcus daemon already exists
	int ret = check_logcus();
	if(ret == 1) {
		unsigned *addr = open_shared();
		*addr = * addr + 1; // just mark that one more process uses logcus
		return EXIT_SUCCESS;
	}

	// Start initalization (no daemon exists).
	FILE *fp= NULL;
	pid_t process_id1 = 0;  // child - (will be killed)
	pid_t process_id2 = 0; // grandchild -> daemon
	pid_t sid = 0;


	//shared_add();
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

		char cwd[1024];
		if(getcwd(cwd, sizeof(cwd)) == NULL){
			return(EXIT_FAILURE); // cannot get the path
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

		// Create log either to project src dir or user defined path
		if(log_path == NULL) {
			char *full_path = concat(cwd, "/log.txt");
			fp = fopen(full_path, "w+");
			free(full_path);
		} else {
			fp = fopen(log_path, "w+");
		}


		if(fp == NULL) {
			return EXIT_FAILURE; // cannot create log
		}

		// Create shared process counter variable
		unsigned * addr = create_shared();
		*addr = 1;

		/* DAEMON INITALIZED START MAINLOOP */
		while (*addr > 0)	{
			//Dont block context switches, let the process sleep for some time
			sleep(1);
			int nbytes = read(fd, buffer, sizeof(buffer));
			if(nbytes > 0 && sizeof(buffer) > 0) {
				fprintf(fp, "%s", buffer);
			}
			fflush(fp);
		}
		// Shutting daemon process down. Do a clean exit.
		remove("/tmp/pid.txt"); // remove tmp file
		remove(myfifo);
		shm_unlink("/users"); // free shared memory
		close(fd); // close fifo
		fprintf(fp, "Daemon stopped with a clean exit\n");
		fclose(fp);
		return(EXIT_SUCCESS);
	}
	else {
		/*---PARENT--- */
		return (EXIT_SUCCESS); // continue executing the program
	}

	// Program shouldn't get here.
	return (EXIT_FAILURE);
}
