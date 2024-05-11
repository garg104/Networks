// Simple shell example using fork() and execlp().

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <sys/time.h>
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
#include <signal.h>



struct request_format {
  char filename[8];
  unsigned char lsb;
  unsigned char msb;
} request;


struct packet_format {
  unsigned char packet_no;
  //int size;
  char data[];
};

int sockfd;
int windowsize;
int timeout;
int offset;
int blocksize;
int done;
int last;
int terminate = 0;

char **data;


// define the socket structs
struct sockaddr_in servaddr;
struct sockaddr_in cliaddr;
socklen_t client_struct_length = sizeof(cliaddr);

int toss();
int check_command(char *);
int check_ip(char *);
void handle_alarm(int );
void handle_interupt(int );
int recieve_request(int );
void perform_checks(int , int);


void handle_alarm(int sig) {
  // send the packets again
  printf("ALARM went off! Resending packets\n");
  alarm(0);
  struct itimerval timer;
  timer.it_interval.tv_sec = 0;
  timer.it_interval.tv_usec = 0;
  timer.it_value.tv_sec = 0;
  timer.it_value.tv_usec = timeout;
  signal(SIGALRM, handle_alarm);

  for (int i = 0; i < windowsize; i++) {
    struct packet_format *packet;
    int size = sizeof(unsigned char) + sizeof(char) * blocksize;
    packet = malloc(size);
    memcpy(packet->data, data[i], blocksize);
    packet->packet_no = (unsigned char) (i + offset) & 0xff;
    if (sendto(sockfd, packet, size, 0,
          (struct sockaddr*)&cliaddr, client_struct_length) < 0){
      printf("Can't send\n");
      exit(1);
    }

    free(packet);
    packet = NULL;

    if (done == 1 && last == i) {
      // last packet being recent
      break;
    }
  } // end-for

  setitimer(ITIMER_REAL, &timer, 0);
}


int main(int argc, char **argv) {
  if (argc != 7) {
    printf("Incorect Number of Arguments Passed! Please run the server again.\n");
    exit(1);
  } else {
    // print out the arguments
    for (int i = 0; i < argc; ++i) {
      printf("argv[%d]: %s\n", i, argv[i]);
    }
  }
  // convert the argv strings to ints
  int port = atoi(argv[2]);
  int secret_key = atoi(argv[3]);
  blocksize = atoi(argv[4]);
  windowsize = atoi(argv[5]);
  timeout = atoi(argv[6]);


  // Creating socket file descriptor
  if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) {
    perror("socket creation failed");
    exit(EXIT_FAILURE);
  }

  // clear the memory space
  memset(&servaddr, 0, sizeof(servaddr));
  memset(&cliaddr, 0, sizeof(cliaddr));

  // Filling server information
  servaddr.sin_family = AF_INET; // IPv4
  servaddr.sin_addr.s_addr = inet_addr(argv[1]);
  servaddr.sin_port = htons(port);

  // Bind the socket with the server address
  if (bind(sockfd, (const struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
    perror("bind failed");
    exit(EXIT_FAILURE);
  }

  // Receive client's message and perform the checks
  bzero(request.filename, 8);
  printf("\n>----------\n");
  int secret_rec = recieve_request(-1);
  perform_checks(secret_rec, secret_key);

  // open the requested file to transfer
  FILE *file = fopen(request.filename, "r");
  int fp = fileno(file);


  done = 0;
  offset = 0;
  signal(SIGALRM, handle_alarm);
  signal(SIGINT, handle_interupt);

  // send W packets wait for the ACk, resend if no ACK recieved
  while(1) {
    // set the timer
    struct itimerval timer;
    timer.it_interval.tv_sec = 0;
    timer.it_interval.tv_usec = 0;
    timer.it_value.tv_sec = 0;
    timer.it_value.tv_usec = timeout;

    // allocate space to keep track of the data globally
    data = malloc(sizeof(char) * windowsize * blocksize);

    // send the W packets each of blocksize
    for (int i = 0; i < windowsize; i++) {
      // read in the blocksize from the file
      struct packet_format *packet;
      int size = sizeof(unsigned char) + sizeof(char) * blocksize;
      packet = malloc(size);
      int bytes_read = read(fp, packet->data, blocksize);
      if (bytes_read < blocksize) {
        done = 1;
        last = i;
        packet->data[bytes_read] = '\0';
      }
      packet->packet_no = (unsigned char)(i + offset) & 0xff;

      data[i] = malloc(blocksize);
      memset(data[i], 0, blocksize);
      memcpy(data[i], packet->data, blocksize);

      //if (i != 4) {
      // send the packet to the client
      if (sendto(sockfd, packet, size, 0,
            (struct sockaddr*)&cliaddr, client_struct_length) < 0){
        printf("Can't send\n");
        return -1;
      }
      //}

      free(packet);
      packet = NULL;

      if (done == 1) {
        break;
      }

    } // end-for

    setitimer(ITIMER_REAL, &timer, 0);

    // wait for the ACK
    int last_packet_recieved;
    if (recvfrom(sockfd, &last_packet_recieved, sizeof(int), 0,
          (struct sockaddr*)&cliaddr, &client_struct_length) < 0){
      printf("Couldn't receive\n");
      return -1;
    }
    // remove the timer
    alarm(0);

    // handle wrapping
    if (last_packet_recieved == windowsize - 1) {
      offset = windowsize;
    } else {
      offset = 0;
    }

    // free the global data after the ACK and set it to null
    for(int i = 0; i < windowsize; i++) {
      if (done == 1 && i == last) {
        break;
      }
      free(data[i]);
    }
    free(data);
    data = NULL;

    // file transmission has completed
    if (done == 1) {
      printf("File transmission completed!\n");
      printf(">----------\n\n");
      break;
    }

  } // end-while

  close(sockfd);
  close(fp);

} // end-main



int recieve_request(int temp_secret) {
  int temp = 0;
  while (1) {
    if (recvfrom(sockfd, &request, sizeof(request), 0, (struct sockaddr*)&cliaddr, &client_struct_length) < 0){
      printf("Couldn't receive\n");
      return -1;
    }

    if (temp == 0) {
      // request recieved the first time. Has to be the client request
      temp_secret = (int)(((unsigned)request.msb << 8) | request.lsb );
      printf("Received request from IP: %s and port: %i\n",
          inet_ntoa(cliaddr.sin_addr), ntohs(cliaddr.sin_port));
    }
    int current = (int)(((unsigned)request.msb << 8) | request.lsb );

    if (temp != 0 && temp_secret != current) {
      // this is the ACK ACK packet
      // we assume that this packet never gets dropped
      // we can break out of the loop to send the file packets now
      break;
    }

    // send the ACK
    int ack = 1;
    if (sendto(sockfd, &ack, sizeof(int), 0,
          (struct sockaddr*)&cliaddr, client_struct_length) < 0){
      printf("Can't send\n");
      return -1;
    }

    temp++;
  }

  return temp_secret;
} // end-recieve_request


void perform_checks(int rec, int secret_key) {
  // perform checks
  char *ip = inet_ntoa(cliaddr.sin_addr);
  int check = check_ip(ip);
  if (check == 0) {
    //close the connection
    printf("Invalid Client IP address! Closing the server\n");
    close(sockfd);
    exit(1);
  }

  // check if the secret key matches
  printf("Client's key is: %d\n", rec);
  if (secret_key != rec) {
    //close the connection
    printf("Invalid Secret Key!\n");
    close(sockfd);
    exit(1);
  }

  // check the filename and if it exists
  check = 1;
  if (strlen(request.filename) <= 8 && strlen(request.filename) > 0) {
    for (int i = 0; i < strlen(request.filename); i++) {
      if ((request.filename[i] >= 'a' && request.filename[i] <= 'z') ||
          (request.filename[i] >= 'A' && request.filename[i] <= 'Z' )) {
        // the char is an alphabet
      } else {
        check = 0;
        break;
      }
    }
    if (check == 0) {
      printf("Invalid file name!\n");
      close(sockfd);
      exit(1);
    }
  } else {
    // file name length is not correct.
    // close the connection.
    printf("Invalid file name length!\n");
    close(sockfd);
    exit(1);
  }

  // check if the file exists in the currnent directory
  printf("File to be transfered is %s\n", request.filename);
  if (access(request.filename, F_OK) != 0) {
    printf("Fle does not exist\n");
    close(sockfd);
    exit(1);
  }

} // end-perform_checks





int toss() {
  srand(time(NULL));
  return (rand() % 2);
}


int check_command(char *command) {
  char *command1 = "date\n";
  char *command2 = "/bin/date\n";

  if (strlen(command) != strlen(command1)) {
    if (strlen(command) != strlen(command2)) {
      return 0;
    }
  }

  int flag = 1;
  for (int i = 0; i < strlen(command1); i++) {
    if (command[i] != command1[i]) {
      flag = 0;
    }
  }
  if (flag == 1) {
    return 1;
  }

  flag = 1;
  for (int i = 0; i < strlen(command2); i++) {
    if (command[i] != command2[i]) {
      flag = 0;
    }
  }

  return flag;

}




int check_ip(char *ip) {
  char * lab_ip1 = "128.10.25.";
  char * lab_ip2 = "128.10.112.";
  char * lab_ip3 = "128.10.";
  char * lab_ip4 = "10.192.29.83";

  int flag = 1;
  for (int i = 0; i < strlen(lab_ip1); i++) {
    if (lab_ip1[i] != ip[i]) {
      flag = 0;
    }
  }
  if (flag == 1) {
    return 1;
  }

  flag = 1;
  for (int i = 0; i < strlen(lab_ip2); i++) {
    if (lab_ip2[i] != ip[i]) {
      flag = 0;
    }
  }
  if (flag == 1) {
    return 1;
  }

  flag = 1;
  for (int i = 0; i < strlen(lab_ip3); i++) {
    if (lab_ip3[i] != ip[i]) {
      flag = 0;
    }
  }
  if (flag == 1) {
    return 1;
  }

  flag = 1;
  for (int i = 0; i < strlen(lab_ip4); i++) {
    if (lab_ip4[i] != ip[i]) {
      flag = 0;
    }
  }

  return flag;
}


void handle_interupt(int sig) {
  printf("\n");
  exit(0);
}

//>----------------------------------------------------------------------------

/*
   pid_t k;
   int status;
   int clisock = -1;
   while (1) {
   printf("\n>--------\n");
   fflush(stdout);

 * 1) accept a client connection
 * 2) toss a coin
 * 3) ignore or accept?
 * 4) to accept check the IP
 * 5) check if request is valid
 * 6) fork if secure
//

// accept the client connection
clisock = accept(sockfd, (struct sockaddr *)&cliaddr, &client_struct_length);
// printf("after accept\n");
if (clisock < 0) {
printf("server accept failed...\n");
exit(1);
}

// read the command from client and copy it in buffer
char *ip = inet_ntoa(cliaddr.sin_addr);
bzero(request.filename, 8);
if (read(clisock, &request, sizeof(request)) < 0) {
perror("Cannot read command from client");
}

// toss a choin
int call = toss();
// printf("Call is always tails for testing\n");
// call = 0;
if (call == 1) {
// toss is heads, ignore the request
printf("Client %s's request was ignored\n", ip);
close(clisock);
continue;
} else {
printf("Client %s's request was not ignored\n", ip);
}

// toss is tails, check the IP address
int check = check_ip(ip);
if (check == 0) {
//close the connection
printf("Invalid Client IP address!\n");
close(clisock);
continue;
}

// check if the secret key matches
int rec = (int)(((unsigned)request.msb << 8) | request.lsb );
printf("Client's key is: %d\n", rec);
if (secret_key != rec) {
//close the connection
printf("Invalid Secret Key!\n");
close(clisock);
continue;
}

// check the filename and if it exists
check = 1;
if (strlen(request.filename) <= 8 && strlen(request.filename) > 0) {
for (int i = 0; i < strlen(request.filename); i++) {
if ((request.filename[i] >= 'a' && request.filename[i] <= 'z') ||
(request.filename[i] >= 'A' && request.filename[i] <= 'Z' )) {
// the char is an alphabet
} else {
check = 0;
break;
}
}
if (check == 0) {
  printf("Invalid file name!\n");
  close(clisock);
  continue;
}
} else {
  // file name length is not correct.
  // close the connection.
  printf("Invalid file name length!\n");
  close(clisock);
  continue;
}

// check if the file exists in the currnent directory
printf("The filename is %s\n", request.filename);
if (access(request.filename, F_OK) != 0) {
  printf("Fle does not exist\n");
  close(clisock);
  continue;
}
FILE *file = fopen(request.filename, "r");
int fp = fileno(file);

FILE *file2 = fopen("copyFile", "w");
int fp2 = fileno(file2);

k = fork();
if (k==0) {
  // child node
  // read the file and write it to the socket
  char *buf= malloc(blocksize);
  //char *buf;//= malloc(blocksize);
  bzero(buf, blocksize);
  //for (int i = 0; i < blocksize; i++) {
  //  buf[i] = '\0';
  //}
  while(read(fp, buf, blocksize) > 0) {
    //buf[blocksize - 1] = '\0';
    write(clisock, buf, blocksize);
    //fwrite(buf, 1, strlen(blocksize), file2);
    //bzero(buf, blocksize);
    for (int i = 0; i < blocksize; i++) {
      buf[i] = '\0';
    }
  }
  free(buf);
  char eof[blocksize];
  for (int i = 0; i < blocksize; i++) {
    eof[i] = '\0';
  }
  write(clisock, eof, blocksize);
  // close the connection and end the child process
  close(clisock);
  close(fp);
  exit(0);
} else {
  // parent node
  waitpid(k, &status, 0);
}

} // end-while
*/






