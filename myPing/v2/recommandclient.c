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
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <ctype.h>
#include <time.h>

int sockfd;
char * private;
int count = 0;
char buf1[PIPE_BUF];
struct sockaddr_in servaddr;
struct sockaddr_in cliaddr;
socklen_t server_struct_length = sizeof(cliaddr);

void handle_interupt(int sig) {
  printf("\n");
  char *exitc = "exitexitexit1234";
  if (write(sockfd, exitc, 16) < 0) {
    printf("Cannot write to server, please terminate the server and start again!\n");
    perror("write");
  }
  close(sockfd);
  exit(0);
}


void handle_sigio(int sig) {
  // cancel the alarm
  // printf("Canceling the alarm\n");
  count = 0;
  alarm(0);
  char output[PIPE_BUF];
  if(read(sockfd, output, sizeof(output)) < 0){
    printf("Error while receiving server's msg\n");
    return;
  } else {
    printf("Command output is: %s", output);
  }
}




void handle_alarm(int sig) {
  // the alarm went off
  printf("Alarm %d went off!\n", count);

  // close the connection
  char *exitc = "exitexitexit1234";
  if (write(sockfd, exitc, 16) < 0) {
    printf("Cannot write to server\n");
    perror("write");
  }
  close(sockfd);

  if (count < 3) {
    count++;
    printf("Retry %d:---------\n", count);
    // make a new connection
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0 ) {
      perror("socket creation failed");
      exit(EXIT_FAILURE);
    }

    if (connect(sockfd, (struct sockaddr*)&servaddr, server_struct_length) != 0) {
      printf("connection with the server failed...\n");
      exit(0);
    } else {
      printf("  Connected to the server..\n");
    }


    fcntl(sockfd, F_SETOWN, getpid());
    fcntl(sockfd, F_SETFL, O_NONBLOCK | FASYNC);

    signal(SIGINT, handle_interupt);
    signal(SIGALRM, handle_alarm);
    signal(SIGIO, handle_sigio);

    // try it again
    // printf("Trying it again %s\n", buf1);
    alarm(2);
    if (write(sockfd, buf1, strlen(buf1)) < 0) {
      printf("Cannot write to server\n");
      perror("write");
    }

    //read(sockfd, buf, sizeof(buf));

  } else {
    printf("Client could not get response. Client is being terminated\n");
    fflush(stdout);
    exit(0);
  }
}

int main(int argc, char **argv) {
  if (argc != 3) {
    printf("Incorect Number of Arguments Passed! Please run the client again.\n");
    exit(1);
  } else {
    // print out the arguments
    for (int i = 0; i < argc; ++i) {
      printf("argv[%d]: %s\n", i, argv[i]);
    }
  }

  // convert the string to int port number
  int port = atoi(argv[2]);

  // Creating socket file descriptor
  if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0 ) {
    perror("socket creation failed");
    exit(EXIT_FAILURE);
  }

  memset(&servaddr, 0, sizeof(servaddr));
  memset(&cliaddr, 0, sizeof(cliaddr));

  // Filling server information
  servaddr.sin_family = AF_INET; // IPv4
  servaddr.sin_addr.s_addr = inet_addr(argv[1]);
  servaddr.sin_port = htons(port);


  // Filling client information
  cliaddr.sin_family = AF_INET; // IPv4
  cliaddr.sin_addr.s_addr = htonl(INADDR_ANY);
  cliaddr.sin_port = htons(0);

  // connect the client socket to server socket
  if (connect(sockfd, (struct sockaddr*)&servaddr, server_struct_length) != 0) {
    printf("connection with the server failed...\n");
    exit(0);
  } else {
    printf("connected to the server..\n");
  }

  fcntl(sockfd, F_SETOWN, getpid());
  fcntl(sockfd, F_SETFL, O_NONBLOCK | FASYNC);

  signal(SIGINT, handle_interupt);
  signal(SIGALRM, handle_alarm);
  signal(SIGIO, handle_sigio);

  printf("\n>----------\n ");
  while (1) {
    // Take an input from the user
    char buf[PIPE_BUF];
    bzero(buf, sizeof(buf));
    //printf(">----------\n ");
    fflush(stdout);
    fgets(buf, PIPE_BUF, stdin);
    strcpy(buf1, buf);
    alarm(2);
    printf("Command is %s", buf);
    if (write(sockfd, buf, sizeof(buf)) < 0) {
      printf("Cannot write to server\n");
      perror("write");
    }

    //read(sockfd, buf, sizeof(buf));
    //printf("Command output is %s", buf);

  } // end while

}

/*
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
 */
