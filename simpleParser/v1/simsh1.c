// Simple shell example using fork() and execlp().

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <string.h>

#include "parser.h"

extern char ** parser(char buf[100], char *argv[1000]);

/*
char ** parser(char buf[100], char *argv[1000]) {
  // get the length
  int len = strlen(buf);

  // make an array of pointers
  // char * argv[1000];
  int command_counter = 0;

  // split the input
  argv[0] = strtok(buf, " ");

  // read the commmands one by one
  char *arg;
  while (arg = strtok(NULL, " ")) {
    command_counter++;
    argv[command_counter] = arg;
  }
  command_counter++;
  argv[command_counter] = (char *) NULL;

  return argv;
}
*/


int main(void) {
  pid_t k;
  char buf[100];
  int status;
  int len;

  while(1) {

    // print prompt
    fprintf(stdout,"[%d]$ ",getpid());

    // read command from stdin
    fgets(buf, 100, stdin);

    // get the length
    len = strlen(buf);

    // make an array of pointers
    char *argv[1000];
    char ** argv1 = parser(buf, argv);

    if(len == 1) 				// only return key pressed
      continue;
    buf[len-1] = '\0';

    k = fork();
    if (k==0) {
      // child code
      if(execvp(argv1[0],argv1) == -1)	// if execution failed, terminate child
        exit(1);
    } else {
      // parent code 
      waitpid(k, &status, 0);
    }
  }
}
