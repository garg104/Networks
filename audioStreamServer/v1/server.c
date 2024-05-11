// Server side implementation
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/wait.h>
#include <fcntl.h>
#include "endian_convertor.h"
#include <sys/stat.h>
#include <signal.h>
#include <sys/time.h>
#include <time.h>
#include <pthread.h>
#include <semaphore.h>
#include <string.h>


#define MAX 4096
#define SA struct sockaddr


struct log {
  long long lambda;
  struct timeval time;
  struct log *next;
};

struct log *head = NULL;
struct log *current = NULL;
long long int link_length;

long long int client_num;


struct sockaddr_in cliaddr;
struct sockaddr_in servaddr;
struct timespec time_rem;
int sockfd;
int socknew;
char *port, *ip, *logfile;
char filename[9];
float lambda;
long long slptime;
int file_read_done = 0;
unsigned short blocksize_num;
volatile int retransmit=0;

off_t fsize(const char *filename)
{
  struct stat st;

  if (stat(filename, &st) == 0)
    return st.st_size;

  return -1;
}

void uinttochar(uint8_t *x, char *y)
{
  for (int i = 0; i < 8; i++)
  {
    y[i] = (char)x[i + 2];
  }
  y[8] = '\0';
}

int check_ip(uint32_t s_addr)
{
  if (((s_addr & 0xFF) == 128) && (((s_addr >> 8) & 0xFF) == 10) && ((((s_addr >> 16) & 0xFF) == 25) || (((s_addr >> 16) & 0xFF) == 112)))
    return 1;
  return 0;
}

void socket_connection()
{
  sockfd = socket(AF_INET, SOCK_DGRAM, 0);
  if (sockfd == -1)
  {
    printf("socket creation failed...\n");
    exit(0);
  }
  else {
    printf("Socket successfully created..\n");
  }

  memset(&servaddr, 0, sizeof(servaddr));
  // Filling server information
  servaddr.sin_family = AF_INET; // IPv4
  servaddr.sin_port = htons((unsigned int)atoi(port));
  servaddr.sin_addr.s_addr = inet_addr(ip);

  // Binding newly created socket to given IP and verification
  if ((bind(sockfd, (SA *)&servaddr, sizeof(servaddr))) != 0) {
    printf("socket bind failed...\n");
    exit(0);
  } else {
    printf("Socket successfully binded..\n");
  }

}

int receivefilename() {
  uint8_t receive_buff[10];
  unsigned int len = sizeof(cliaddr);
  bzero(receive_buff, 10);
  int n = recvfrom(sockfd, (char *)receive_buff, 10, 0, ( struct sockaddr *) &cliaddr,
      &len);
  blocksize_num = littletoshort(receive_buff);
  uinttochar(receive_buff, filename);
  printf("Return value is %d - %s", n, filename);
  int tempsize = fsize(filename);
  if(tempsize < 0) {
    printf("File does not exists\n");
    return 0;
  }
  int filelen = strlen(filename);
  if(filelen <=8 && (filename[filelen-3]=='.' && filename[filelen-2]=='a' && filename[filelen-1]=='u')) {
    unsigned int filesize = tempsize;
    printf("Filename: %s, filesize = %u\n", filename, filesize);
    fflush(stdout);
    return 1;
  }
  printf("Filename: %s, does not meet the requirements\n", filename);
  return 0;
  
}

void timer_handler (int signum)
{
  retransmit = 1;
}

void waitforack() {
  unsigned int len = sizeof(struct sockaddr_in);
  unsigned short recv_val;
  int n;
  uint8_t payload[2];
  n = recvfrom(socknew, (uint8_t *)payload, 2, 0, (struct sockaddr *) &cliaddr, &len);
  if(n > 0) {
    // get the values from the ACK
    recv_val = littletoshort(payload);
    slptime = (long long)recv_val*1000000;
    if (slptime > 999999999) {
      slptime = 999999999;
    }
  } else {
    perror("-1");
  }
}

void logevent() {
  struct log *link = (struct log*) malloc(sizeof(struct log));
  bzero(link, sizeof(struct log));
  link->lambda = slptime;
  gettimeofday(&(link->time), NULL);
  if (current != NULL) {
    current->next = link;
  }
  current = link;

  if (link_length == 0) {
    head = current;
  }
  link_length++;
}

void handle_sigio(int sig) {
  waitforack();
}

void sendfile() {
  uint8_t buffer[MAX];
  unsigned int len;
  int fd = open(filename, O_RDONLY);
  len = sizeof(cliaddr);

  fcntl(socknew, F_SETOWN, getpid());
  fcntl(socknew, F_SETFL, O_NONBLOCK | FASYNC);

  signal(SIGIO, handle_sigio);

  link_length= 0;
  while (read(fd, buffer, blocksize_num) > 0) {
    logevent();
    sendto(socknew, (const uint8_t *)buffer, blocksize_num, MSG_CONFIRM, (const struct sockaddr *) &cliaddr,len);
    bzero(buffer, MAX);
    int temp6 = nanosleep((const struct timespec[]){{0, slptime}}, &time_rem);
    while (temp6 < 0) {
      temp6 = nanosleep(&time_rem, &time_rem);
    }
  }

  for(int i=0;i<8;i++) {
    sendto(socknew, (const uint8_t *)buffer, 0, MSG_CONFIRM, (const struct sockaddr *) &cliaddr,len);
  }
  file_read_done = 1;
  close(fd);
  printf("File Transfer done\n");
}

void filelogging() {
  // make the client log file name
  char logfilename[100];
  bzero(logfilename, 100);
  sprintf(logfilename, "%s%lld", logfile, client_num);
  char * client_logfile = logfilename;
  printf("logfile is %s\n", client_logfile);

  // perform file IO
  FILE *fp = fopen(logfilename, "w");
  int fd = fileno(fp);

  // output current to the file
  int i = 0;
  current = head;
  long long start_time = (current->time).tv_sec * 1000000 + (current->time).tv_usec;
  while (i < link_length) {
    // get the timestamp in micro-seconds
    long long int time;
    time = (current->time).tv_sec * 1000000 + (current->time).tv_usec;
    time = time - start_time;
    char newlog[100];
    bzero(newlog, 100);
    sprintf(newlog, "%llu %lld\n", time, current->lambda);

    write(fd, newlog, strlen(newlog));

    // move forwad in the linked list
    current = current->next;
    i++;
  }

  close(fd);
}


int main(int argc, char *argv[]) {
  ip = argv[1];
  port = argv[2];
  lambda = atof(argv[3]);
  slptime = 1000000000.0/lambda;
  if (slptime > 999999999) {
    slptime = 999999999;
  }
  logfile = argv[4];
  socket_connection();

  // make a while loop for continous server execution
  // fork the process to send the file and then make it wait on the new client
  // connection again. A client will only ever have one request. Make a new
  // socket and then the communication of the client for file transfer will
  // happen with the new socket made. The orignal socket will be used to listen
  // for new connections.


  client_num = 0;
  while (1) {
    if (receivefilename()) {
      // fork to make a new process
      client_num++;
      pid_t pid = fork();
      if (pid == 0) {
        // child node
        // make a new socket using a different port number
        // transer the file

        socknew = 0;
        if ((socknew = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) {
          perror("socket creation failed");
          exit(EXIT_FAILURE);
        }

        // update the port number to a new random port number
        struct sockaddr_in temp;
        temp.sin_family = AF_INET; // IPv4
        temp.sin_port = htons(0);
        temp.sin_addr.s_addr = inet_addr(ip);

        // Bind the socket with the server address
        if (bind(socknew, (const struct sockaddr *)&temp, sizeof(temp)) < 0) {
          perror("bind failed");
          exit(EXIT_FAILURE);
        }

        // send the audio file
        sendfile();

        // log the file
        filelogging();

        printf("Logging Done %lld\n", client_num);

        // exit the process
        exit(0);
      } else {
        // go back to recieve more requests
        // parent node
      }
    }
  }
  close(sockfd);
}


