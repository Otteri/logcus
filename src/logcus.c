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
char buffer[100]; //this to config?
//static int *using_logcus = 0; //+1 init <and -1 close. if 0 do not run daemon.
int shared_fd;
unsigned* addr; //tells how many processes are using daemon

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

 	pid_file = fopen ("/tmp/pid.txt", "r");
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
	printf("ERROR! daemon is not currently running\n");
	return -1;
}


unsigned * open_shared() {
 // Create new obj
 int shared_fd = shm_open("/users", O_RDWR, 0666);
 if(shared_fd == -1) {
	 fprintf(stderr, "Cannot create shared object: using_logcus. %s\n",
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
}


int close_logcus() {
	is_running = 0;
	printf("closing logcus!\n");
	unsigned * addr = open_shared();
	printf("addr in close: %d\n", *addr);
	*addr = *addr - 1;
	printf("Processes using logcus: %d\n", *addr);
	fd = open(myfifo, O_WRONLY);
	write(fd, "terminate", 10);
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






		// CREATING ADDR
		//TODO: Function for setting follofing objects???

		// Create new obj
		shared_fd = shm_open("/users", O_RDWR | O_CREAT, 0666);
		if(shared_fd == -1){
			fprintf(fp, "Cannot create shared object: using_logcus. %s\n",
			strerror(errno));
			return(EXIT_FAILURE);
		}
		// Set size of memory obj
		if(ftruncate(shared_fd, sizeof(*addr)) == -1){
			fprintf(fp, "ftruncate: %s\n", strerror(errno));
			return EXIT_FAILURE;
		}
		// Map the memory obj
		addr = mmap(0, sizeof(*addr), PROT_READ | PROT_WRITE, MAP_SHARED, shared_fd, 0);
		if(addr == MAP_FAILED) {
			fprintf(fp, "mmap failed: %s\n", strerror(errno));
			return EXIT_FAILURE;
		}
		//shm_unlink("/users");

		*addr = 1;
		fprintf(fp, "addr: %d\n", *addr);


  	//unsigned * addr = open_shared();








		/*
		using_logcus = mmap(NULL, sizeof(*using_logcus), PROT_READ | PROT_WRITE, MAP_SHARED, -1, 0);
		*using_logcus = 1;
		*/

		/* DAEMON INITALIZED START MAINLOOP */
		while (*addr > 0)	{
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
		remove("/tmp/pid.tmp"); // remove tmp file
		//shm_unlink("/users"); // free shared memory
		close(fd); // close fifo
		return(EXIT_SUCCESS);
	}
	else {
		/*---PARENT--- */
		return (EXIT_SUCCESS); // continue executing the program
	}

	// Program shouldn't get here.
	return (EXIT_FAILURE);
}
