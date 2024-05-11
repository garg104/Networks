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

char * private;

void handle_interupt(int sig) {
  printf("\n");
  if (remove(private) != 0) {
    printf("The FIFO could not be deleted. please manually delete it by: rm client*\n");
    exit(1);
  }
  exit(0);
}



int main(void) {

  // get the pid for the client and make a unique private fifo for the client.
  // After that get the command from the user and wparse the command and the
  // fifo name as mentioned in the handout and then wrie this on the public
  // fifo. close the public fifo and then open the private fifo for reading.
  // Read in the ouput from the private fifo and print it to the stdout.

  int public_fifo;
  int private_fifo;
  int pid = getpid();

  // public fifo file
  char * server_fifo = "./serverfifo.dat";

  // private fifo file
  char client_buf[100];
  sprintf(client_buf, "./client%d", pid);
  char * client_fifo = client_buf;
  printf("%s\n", client_fifo);
  private = client_fifo;

  // make the private client fifo
  mkfifo(client_fifo, 0666);

  // handle interrupt signal
  signal(SIGINT, handle_interupt);

  char buf[PIPE_BUF];
  char command[PIPE_BUF];
  while(1) {
    // print prompt
    fprintf(stdout,"\n[%d]$ ",getpid());
    fflush(stdout);

    // Take an input from the user
    // 100 is the maximum length
    printf("> ");
    fgets(buf, PIPE_BUF, stdin);

    // make the command by parsing the pid as mentioned in the handout
    sprintf(command, "%d\n", pid);
    strcat(command, buf);
    // printf("%s", command);


    // open public fifo in write only
    public_fifo = open(server_fifo,O_WRONLY);
    if (public_fifo < 0) {
      fprintf(stderr, "Error: Pipe not found\n");
      exit(1);
    }

    // Write the input string on FIFO
    // and close it
    // ignore the request if the request is greater that PIPE_BUF
    int len = strlen(buf);
    if (len >= PIPE_BUF) {
      printf("Request Rejected: Request size greater than PIPE_BUF\n");
    } else {
      write(public_fifo, command, strlen(command) + 1);
      close(public_fifo);
    }


    // open client fifo for read only and read
    // print the string and close
    char output[PIPE_BUF] = {'\0'};
    private_fifo = open(client_fifo, O_RDONLY);
    read(private_fifo, output, PIPE_BUF);
    printf("%s", output);
    fflush(stdout);

    close(private_fifo);
  }
}
