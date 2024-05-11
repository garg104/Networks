// Simple parser to parse the inpiut and return the pointer array

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <string.h>

#include "parser.h"

char ** parser(char buf[100], char *argv[1000]) {
  // get the length
  // int len = strlen(buf);

  // make an array of pointers
  // char * argv[1000];
  int command_counter = 0;

  // split the input
  argv[0] = strtok(buf, " ");

  // read the commmands one by one
  char *arg;
  while ((arg = strtok(NULL, " "))) {
    command_counter++;
    argv[command_counter] = arg;
  }
  command_counter++;
  argv[command_counter] = (char *) NULL;

  return argv;
}

