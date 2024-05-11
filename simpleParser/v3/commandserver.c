// Simple shell example using fork() and execlp().

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <limits.h>

#include "parser.h"

extern char ** parser(char buf[PIPE_BUF], char *argv[1000], char * delim);


void handle_interupt(int sig) {
  printf("\n");
  if (remove("serverfifo.dat") != 0) {
    printf("The FIFO could not be deleted. please manually delete it by: rm serverfifo.dat\n");
    exit(1);
  }
  exit(0);
}


int main(void) {
  pid_t k;
  int status;
  int len;

  // integer to keep track of the public fifo
  int public_fifo;
  int private_fifo;

  // Public FIFO file
  char * server_fifo = "./serverfifo.dat";

  // creating the named file(FIFO)
  // mkfifo(<pathname>, <permission>)
  mkfifo(server_fifo, 0666);

  // handle interrupt signal
  signal(SIGINT, handle_interupt);

  // try to open the public fifo outside and manage it as it will be the same
  // for  multiple clients

  // read in from the public fifo which will contain client's fifo name and
  // command, open the clients private fifo and then call dup2 to change the
  // output of execvp from stdout to clients fifo. After writing close the
  // clients fifo


  while(1) {

    // print prompt
    fprintf(stdout,"[%d]$ ",getpid());
    fflush(stdout);

    // first open in read only and read
    public_fifo = open(server_fifo, O_RDONLY);

    int j = 0;
    int flag = 0;
    char temp2[1];
    char buf[PIPE_BUF] = {'\0'};

    // read in character by charater till \n
    read(public_fifo, temp2, 1);
    while (flag != 2 && j < PIPE_BUF) {
      if (temp2[0] == '\n') {
        flag++;
      }
      buf[j] = temp2[0];
      read(public_fifo, temp2, 1);
      // printf("\nRead is: %c\n", temp[0]);
      j++;
    }
    // read in the \0
    read(public_fifo, temp2, 1);

    // read(public_fifo, buf, PIPE_BUF);

    close(public_fifo);

    // print the string and close
    printf("\nClient command:\n----------\n%s----------\n", buf);
    fflush(stdout);

    // parse the client pid
    int i = 0;
    char client_pid[PIPE_BUF];
    while(buf[i] != '\n') {
      client_pid[i] = buf[i];
      i++;
    }
    client_pid[i] = '\0';

    // retrieve the client command
    char * temp[2];
    char ** client_message = parser(buf, temp, "\n");
    char * command = client_message[1];

    // parse and call execvp
    // get the length
    // make an array of pointers
    len = strlen(command);
    if(len == 1) { // only return key pressed
      continue;
    }
    char *argv[1000];
    char ** argv1 = parser(command, argv, " ");

    // open the client fifo for write only
    char client_fifo[PIPE_BUF + 8] = "./client";
    strcat(client_fifo, client_pid);
    private_fifo = open(client_fifo, O_WRONLY);

    int stdout_copy = dup(STDOUT_FILENO);
    int private_fifo_copy = dup(private_fifo);

    k = fork();
    if (k==0) {
      // child code
      dup2(private_fifo, STDOUT_FILENO);
      if(execvp(argv1[0],argv1) == -1) {// if execution failed, terminate child
        dup2(stdout_copy, STDOUT_FILENO);
        close(stdout_copy);
        close(private_fifo);
        close(private_fifo_copy);
        exit(1);
      }
    } else {
      // parent code 
      waitpid(k, &status, 0);
      dup2(stdout_copy, STDOUT_FILENO);
      close(stdout_copy);
      close(private_fifo);
      close(private_fifo_copy);
      //printf("\n\nDONE WITH DUP2\n\n");
    }

  }
}
