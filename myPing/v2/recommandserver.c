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

void handle_interupt(int sig) {
  printf("\n");
  exit(0);
}

int check_ip(char *ip) {
  char * lab_ip1 = "128.10.25.";
  char * lab_ip2 = "128.10.112.";
  char * lab_ip3 = "128.10.2.13";
  char * lab_ip4 = "10.192.30.11";

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

int toss() {
  srand(time(NULL));
  return (rand() % 2);
}


int main(int argc, char **argv) {

  if (argc != 3) {
    printf("Incorect Number of Arguments Passed! Please run the server again.\n");
    exit(1);
  } else {
    // print out the arguments
    for (int i = 0; i < argc; ++i) {
      printf("argv[%d]: %s\n", i, argv[i]);
    }
  }

  signal(SIGINT, handle_interupt);

  // convert the string to int port number
  int port = atoi(argv[2]);
  struct sockaddr_in servaddr;
  struct sockaddr_in cliaddr;
  socklen_t client_struct_length = sizeof(cliaddr);

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


  // Bind the socket with the server address
  if (bind(sockfd, (const struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
    perror("bind failed");
    exit(EXIT_FAILURE);
  }

  // Now server is ready to listen and verification
  if ((listen(sockfd, 5)) != 0) {
    printf("Listen failed...\n");
    exit(1);
  } else {
    //printf("Server listening..\n");
  }

  pid_t k0;
  int clisock = -1;
  //int status0;
  int status;
  pid_t k;
  printf("\n");
  printf(">-------- ");
  printf("\n");
  while (1) {
    fflush(stdout);
    /*
     * 1) accept a client connection
     * 2) toss a coin
     * 3) ignore or accept?
     * 4) to accept check the IP
     * 5) check if command is valid
     * 6) fork if secure
     */

    // accept the client connection
    clisock = accept(sockfd, (struct sockaddr *)&cliaddr, &client_struct_length);
    if (clisock < 0) {
      printf("server accept failed...\n");
      exit(0);
    } else {
      //printf("server accepted the client...\n");
    }

    // fork to make a thread for client
    k0 = fork();
    if (k0==0) {
      // child node
      while (1) {
        // read the command from client and copy it in buffer
        char *ip = inet_ntoa(cliaddr.sin_addr);
        char command[PIPE_BUF];
        bzero(command, PIPE_BUF);
        if (read(clisock, command, sizeof(command)) < 0) {
          perror("Cannot read command from client");
        }

        if (strcmp(command, "exitexitexit1234") == 0) {
          //printf("\nIN BREAK\n");
          close(clisock);
          break;
        } else {
          printf("Client %s's command is %s", ip, command);
        }

        // toss a choin
        int call = toss();
        //printf("Call is always tails for testing\n");
        //call = 0;
        if (call == 1) {
          // toss is heads, ignore the request
          printf("Client %s's request was ignored\n", ip);
          continue;
        } else {
          printf("Client %s's request was not ignored\n", ip);
        }

        // toss is tails, check the IP address
        //printf("The client ip is: %s\n", ip);
        int check = check_ip(ip);
        //printf("IP check is %d\n", check);
        if (check == 0) {
          printf("Invalid Client IP address!\n");
          continue;
        }


        // check if the command is permitted
        check = check_command(command);
        //printf("Command check is %d\n", check);
        if (check == 0) {
          printf("Command cannot be executed due to security checks!\n");
          printf("Only date and /bin/date allowed!\n");
          continue;
        }
        command[strlen(command) - 1] = '\0';

        int stdout_copy = dup(STDOUT_FILENO);
        int clisock_copy = dup(clisock);

        k = fork();
        if (k==0) {
          //child-child node to execute teh command
          dup2(clisock, STDOUT_FILENO);
          if (execlp(command, command, NULL) == -1) {// if execution failed, terminate child
            printf("Command could not be executed\n");
            dup2(stdout_copy, STDOUT_FILENO);
            close(stdout_copy);
            close(clisock);
            close(clisock_copy);
            exit(1);
          }
        } else {
          // child-parent node
          waitpid(k, &status, 0);
        }

      } // end-while
      break;
    } else {
      // parent node
      // waitpid(k0, &status0, 0);
    }

  } // end while


}

//char **commands;
//commands[0] = command;

// fork to execute child process
/*
   k = fork();
   if (k==0) {
// child code
printf("before execlp\n");
fflush(stdout);
dup2(clisock, STDOUT_FILENO);
if (execlp(command, command, NULL) == -1) {// if execution failed, terminate child
printf("Command could not be executed\n");
dup2(stdout_copy, STDOUT_FILENO);
close(stdout_copy);
close(clisock);
close(clisock_copy);
exit(1);
}
} else {
// parent code 
//clisock = -1;
waitpid(k, &status, 0);
//dup2(stdout_copy, STDOUT_FILENO);
//close(stdout_copy);
//close(clisock);
//close(clisock_copy);
//printf("\n\nDONE WITH DUP2\n\n");
}*/





/*

   pid_t k;
   int status;
   int len;

// integer to keep track of the public fifo
int public_fifo;
int private_fifo;

// Public FIFO file
char * server_fifo = "./serverfifo.dat";

// creating the named file(FIFO)
// mkfifo(<pathname>, <permission>)
mkfifo(server_fifo, 0666);

// handle interrupt signal
signal(SIGINT, handle_interupt);

// try to open the public fifo outside and manage it as it will be the same
// for  multiple clients

// read in from the public fifo which will contain client's fifo name and
// command, open the clients private fifo and then call dup2 to change the
// output of execvp from stdout to clients fifo. After writing close the
// clients fifo


while(1) {

// print prompt
fprintf(stdout,"[%d]$ ",getpid());
fflush(stdout);

// first open in read only and read
public_fifo = open(server_fifo, O_RDONLY);

int j = 0;
int flag = 0;
char temp2[1];
char buf[PIPE_BUF] = {'\0'};

// read in character by charater till \n
read(public_fifo, temp2, 1);
while (flag != 2 && j < PIPE_BUF) {
if (temp2[0] == '\n') {
flag++;
}
buf[j] = temp2[0];
read(public_fifo, temp2, 1);
// printf("\nRead is: %c\n", temp[0]);
j++;
}
// read in the \0
read(public_fifo, temp2, 1);

// read(public_fifo, buf, PIPE_BUF);

close(public_fifo);

// print the string and close
printf("\nClient command:\n----------\n%s----------\n", buf);
fflush(stdout);

// parse the client pid
int i = 0;
char client_pid[PIPE_BUF];
while(buf[i] != '\n') {
client_pid[i] = buf[i];
i++;
}
client_pid[i] = '\0';

// retrieve the client command
char * temp[2];
char ** client_message = parser(buf, temp, "\n");
char * command = client_message[1];

// parse and call execvp
// get the length
// make an array of pointers
len = strlen(command);
if(len == 1) { // only return key pressed
  continue;
}
char *argv[1000];
char ** argv1 = parser(command, argv, " ");

// open the client fifo for write only
char client_fifo[PIPE_BUF + 8] = "./client";
strcat(client_fifo, client_pid);
private_fifo = open(client_fifo, O_WRONLY);

int stdout_copy = dup(STDOUT_FILENO);
int private_fifo_copy = dup(private_fifo);

k = fork();
if (k==0) {
  // child code
  dup2(private_fifo, STDOUT_FILENO);
  if(execvp(argv1[0],argv1) == -1) {// if execution failed, terminate child
    dup2(stdout_copy, STDOUT_FILENO);
    close(stdout_copy);
    close(private_fifo);
    close(private_fifo_copy);
    exit(1);
  }
} else {
  // parent code 
  waitpid(k, &status, 0);
  dup2(stdout_copy, STDOUT_FILENO);
  close(stdout_copy);
  close(private_fifo);
  close(private_fifo_copy);
  //printf("\n\nDONE WITH DUP2\n\n");
}

}
*/
