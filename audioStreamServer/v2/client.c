
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/time.h>
#include <signal.h>
#include <sys/stat.h>
#include "endian_convertor.h"
#include <sys/wait.h>
#include <fcntl.h>
#include <alsa/asoundlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <errno.h>

#define MAX 4096
#define SA struct sockaddr

volatile unsigned int cnt = 3;
volatile int recv_started = 1;
struct sockaddr_in servaddr;
struct sockaddr_in recvaddr;
int sockfd;
char *port, *ip;
char *filename;
int file_read_done = 0;
unsigned short blocksize_num;
long long windowsize_num;
long long filledbuf=0;
long long targetbuf;
double lambda;
unsigned short pspace = 0;
unsigned short method;
long long currbuf=0;
long long startbuf=0;
char *logfile;
char *fifo_buffer;
sem_t mutex;
size_t bufsize;
float epsilon;
float beta;

struct log {
  long long buffersize;
  struct timeval time;
  struct log *next;
};
struct log *head = NULL;
struct log *current = NULL;
long long int link_length;


char recv_filename[20] = "recv_";

/* audio codec library functions */

static snd_pcm_t *mulawdev;
static snd_pcm_uframes_t mulawfrms;

// Initialize audio device.
void mulawopen(size_t *bufsiz) {
  snd_pcm_hw_params_t *p;
  unsigned int rate = 8000;

  snd_pcm_open(&mulawdev, "default", SND_PCM_STREAM_PLAYBACK, 0);
  snd_pcm_hw_params_alloca(&p);
  snd_pcm_hw_params_any(mulawdev, p);
  snd_pcm_hw_params_set_access(mulawdev, p, SND_PCM_ACCESS_RW_INTERLEAVED);
  snd_pcm_hw_params_set_format(mulawdev, p, SND_PCM_FORMAT_MU_LAW);
  snd_pcm_hw_params_set_channels(mulawdev, p, 1);
  snd_pcm_hw_params_set_rate_near(mulawdev, p, &rate, 0);
  snd_pcm_hw_params(mulawdev, p);
  snd_pcm_hw_params_get_period_size(p, &mulawfrms, 0);
  *bufsiz = (size_t)mulawfrms;
  return;
}

// Write to audio device.
#define mulawwrite(x) snd_pcm_writei(mulawdev, x, mulawfrms)

// Close audio device.
void mulawclose(void) {
  snd_pcm_drain(mulawdev);
  snd_pcm_close(mulawdev);
}

unsigned short getpspace() {
  float gamma = 3.19;
  float alpha = 0.6;
  sem_wait(&mutex);
  if(method==0) {
    lambda = lambda + (epsilon*(targetbuf - filledbuf)/blocksize_num);
  }
  if(method == 1) {
    lambda = (lambda + (epsilon*(targetbuf - filledbuf)/blocksize_num) - beta*(lambda - gamma));
  }
  if(method ==2) {
    if(filledbuf <= blocksize_num*3) {
      lambda = (lambda + (0.5*epsilon*(targetbuf - filledbuf)/blocksize_num) - beta*(lambda - gamma)) + alpha*(lambda/2 - gamma/3);
    } else if (((windowsize_num - filledbuf) <= blocksize_num*3)) {
      lambda = (lambda + (5*epsilon*(targetbuf - filledbuf)/blocksize_num) - beta*(lambda - gamma)) + alpha*(lambda/2 - gamma/3);
    }else {
      lambda = (lambda + (epsilon*(targetbuf - filledbuf)/blocksize_num) - beta*(lambda - gamma));
    }
  }

  sem_post(&mutex);
  if(lambda < 0 ) {
    lambda = -lambda;
  }
  pspace = (unsigned short)(1000.0/lambda);
  return pspace;
}

void retry()
{
  close(sockfd);
  cnt--;
}

off_t fsize(const char *filename)
{
  struct stat st;
  if (stat(filename, &st) == 0)
    return st.st_size;

  return -1;
}

void catch_alarm(int sig)
{
  recv_started = 0;
  printf("Hit the alarm\n");
  retry();
}

void socket_connection()
{
  if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
  {
    perror("socket creation failed");
    exit(EXIT_FAILURE);
  }
  // Filling server information
  servaddr.sin_family = AF_INET;
  servaddr.sin_port = htons((unsigned int)atoi(port));
  servaddr.sin_addr.s_addr = inet_addr(ip);

  // memset(&recvaddr, 0, sizeof(recvaddr));
}

void chartouint(char *x, uint8_t *y)
{
  for (int i = 2; i < 10; i++)
  {
    y[i] = (uint8_t)x[i - 2];
  }
}

void sendfilename() {
  uint8_t send_buff[10];
  shorttolittle(blocksize_num, send_buff);
  chartouint(filename, send_buff);

  sendto(sockfd, (const char *)send_buff, sizeof(send_buff),
      MSG_CONFIRM, (const struct sockaddr *) &servaddr, 
      sizeof(servaddr));
  printf("Filename sent.\n");
}

void custom_alarm(unsigned int timerv) {
  struct sigaction sa;
  struct itimerval timer;

  /* Install timer_handler as the signal handler for SIGVTALRM. */
  memset (&sa, 0, sizeof (sa));
  sa.sa_handler = &catch_alarm;
  sigaction (SIGALRM, &sa, NULL);

  /* Configure the timer to expire after 500 msec... */
  timer.it_value.tv_sec = 0;
  timer.it_value.tv_usec = timerv;
  /* ... and no alarm at interval after that. */
  timer.it_interval.tv_sec = 0;
  timer.it_interval.tv_usec = 0;
  /* Start a virtual timer. It counts down whenever this process is
     executing. */
  setitimer (ITIMER_REAL, &timer, NULL);
}

void sendack(unsigned short value) {
  uint8_t payload[2];
  shorttolittle(value, payload);
  sendto(sockfd, (const uint8_t *)payload, 2,
      MSG_CONFIRM, (const struct sockaddr *) &recvaddr, 
      sizeof(servaddr));
}


void filelogging() {
  // make the client log file name
  char logfilename[100];
  bzero(logfilename, 100);
  sprintf(logfilename, "%s", logfile);
  char * client_logfile = logfilename;
  printf("logfile is %s\n", client_logfile);

  // perform file IO
  FILE *fp = fopen(logfile, "w");
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
    sprintf(newlog, "%llu %lld\n", time, current->buffersize);

    write(fd, newlog, strlen(newlog));

    // move forwad in the linked list
    current = current->next;
    i++;
  }

  close(fd);
}

void logevent() {
  struct log *link = (struct log*) malloc(sizeof(struct log));
  bzero(link, sizeof(struct log));
  link->buffersize = filledbuf;
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


int check_inputs(char *fname)
{
  unsigned int file_name_length = strlen(filename);
  if (file_name_length > 8)
  {
    printf("Filename length : %u greater than 8 bytes\n", file_name_length);
    return 0;
  }
  for(int i=0;i<file_name_length;i++) {
    if(!((filename[i]>='A' && filename[i]<='Z' ) || (filename[i]>='a' && filename[i]<='z' ) || filename[i]=='.')) {
      printf("Filename contains non-alpha characters %c\n", filename[i]);
      return 0;
    }
  }
  return 1;
}


void handle_sigalarm(int sig) {
  // mulawopen(&bufsize);
  char *buf;
  bufsize = 4096;
  buf = (char *)malloc(bufsize);
  bzero(buf, bufsize);
  if(filledbuf >= 4096) {
    sem_wait(&mutex);
    memcpy(buf, fifo_buffer+startbuf, bufsize);
    startbuf += bufsize;
    startbuf = startbuf%windowsize_num;
    filledbuf -= bufsize;
    logevent();
    sem_post(&mutex);
    sendack(getpspace());
    int error  = mulawwrite(buf);
    if(error < 0) {
      perror("mulawrite");
      snd_pcm_recover(mulawdev, error, 0);
    }
  }
  free(buf);
}


void streamaudio() {
  unsigned int len=sizeof(struct sockaddr_in);
  int n;
  bufsize = 4096;
  logevent();
  char *buffer = (char*) malloc(blocksize_num);
  fifo_buffer = (char*) malloc(windowsize_num);
  bzero(buffer, blocksize_num);
  bzero(fifo_buffer, windowsize_num);	
  //settimer();
  struct itimerval timer;
  /* Configure the timer to expire after 250 msec... */
  timer.it_value.tv_sec = 0;
  timer.it_value.tv_usec = 313000;
  /* ... and every 250 msec after that. */
  timer.it_interval.tv_sec = 0;
  timer.it_interval.tv_usec = 313000;
  /* Start a virtual timer. It counts down whenever this process is
     executing. */
  setitimer (ITIMER_REAL, &timer, NULL);
  signal(SIGALRM, handle_sigalarm);
  // int packet_read = 0;
  mulawopen(&bufsize);
  while(!file_read_done) {
    n = recvfrom(sockfd, (char *)buffer, blocksize_num, 
        0, (struct sockaddr *) &recvaddr,
        &len);
    // packet_read = 0;
    fflush(stdout);
    if(n==0) {
      file_read_done = 1;
    }
    if(n>0){
      sem_wait(&mutex);
      if(filledbuf + n <= windowsize_num) {
        currbuf = currbuf%windowsize_num;
        if((windowsize_num - currbuf) < n) {
          unsigned long part1 = (windowsize_num - currbuf);
          memcpy(fifo_buffer+currbuf, buffer, part1);
          currbuf = 0;
          if(n - part1 > 0) {
            memcpy(fifo_buffer, buffer+part1, n - part1);
            currbuf = (n-part1);
          }
        } else {
          memcpy(fifo_buffer+currbuf, buffer, n);
          currbuf = (currbuf+n)%(windowsize_num);
        }
        filledbuf +=n;
        // packet_read = 1;
    }
    logevent();
    sem_post(&mutex);
    sendack(getpspace());
    }
  }
  while(filledbuf >= 4096);
  free(buffer);
  mulawclose();
  free(fifo_buffer);
}


void readparam() {
  FILE *fp = fopen("audiocliparam.dat", "r");
  fscanf(fp,"%f",&epsilon);
  fscanf(fp,"%f",&beta);
  fclose(fp);
}


int main(int argc, char *argv[])
{
  ip = argv[1];
  port = argv[2];
  filename = argv[3];
  blocksize_num = (unsigned short)strtoul(argv[4], NULL, 0);
  windowsize_num = (long long)strtoll(argv[5], NULL, 0);
  targetbuf = (long long)strtoll(argv[6], NULL, 0);
  lambda = atof(argv[7]);
  pspace = (unsigned short)(1000.0/lambda);
  method =  (unsigned short)strtoul(argv[8], NULL, 0);
  logfile = argv[9];
  strcat(recv_filename, filename);
  readparam();


  sem_init(&mutex, 0, 1);
  if (check_inputs(filename))
  {
    socket_connection();
    sendfilename();
    streamaudio();
    filelogging();
    close(sockfd);
  }
  sem_destroy(&mutex);
  return 0;
}
