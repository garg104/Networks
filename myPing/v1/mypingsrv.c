// Server side implementation of UDP client-server model
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/wait.h>
#include <ctype.h>


#define PORT     8080
#define MAXLINE 1024

int sockfd;

void handle_interupt(int sig) {
  close(sockfd);
  exit(0);
}



// Driver code
int main(int argc, char **argv) {
  if (argc != 3) {
    printf("Incorect Number of Arguments Passed! Please run the server again.\n");
    fflush(stdout);
    exit(1);
  } else {
    // print out the arguments
    for (int i = 0; i < argc; ++i) {
      printf("argv[%d]: %s\n", i, argv[i]);
    }
  }

  // convert the string to int port number
  int port = atoi(argv[2]);
  //char buffer[MAXLINE];
  //char *hello = "Hello from server";
  struct sockaddr_in servaddr, cliaddr;
  socklen_t client_struct_length = sizeof(cliaddr);
  //int status;

  // Creating socket file descriptor
  if ((sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0 ) {
    perror("socket creation failed");
    exit(EXIT_FAILURE);
  }

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

  printf("Listening for incoming messages...\n\n");
  while (1) {
    int client_message[2];
    // int server_message[2];

    // Receive client's message:
    if (recvfrom(sockfd, client_message, sizeof(client_message), 0,
          (struct sockaddr*)&cliaddr, &client_struct_length) < 0){
      printf("Couldn't receive\n");
      return -1;
    }
    printf("Received message from IP: %s and port: %i\n",
        inet_ntoa(cliaddr.sin_addr), ntohs(cliaddr.sin_port));

    // printf("Msg from client: %d, %d\n", client_message[0], client_message[1]);

    //int S = client_message[0];
    int D = client_message[1];
    if (D == 0) {
      // send it back
      if (sendto(sockfd, client_message, sizeof(client_message), 0,
            (struct sockaddr*)&cliaddr, client_struct_length) < 0){
        printf("Can't send\n");
        return -1;
      }
    } else if (D == 99) {
      printf("Closing server with exit(1)\n");
      exit(1);
    } else if (D >= 1 && D <= 5) {
      // fork and sleep the child process
      pid_t pid = fork();
      if (pid == 0) {
        // child node
        // sleep
        //printf("SleepingServer\n");
        //fflush(stdout);
        sleep(D);
        //printf("SleepingServer1\n");
        //fflush(stdout);
        if (sendto(sockfd, client_message, sizeof(client_message), 0,
              (struct sockaddr*)&cliaddr, client_struct_length) < 0){
          printf("Can't send\n");
          return -1;
        }
        exit(0);
      }
    }

    fflush(stdout);
  }

  return 0;
}


// Change to uppercase:
/*
   for(int i = 0; client_message[i]; ++i)
   client_message[i] = toupper(client_message[i]);

// Respond to client:
strcpy(server_message, client_message);
 */

/*
   if (sendto(sockfd, client_message, sizeof(client_message), 0,
   (struct sockaddr*)&cliaddr, client_struct_length) < 0){
   printf("Can't send\n");
   return -1;
   }*/


//} else {
// parentNode
//  waitpid(pid, &status, 0);
//}

/*

   char client_message[2] = {'\0'};
   char server_message[2];
   if (recvfrom(sockfd, client_message, sizeof(client_message), 0,
   (struct sockaddr*)&cliaddr, &client_struct_length) < 0){
   printf("Couldn't receive\n");
   return -1;
   }
 */



