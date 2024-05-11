// Simple parser to parse the inpiut and return the pointer array

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <string.h>
#include <limits.h>

#include "parser.h"

char ** parser(char buf[PIPE_BUF], char *argv[1000], char * delim) {
  // get the length
  // int len = strlen(buf);

  // make an array of pointers
  // char * argv[1000];
  int command_counter = 0;

  // split the input
  argv[0] = strtok(buf, delim);

  // read the commmands one by one
  char *arg;
  while ((arg = strtok(NULL, delim))) {
    command_counter++;
    argv[command_counter] = arg;
  }
  command_counter++;
  argv[command_counter] = (char *) NULL;

  return argv;
}

