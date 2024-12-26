#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h> #include <arpa/inet.h>
#include <signal.h>
#include <sys/epoll.h>
#include <pthread.h>
#include "colors.h"

#define BUFFER_SIZE 1024
#define MAX_EVENTS 2

int client_socket = -1;
int epoll_fd = -1;
char* name;
sig_atomic_t clientIsActive = 0;

static void sendto_named(char *msg);

static void sigint_handler(int signo)
{
  clientIsActive = 0;
  (void)printf("Reg sigINT!\n");
  sendto_named("Left the chat");
  (void)close(epoll_fd);
  (void)close(client_socket);
  sleep(2);
  (void)printf("Caught sigINT!\n");
  exit(EXIT_SUCCESS);
}

void validate_convert_port(char *port_str, struct sockaddr_in *sock_addr)
{
 int port;

 if (port_str == NULL) {
   perror("Invalid port_str\n");
   exit(EXIT_FAILURE);
 }

 if (sock_addr == NULL) {
   perror("Invalid sock_addr\n");
   exit(EXIT_FAILURE);
 }

 port = atoi(port_str);

 if (port == 0) {
   perror("Invalid port\n");
   exit(EXIT_FAILURE);
 }

 sock_addr->sin_port = htons((uint16_t)port);
 printf("Port: %d\n",
 ntohs(sock_addr->sin_port));
}

void validate_convert_addr(
char *ip_str,
struct sockaddr_in *sock_addr)
{
  if (ip_str == NULL) {
   perror("Invalid ip_str\n");
   exit(EXIT_FAILURE);
 }

 if (sock_addr == NULL) {
   perror("Invalid sock_addr\n");
   exit(EXIT_FAILURE);
 }

 printf("IP Address: %s\n", ip_str);

 if (inet_pton(AF_INET, ip_str,
 &(sock_addr->sin_addr)) <= 0) {
    perror("Invalid address\n");
    exit(EXIT_FAILURE);
  }
}

void validate_convert_name(
char *name_str  )
{
  name = name_str;
}

#define MAX_LINE        256
struct sockaddr_in server_addr;
char message[MAX_LINE];
char named_message[MAX_LINE];

static void sendto_named(char *msg){
  snprintf(named_message, MAX_LINE, "%s%s%s", name, ": ", msg);
    int ret = sendto(client_socket, named_message, strlen(named_message), 0, (struct sockaddr*)&server_addr, sizeof(server_addr));
    //printf("update\n");

    if (ret < 0) {
       perror("send error\n");
       (void)close(client_socket);
       exit(1);
    }
}

void recv_data(char *buffer)
{
  int ret, len;

  len = recvfrom(client_socket,
  buffer, BUFFER_SIZE, 0, NULL, NULL);

  if (len > 0) {
    buffer[len] = '\0';
    //(void)printf("%s\n",
    //buffer);

    int name_i = (int)(strchr(buffer, ':')-buffer);
    char noname_buf[BUFFER_SIZE];
    char name_buf[16];
    strncpy(noname_buf, buffer + name_i + 2, strlen(buffer) - name_i - 2);
    noname_buf[strlen(buffer) - name_i - 2] = '\0';
    strncpy(name_buf, buffer, name_i);
    name_buf[name_i] = '\0';
    //printf ("L %d\n", name_i);
    //printf ("N %s\n", name_buf);
    //printf ("M %s\n", noname_buf);
    printf (ANSI_COLOR_BLUE2 "%s" ANSI_COLOR_RESET ": %s\n" , name_buf, noname_buf);

  } else if (len == 0) {
      printf("Connection closed\n");
      exit(EXIT_FAILURE);
   }
}


void register_signal_handler(
int signum,
void (*handler)(int))
{
  if (signal(signum, handler) == SIG_ERR)
  {
     printf("Cannot handle signal\n");
     exit(EXIT_FAILURE);
  }
}



void* kb_reader(void* args){
	while(clientIsActive){
		//printf("Message: ");
		fgets(message, MAX_LINE, stdin);
		int c = strlen(message) - 1;
		message[c] = '\0';
    sendto_named(message);
		// snprintf(named_message, MAX_LINE, "%s%s%s", name, ": ", message);
    // int ret = sendto(client_socket, named_message, strlen(named_message), 0, (struct sockaddr*)&server_addr, sizeof(server_addr));
    // //printf("sendbuffer = %s\n", message);

    // if (ret < 0) {
    //    perror("send error1\n");
    //    (void)close(client_socket);
    //    exit(1);
    // }
  }
}

void* updater(void* args){
	while(clientIsActive){
		//printf("Message: ");
    sendto_named("upd");
		// snprintf(named_message, MAX_LINE, "%s%s", name, ": upd");
    // int ret = sendto(client_socket, named_message,
    // strlen(named_message), 0,
    // (struct sockaddr*)&server_addr,
    // sizeof(server_addr));
    // //printf("update\n");

    // if (ret < 0) {
    //    perror("send error2\n");
    //    (void)close(client_socket);
    //    exit(1);
    // }
    sleep(1);
  }
}

int main(int argc, char *argv[])
{
  int ready_fds;
  int ret;
  struct epoll_event
  events[MAX_EVENTS];
  
  char buffer[BUFFER_SIZE];
  char *str = "HI";

  register_signal_handler(SIGINT, sigint_handler);

  if (argc != 4) {
    printf("%s<port-number><ip-addr><name>\n",
    argv[0]);
    exit(EXIT_FAILURE);
  }

  memset(&server_addr, 0,
  sizeof(server_addr));
  server_addr.sin_family = AF_INET;
  validate_convert_port(argv[1],  &server_addr);
  validate_convert_addr(argv[2],  &server_addr);
  validate_convert_name(argv[3]);

  client_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

  if (client_socket < 0) {
    perror("socket");
    return -1;
  }

  epoll_fd = epoll_create1(0);

  if (epoll_fd < 0) {
   perror("Epoll creation failed");
   (void)close(client_socket);
   return -2;
  }

  struct epoll_event event;
  pthread_t message_reader;
  pthread_t message_updater;

  event.events = EPOLLIN | EPOLLET;
  event.data.fd = client_socket;
  ret = epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_socket, &event);

  if (ret < 0) {
       perror("Epoll_ctl failed");
       (void)close(client_socket);
       return -3;
  }

  clientIsActive = 1;

  pthread_create(&message_reader, NULL, kb_reader, NULL);
  pthread_create(&message_updater, NULL, updater, NULL);

  while (1) {
    

    ready_fds = epoll_wait(epoll_fd,
                events,
                MAX_EVENTS, -1);

    if (ready_fds < 0) {
      perror("Epoll wait failed");
      break;
    }

    if (events[0].data.fd ==
    client_socket) {
      recv_data(buffer);
    }

    else
    {
        printf("Other event\n");
    }
  }

  (void)close(epoll_fd);
  (void)close(client_socket);
  
  return 0;
}
