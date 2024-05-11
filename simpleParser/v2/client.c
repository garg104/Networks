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

extern char ** parser(char buf[100], char *argv[1000]);


int main(void) {
  int fd1;

  // FIFO file
  char * myfifo = "./serverfifo.dat";

  // creating the named file(FIFO)
  // mkfifo(<pathname>, <permission>)
  // mkfifo(myfifo, 0666);
  char buf[100];


  while(1) {
    // first open in write only
    fd1 = open(myfifo,O_WRONLY);
    if (fd1 < 0) {
      fprintf(stderr, "Error: Pipe not found\n");
      exit(1);
    }

    // print prompt
    fprintf(stdout,"[%d]$ ",getpid());

    // Take an input fro m the user
    // 100 is the maximum length
    printf("> ");
    fgets(buf, 100, stdin);

    // Write the input string on FIFO
    // and close it
    // ignore the request if the request is greater that PIPE_BUF
    int len = strlen(buf);
    if (len >= PIPE_BUF) {
      printf("Request Rejected: Request size greater than PIPE_BUF\n");
    } else {
      write(fd1, buf, strlen(buf)+1);
      close(fd1);
    }


    // open fifo for read only and read
    //fd1 = open(myfifo, O_RDONLY);
    // read(fd1, str2, 1000);


    // print the string and close
    // printf("%s\n", str2);
    // close(fd1);

  }
}
