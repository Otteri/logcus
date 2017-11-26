#include <stdio.h>
#include <stdlib.h>
#include <unistd.h> //sleep
#include "logcus.h" //currently linked in makefile


/*    Rough program structure:
 * ------------------------------------------------
 *
 */

int main(int argc, char *argv[]) {
  (void) argc;
  (void) argv;

  printf("Hello!\n");
  init_logcus();
  while(1) {
    printf("main is running...\n");
    sleep(1);
  }
  return 0;
}
