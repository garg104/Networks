#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/file.h>
#include <signal.h>

#define MAXLINE 1024


struct timeval *ptr = NULL;
struct sockaddr_in servaddr;
struct sockaddr_in cliaddr;
socklen_t server_struct_length = sizeof(servaddr);
int client_struct_length = sizeof(cliaddr);
int sockfd;
int mid_value;
int n;

void sig_handler(int signum) {
  if (signum == SIGALRM) {
    // the alarm went off
    // termiante the client process
    printf("Alarm went off, terminating the client with exit(1)!\n");
    exit(1);
  }

  if (signum == SIGIO) {
    // printf("recieved sigio\n");
    int server_message[2];
    if(recvfrom(sockfd, server_message, sizeof(server_message), 0,
          (struct sockaddr*)&servaddr, &server_struct_length) < 0){
      printf("Error while receiving server's msg\n");
      return;
    }

    struct timeval end_time;
    gettimeofday(&end_time, NULL);

    int s = server_message[0];
    int i = s - mid_value;
    long double rtt = ((end_time.tv_sec * 1000000 + end_time.tv_usec) - (ptr[i].tv_sec * 1000000 + ptr[i].tv_usec));
    rtt = rtt/1000;
    if (s == mid_value + n - 1) {
      // remove alarm as we recieved the last packet and exit
      alarm(0);
      // printf("removing alarm\n");
      printf("RTT for packet %d is:  %0.3Lfms\n", s, rtt);
      fflush(stdout);
      exit(0);
    }
    printf("RTT for packet %d is:  %0.3Lfms\n", s, rtt);
    fflush(stdout);
  }

}



// Driver code
int main(int argc, char **argv) {
  if (argc != 4) {
    printf("Incorect Number of Arguments Passed! Please run the client again.\n");
    fflush(stdout);
    exit(1);
  } else {
    // print out the arguments
    for (int i = 0; i < argc; ++i) {
      printf("argv[%d]: %s\n", i, argv[i]);
    }
  }

  // convert the string to int port number
  int port = atoi(argv[3]);

  // Creating socket file descriptor
  if ((sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0 ) {
    perror("socket creation failed");
    exit(EXIT_FAILURE);
  }

  memset(&servaddr, 0, sizeof(servaddr));
  memset(&cliaddr, 0, sizeof(cliaddr));

  // Filling server information
  servaddr.sin_family = AF_INET; // IPv4
  if (inet_aton(argv[2], &servaddr.sin_addr) == 0) {
    fprintf(stderr, "Invalid address\n");
    exit(EXIT_FAILURE);
  }
  servaddr.sin_port = htons(port);
  servaddr.sin_addr.s_addr = inet_addr(argv[2]);

  // filling client information
  cliaddr.sin_family = AF_INET; // IPv4
  if (inet_aton(argv[1], &cliaddr.sin_addr) == 0) {
    fprintf(stderr, "Invalid address\n");
    exit(EXIT_FAILURE);
  }
  cliaddr.sin_port = htons(0);
  cliaddr.sin_addr.s_addr = inet_addr(argv[1]);

  // Bind the socket with the client address
  if (bind(sockfd, (struct sockaddr *)&cliaddr, sizeof(cliaddr)) < 0) {
    perror("bind failed");
    exit(EXIT_FAILURE);
  }


  // read in the ping params from the file pingparam.dat
  // open the file
  FILE *fp = fopen("./pingparam.dat", "r");
  if (fp == NULL) {
    fprintf(stderr, "File pingparam.dat doesn't exist. Please make a valid file and then run the client again.\n");
    exit(EXIT_FAILURE);
  }

  // read in the line
  char c[1000];
  fscanf(fp, "%[^\n]", c);

  // assuming that that the data is correct
  int N;
  int T;
  int D;
  int S;
  int k = 0;
  for (int i = 0; i < strlen(c); i++) {
    char temp_int[1000] = {'\0'};
    int j = 0;
    while (1) {
      char temp = c[i];
      if (temp <= 57 && temp >= 48) {
        temp_int[j] = temp;
        j++;
      } else if (temp == ',' || i >= strlen(c)) {
        temp_int[j] = '\0';
        k++;
        break;
      } else if (temp != ' ') {
        fprintf(stderr, "File pingparam.dat format is incorrect.\n");
        exit(EXIT_FAILURE);
      }
      i++;
    }
    if (k == 1) {
      N = atoi(temp_int);
    } else if (k == 2) {
      T = atoi(temp_int);
    } else if (k == 3) {
      D = atoi(temp_int);
    } else if (k == 4) {
      S = atoi(temp_int);
    }
  }

  printf("ping params are: %d,%d,%d,%d\n", N, T, D, S);

  if (!(N >= 1 && N <= 7)) {
    fprintf(stderr, "Value of N is incorrect. Value should be between 1 and 7.\n");
    exit(EXIT_FAILURE);
  }
  if (N > 1 && !(T >= 1 && T <= 5)) {
    fprintf(stderr, "Value of T is incorrect. Value should be between 1 and 5.\n");
    exit(EXIT_FAILURE);
  }

  //if (fcntl(sockfd, O_NONBLOCK)) {
  //  perror("fcntl failed");
  //}
  fcntl(sockfd, F_SETOWN, getpid());
  fcntl(sockfd, F_SETFL, O_NONBLOCK | FASYNC);
  signal(SIGALRM, sig_handler);
  signal(SIGIO, sig_handler);

  struct timeval start_times[N];
  ptr = start_times;
  //char client_message[2] = {'h','i'};
  //char *message = htonl(client_message);

  // send the packets now synchrnously and handle recvfrom asynchronously
  // infite loop to keep the process running, ends when the alarm goes off or the last packet is recieved
  int flag = 1;
  mid_value = S;
  n = N;
  fd_set set;
  FD_ZERO (&set);
  printf("Sending packets now >---------\n\n");
  while (1) { 
    for (int i = 0; i < N && flag; i++) {
      // sleep for T seconds
      if (i > 0) {
        //printf("sleeping for %d seconds\n", T);
        //select(0, NULL, NULL, NULL, &timeout);
        //sleep(T);

        struct timeval timeout;
        gettimeofday(&timeout, NULL);
        //printf("before adding T: %ld\n", timeout.tv_sec);
        timeout.tv_sec = timeout.tv_sec + T;
        //printf("after adding T: %ld\n", timeout.tv_sec);
        struct timeval current;
        gettimeofday(&current, NULL);

        while (timeout.tv_sec > current.tv_sec) {
          gettimeofday(&current, NULL);
        }
        // nanosleep((const struct timespec[]){{3, 0L}}, NULL);
        // printf("sleeping1\n");
      }

      // set the alarm for the last packet
      if (N > 1 && i == N - 1) {
        printf("  setting alarm for last packet\n");
        alarm(10);
        flag = 0;
      }
      if (N == 1) {
        flag = 0;
      }

      // fill in the start times
      gettimeofday(&start_times[i], NULL);


      // make the packet
      int temp[2];
      temp[0] = S;
      temp[1] = D;


      // send the packets
      // printf("Sending Packet: %d\n", S);
      if(sendto(sockfd, temp, sizeof(temp), 0, (struct sockaddr*)&servaddr,
            server_struct_length) < 0){
        printf("Unable to send message\n");
        return -1;
      }

      // increament the MID value by one
      S++;
    }
  }

  close(sockfd);

  return 0;
}

/*
   int pid = fork();
   if (pid == 0) {
// child code
// send the packets
printf("here %d\n", i);


// Receive the server's response:
if(recvfrom(sockfd, server_message, sizeof(server_message), 0,
(struct sockaddr*)&servaddr, &server_struct_length) < 0){
printf("Error while receiving server's msg\n");
return -1;
}

// end child process;
break;
}
if(sendto(sockfd, client_message, strlen(client_message), 0, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0){
perror("send failed");
printf("Unable to send message\n");
return -1;
}



 */
