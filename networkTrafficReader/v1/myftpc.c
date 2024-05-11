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
#include <sys/time.h>


struct request_format {
  char filename[8];
  unsigned char lsb;
  unsigned char msb;
} request;


char * private;

void create_request(int , char *);

int main(int argc, char **argv) {
  if (argc != 6) {
    printf("Incorect Number of Arguments Passed! Please run the client again.\n");
    exit(1);
  } else {
    // print out the arguments
    for (int i = 0; i < argc; ++i) {
      printf("argv[%d]: %s\n", i, argv[i]);
    }
  }

  char *filename = argv[3];
  if (strlen(filename) > 7 || strlen(filename) <= 0) {
    // exit the client
    printf("The filename doesn't match the length requirements!\n");
    exit(0);
  }

  // convert the string argv to ints
  int port = atoi(argv[2]);
  int secret_key = atoi(argv[4]);
  int blocksize = atoi(argv[5]);

  // Creating socket file descriptor
  int sockfd;
  struct sockaddr_in servaddr;
  struct sockaddr_in cliaddr;
  socklen_t server_struct_length = sizeof(cliaddr);
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

  /**
  * arm the handlers
  * fcntl(sockfd, F_SETOWN, getpid());
  * fcntl(sockfd, F_SETFL, O_NONBLOCK | FASYNC);
  *
  * signal(SIGINT, handle_interupt);
  * signal(SIGALRM, handle_alarm);
  * signal(SIGIO, handle_sigio);
  **/

  // create the 10 Byte request
  create_request(secret_key, filename);

  // start communication
  printf("\n>----------\n");
  fflush(stdout);

  // get time stamp before sending the request
  struct timeval start;
  gettimeofday(&start, NULL);

  // send the request to the server
  if (write(sockfd, &request, sizeof(request)) < 0) {
    printf("Cannot write to server\n");
    perror("write");
  }

  // create a new file to store the copy from the server
  char newfilename[12];
  sprintf(newfilename, "NEW_%s", request.filename);
  FILE *fp = fopen(newfilename, "w");
  int fd = fileno(fp);

  //char buf[blocksize];
  char *buf = malloc(blocksize);
  while(read(sockfd, buf, blocksize) > 0) {
    int flag = 1;
    for (int i = 0; i < blocksize; i++) {
      if (buf[i] != '\0') {
        flag = 0;
        break;
      }
    }
    if (flag == 1) {
      break;
    }
    //printf("%c-%c\n", buf[0], buf[blocksize - 1]);
    for (int i = 0; i < blocksize; i++) {
      if (buf[i] == '\0') {
        break;
      }
      fputc(buf[i], fp); 
    }
    char *temp = NULL;
    temp = buf;
    //fwrite(temp, 1, blocksize, fp);
    //fflush(fp);
    //write(fd, buf, strlen(buf));
    //for(int i = 0; i < blocksize; i++) {
    //  buf[i] = '\0';
    //}

    bzero(buf, blocksize);
  }
  free(buf);

  fseek(fp, 0L, SEEK_END);
  long long int sz = ftell(fp);

  // close connections
  fclose(fp);
  close(sockfd);

  // get time after completion
  struct timeval end;
  gettimeofday(&end, NULL);

  if (sz == 0) {
    printf("Request was not completed! Please try again.\n");
    printf(">----------\n\n");
    fflush(stdout);
    remove(newfilename);
    exit(0);
  }

  printf("Name of the file transfered is: %s\n", filename);
  printf("File transfered is stored as: %s\n", newfilename);
  printf("The size of the file transfered is: %lld\n", sz);

  long double completion_time = ((end.tv_sec * 1000000 + end.tv_usec) - (start.tv_sec * 1000000 + start.tv_usec));
  completion_time = completion_time/1000;
  printf("Completion time is:  %0.3Lfms\n", completion_time);
  fflush(stdout);

  long double Bpms = (sz * 1000) / (completion_time *1024);
  long double bps = (sz * 8) / (completion_time / 1000);


  printf("Throughput is: %0.3Lf MBps or %0.3Lf bps\n", Bpms, bps);
  printf(">----------\n\n");
  fflush(stdout);



} // end-main

void create_request(int secret_key, char *filename) {
  int foo = secret_key;
  unsigned char lsb = (unsigned)foo & 0xff; // mask the lower 8 bits
  unsigned char msb = (unsigned)foo >> 8;   // shift the higher 8 bits
  request.lsb = lsb;
  request.msb = msb;
  int j = 0;
  for (int i = 0; i < strlen(filename); i++, j++) {
    request.filename[i] = filename[i];
  }
  request.filename[j] = '\0';

} // end-create_request



//---------------

/*

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
char output[blocksize];
if(read(sockfd, output, sizeof(output)) < 0){
printf("Error while receiving server's msg\n");
return;
} else {
printf("Command output is: %s\n", output);
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

*/




