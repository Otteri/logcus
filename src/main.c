#include <stdio.h>
#include <stdlib.h>
#include <unistd.h> //sleep
#include <string.h>
#include "logcus.h" //currently linked in makefile


/*    Rough program structure:
 * ------------------------------------------------
 *
 */

int main(int argc, char *argv[]) {
  (void) argc;
  (void) argv;
  //char string[] = "Hello World!\n";

  printf("Hello!\n");
  init_logcus();

  while(1) {
    printf("main is running...\n");
    logcus("string sent from another program\n");
    //write(input, string, (strlen(string)+1));
    sleep(2);
  }
  return 0;
}
