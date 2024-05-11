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
  int fd1;

  // FIFO file
  char * myfifo = "./serverfifo.dat";

  // creating the named file(FIFO)
  mkfifo(myfifo, 0666);

  // handle interrupt signal
  signal(SIGINT, handle_interupt);

  while(1) {

    // print prompt
    fprintf(stdout,"[%d]$ ",getpid());
    fflush(stdout);

    // first open in read only and read
    fd1 = open(myfifo, O_RDONLY);

    int i = 0;
    char temp[1];
    // read in character by charater till \n
    // printf("\nbefore while\n");
    // fflush(stdout);
    read(fd1, temp, 1);
    // printf("\nFirst Read: %c\n", temp[0]);
    char buf[PIPE_BUF] = {'\0'};
    while (temp[0] != '\n' && i < PIPE_BUF) {
      buf[i] = temp[0];
      read(fd1, temp, 1);
      // printf("\nRead is: %c\n", temp[0]);
      i++;
    }
    // read in the \0
    read(fd1, temp, 1);
    buf[i] = '\n';
    // printf("\nAfter while\n");
    // fflush(stdout);
    // read(fd1, buf, PIPE_BUF);

    // print the string and close
    printf("Client command: %s", buf);
    fflush(stdout);
    close(fd1);

    // parse and call execvp
    // get the length
    len = strlen(buf);

    // make an array of pointers
    char *argv[1000];
    char ** argv1 = parser(buf, argv);

    if(len == 1) { // only return key pressed
      continue;
    }
    buf[len-1] = '\0';
    // printf("\nClient command 2: %s", buf);
    // fflush(stdout);

    k = fork();
    if (k==0) {
      // child code
      if(execvp(argv1[0],argv1) == -1)	// if execution failed, terminate child
        exit(1);
    } else {
      // parent code 
      waitpid(k, &status, 0);
    }



    // Now open in write mode and write
    // string taken from user.
    /*
       FILE *p = popen(buf,"r");
       if (p != NULL ) {
       int i = -1;
       while (!feof(p) && (i < 999)) {
       i++;
       fread(&str1[i],1,1,p);
       }
       i--;
       str1[i] = '\0';
       printf("%s\n", str1);
       int valid = pclose(p);
       if (valid != 0) {
       strcpy(str1, "Invalid Command!");
       }
       }
     */

    /*fd1 = open(myfifo,O_WRONLY);
      write(fd1, "", 1);
      close(fd1);
     */

  }
}
