#include <signal.h>
#include "logcus.h"

void sigint_handler(int sig) {
	logcus("Caught signal: %d. Killing process\n", sig);
	close_logcus();
	exit(0);
}

int main(int argc, char *argv[]) {
	(void) argc;
	(void) argv;
	signal(SIGINT, sigint_handler);
	if (open_logcus() < 0) {
		perror("daemon");
		exit(2);
	}
	while(1) {
    	printf("main is running... [press ^C to stop]\n");
    	logcus("string sent from %s program\n", "main.c");
    	sleep(1);
  	}
  	close_logcus();
  	return 0;
}
