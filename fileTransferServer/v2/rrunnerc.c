// Simple shell example using fork() and execlp().
#define _GNU_SOURCE
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
#include <netdb.h>
#include <ifaddrs.h>


struct request_format {
  char filename[8];
  unsigned int key;
  //unsigned char lsb;
  //unsigned char msb;
} request;


struct packet_format {
  unsigned char packet_no;
  //int size;
  char data[];
};

char * private;
int resend = 0;
int blocksize;

// Creating socket file descriptor
int sockfd;
struct sockaddr_in servaddr;
struct sockaddr_in cliaddr;
socklen_t server_struct_length = sizeof(servaddr);

void create_request(int , char *);
void send_request(int , char *);
unsigned int get_ip();
unsigned int bbdecode(unsigned int, unsigned int);
void complete_transfer(int, int , char **);

void handle_alarm(int sig) {
  alarm(0);
  // resend the request
  if (resend == 10) {
    printf("\nRequest tried 10 times\nTerminating client! Please try again!\n");
    close(sockfd);
    exit(0);
  }
  printf("Resending the request--\n");
  if (sendto(sockfd, &request, sizeof(request), 0, (struct sockaddr*)&servaddr, server_struct_length) < 0){
    printf("Unable to send message\n");
    exit(1);
  }

  // set the timer again
  struct itimerval timer;
  timer.it_interval.tv_sec = 0;
  timer.it_interval.tv_usec = 0;
  timer.it_value.tv_sec = 0;
  timer.it_value.tv_usec = 500000;
  signal(SIGALRM, handle_alarm);

  setitimer(ITIMER_REAL, &timer, 0);

  resend++;
}

int main(int argc, char **argv) {
  if (argc != 7) {
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
  blocksize = atoi(argv[5]);
  int windowsize = atoi(argv[6]);

  // create the socket descriptor
  if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) {
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


  // Bind the socket with the client address
  if (bind(sockfd, (struct sockaddr *)&cliaddr, sizeof(cliaddr)) < 0) {
    perror("bind failed");
    exit(EXIT_FAILURE);
  }


  // start communication
  printf("\n>----------\n");
  fflush(stdout);

  // get time stamp before sending the request
  struct timeval start;
  gettimeofday(&start, NULL);

  // send the 10 Byte request
  send_request(secret_key, filename);

  // create a new file to store the copy from the server
  char newfilename[12];
  sprintf(newfilename, "NEW_%s", request.filename);
  FILE *fp = fopen(newfilename, "w");
  int fd = fileno(fp);

  int i = 0;
  int flag = 1;
  int done = 0;
  int expected = 0;
  int last_packet_no = -1;
  int flush = 0;

  char **data = malloc(sizeof(char) * windowsize * blocksize);

  printf("Recieving file\n");

  while (1) {
    struct packet_format *packet;
    int size = sizeof(unsigned char) + sizeof(char) * blocksize;
    packet = malloc(size);
    memset(packet, 0, size);
    memset(packet->data, 0, blocksize);
    if (recvfrom(sockfd, packet, size, 0,
          (struct sockaddr*)&servaddr, &server_struct_length) < 0){
      perror("Couldn't receive packet:\n");
      exit(1);
    }
    int num = (int) (packet->packet_no);

    // check if this is the last packet recieved
    if (strlen(packet->data) < blocksize) {
      done = 1;
    }

    // packets were not dropped and we are free to write
    // this checks if the ACK was received or not byy the server.
    if (flush == 1) {
      if (num != expected) {
        // retransmission, ACK not recieved
        flag = 1;
        expected = num;
      } else {
        // new packet, ACK recieved
        for (int j = 0; j < i; j++) {
          write(fd, data[j], blocksize);
        }
      }
      // fee the data
      for (int j = 0; j < i; j++) {
        free(data[j]);
      }
      i = 0;
      flush = 0;
      last_packet_no = num - 1;
    }


    // check if the data packet is a retrasmitted data packet
    // required for the case wherre packet was dropped and retransmision started
    if (expected == num) {
      // printf("In retrasmission %d, %d\n", offset, num);
      flag = 1;
      last_packet_no = expected - 1;
      for (int j = 0; j < i; j++) {
        free(data[j]);
      }
      i = 0;
    }

    // check if dropped
    // make sure to consider the case where the last packet arrives
    // but the window in which the lsat packet arrived does not have a dropped
    // packet

    if (num - last_packet_no != 1) {
      // a packet was dropped
      // printf("In dropped\n");
      if (num > windowsize - 1) {
        last_packet_no = windowsize - 1;
      } else {
        last_packet_no = -1;
      }
      // free the data
      for (int j = 0; j < i; j++) {
        free(data[j]);
      }
      i = 0;
      done = 0;
      flag = 0;
      flush = 0;
    } else {
      // packet was not dropped yet
      // printf("In not dropped %s\n", packet->data);
      last_packet_no = num;
      data[i] = malloc(sizeof(char) * blocksize);
      memcpy(data[i], packet->data, blocksize);
      i++;
    }


    // send the ACK if all W packets have been recieved
    if (done == 0 && flag == 1 &&
        ((last_packet_no == windowsize - 1) ||
         (last_packet_no == 2*windowsize - 1))) {

      if (sendto(sockfd, &last_packet_no, sizeof(int), 0, (struct sockaddr*)&servaddr, server_struct_length) < 0) {
        printf("Unable to send message\n");
        return -1;
      }

      if (last_packet_no == 2*windowsize - 1) {
        last_packet_no = -1;
      }

      if (expected == 0) {
        expected = windowsize;
      } else {
        expected = 0;
      }

      // prime the data for writing
      flush = 1;
    }

    if (done == 1) { // if file transmission is done
      complete_transfer(fd, i, data);
      free(packet);
      packet = NULL;
      printf("File recieved!\n\n");
      break;
    }

    free(packet);
    packet = NULL;

  } // end-while




  struct timeval end;
  gettimeofday(&end, NULL);
  fseek(fp, 0L, SEEK_END);
  long long int sz = ftell(fp);


  if (sz == 0) {
    printf("Request was not completed! Please try again.\n");
    printf(">----------\n\n");
    fflush(stdout);
    remove(newfilename);
    exit(0);
  }

  printf("Name of the requested file is: %s\n", filename);
  printf("File is stored as: %s\n", newfilename);
  printf("Size of the file  is: %lld\n\n", sz);

  long double completion_time = ((end.tv_sec * 1000000 + end.tv_usec) - (start.tv_sec * 1000000 + start.tv_usec));
  completion_time = completion_time/1000;
  printf("Completion time is:  %0.3Lfms\n", completion_time);
  fflush(stdout);

  long double Bpms = (sz * 1000) / (completion_time *1024);
  long double bps = (sz * 8) / (completion_time / 1000);


  printf("Throughput is: %0.3Lf MBps or %0.3Lf bps\n", Bpms, bps);
  //printf(">----------\n\n");
  fflush(stdout);




  free(data);
  data = NULL;
  close(fd);
  close(sockfd);
  printf(">----------\n\n");
  fflush(stdout);
} // end-main



void send_request(int secret_key, char *filename) {
  create_request(secret_key, filename);
  if (sendto(sockfd, &request, sizeof(request), 0, (struct sockaddr*)&servaddr, server_struct_length) < 0){
    printf("Unable to send message\n");
    exit(1);
  }

  struct itimerval timer;
  timer.it_interval.tv_sec = 0;
  timer.it_interval.tv_usec = 0;
  timer.it_value.tv_sec = 0;
  timer.it_value.tv_usec = 500000;
  signal(SIGALRM, handle_alarm);

  setitimer(ITIMER_REAL, &timer, 0);

  // recieve ACK and send the ACK ACK
  int ack;
  if (recvfrom(sockfd, &ack, sizeof(int), 0,
        (struct sockaddr*)&servaddr, &server_struct_length) < 0){
    printf("Couldn't receive\n");
    exit(1);
  }
  // cancel the alarm
  alarm(0);

  create_request(secret_key + 1, filename);
  if (sendto(sockfd, &request, sizeof(request), 0, (struct sockaddr*)&servaddr, server_struct_length) < 0){
    printf("Unable to send message\n");
    exit(1);
  }

} // end-send_request


void create_request(int secret_key, char *filename) {
  //int foo = secret_key;
  //unsigned char lsb = (unsigned)foo & 0xff; // mask the lower 8 bits
  //unsigned char msb = (unsigned)foo >> 8;   // shift the higher 8 bits
  //request.lsb = lsb;
  //request.msb = msb;

  // encrypt the key
  unsigned int ip = get_ip();
  request.key = bbdecode(ip, secret_key);


  int j = 0;
  for (int i = 0; i < strlen(filename); i++, j++) {
    request.filename[i] = filename[i];
  }
  request.filename[j] = '\0';

} // end-create_request


unsigned int bbdecode(unsigned int x, unsigned int prikey) {
  // convert both of them to big endian to avoid dealing with both seperately
  unsigned int x_big = htonl(x);
  unsigned int prikey_big = htonl(prikey);

  return (x_big ^ prikey_big);

} // end-bbdecode


unsigned int get_ip() {
  struct ifaddrs *ifaddr;
  int family, s;
  unsigned int temp;
  char host[NI_MAXHOST];

  if (getifaddrs(&ifaddr) == -1) {
    perror("getifaddrs");
    exit(EXIT_FAILURE);
  }

  for (struct ifaddrs *ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
    if (ifa->ifa_addr == NULL)
      continue;

    family = ifa->ifa_addr->sa_family;

    if (family == AF_INET &&
        ((strcmp("bond0", ifa->ifa_name) == 0) ||
         (strcmp("eth0", ifa->ifa_name) == 0))) {
      // printf("%-8s\n", ifa->ifa_name);

      s = getnameinfo(ifa->ifa_addr,
          (family == AF_INET) ? sizeof(struct sockaddr_in) :
          sizeof(struct sockaddr_in6),
          host, NI_MAXHOST,
          NULL, 0, NI_NUMERICHOST);
      if (s != 0) {
        printf("getnameinfo() failed: %s\n", gai_strerror(s));
        exit(EXIT_FAILURE);
      }

      temp = inet_addr(host);
      break;

    }
  }

  freeifaddrs(ifaddr);
  return temp;
} // end-get_ip



void complete_transfer(int fd, int i, char **data) {
  int temp = -1;
  for (int j = 0; j < 8; j++) {
    if (sendto(sockfd, &temp, sizeof(int), 0, (struct sockaddr*)&servaddr, server_struct_length) < 0) {
      perror("Unable to send message\n");
      exit(1);
    }
  }

  for (int j = 0; j < i; j++) {
    if (j == i - 1) {
      write(fd, data[j], strlen(data[j]));
    } else {
      write(fd, data[j], blocksize);
    }
  }
  for (int j = 0; j < i; j++) {
    free(data[j]);
  }
  i = 0;

} // end-complete_transfer





//------------------------------------------------------------------------------


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





/*
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
 */




