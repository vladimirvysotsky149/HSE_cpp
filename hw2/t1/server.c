#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <signal.h>
#include <sys/epoll.h>

#include <sys/socket.h>
#include <netdb.h>
#include "colors.h"

#define BUFFER_SIZE 1024
#define MAX_EVENTS 5

int server_socket = -1;
int epoll_fd = -1;

static void sigint_handler(int signo)
{
  (void)close(epoll_fd);
  (void)close(server_socket);
  sleep(2);
  (void)printf("Caught sigINT!\n");
  exit(EXIT_SUCCESS);
}

void register_signal_handler(int signum, void (*handler)(int))
{
  if (signal(signum, handler) == SIG_ERR) {
     printf("Cannot handle signal\n");
     exit(EXIT_FAILURE);
  }
}

void validate_convert_port(
char *port_str,
struct sockaddr_in *sock_addr)
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

 sock_addr->sin_port = htons(
 (uint16_t)port);
 printf("Port: %d\n",
 ntohs(sock_addr->sin_port));
}

char client_collection[16][16];
int client_not_upd[16];
int client_count = 0;

int get_usr_index(char *msg){
    int name_i = (int)(strchr(msg, ':')-msg);
    char name_str[16];
    strncpy(name_str, msg, name_i);
    name_str[name_i] = '\0';
    char buf_a[256];

    int found = 0;
    for(int i = 0; i < client_count; i++){
        if(i < 16){
            //printf("Check %s with %s size %d", name_str, client_collection[i], name_i);
            if(memcmp(name_str, client_collection[i], name_i)==0){
                return i+1;
            }
        }
    }
    return 0;
}

void set_usrs_wait_read_flag(char *msg){
    printf("Set flag\n");
    for(int i = 0; i < client_count; i++){
        client_not_upd[i] = 1;
    }
}

void clr_usr_wait_read_flag(char *msg){
    printf("Clr flag\n");
    int usr = get_usr_index(msg);
    if(usr > 0){
        client_not_upd[usr-1] = 0;
    }
}

int check_usr_wait_read_flag(char *msg){
    
    int usr = get_usr_index(msg);
    printf("Check flag for user %d\n", usr);
    if(usr > 0){
        return client_not_upd[usr-1];
    }
    return 0;
}

int check_usrs_wait_read_flag(char *msg){
    printf("Check flags\n");
    for(int i = 0; i < client_count; i++){
        if(client_not_upd[i] > 0)
            return 1;
    }
    return 0;
}

void add_usr(char *msg){
    //printf("Try add usr from %s\n", msg);
    int name_i = (int)(strchr(msg, ':')-msg);
    //printf("Shift is %d\n", name_i);
    char name_str[16];
    strncpy(name_str, msg, name_i);
    name_str[name_i] = '\0';
    //printf("Try add usr %s\n", name_str);
    char buf_a[256];

    int found = 0;
    for(int i = 0; i < client_count; i++){
        if(i < 16){
            if(memcmp(name_str, client_collection[i], name_i)==0){
                found++;
            }
        }
        else{
            client_count = 0;
        }
    }
    if(found == 0){
        memcpy(client_collection[client_count], name_str, name_i+1);
        client_count++;
        printf("Add usr %s\n", name_str);
    }
}

char saved_buf[1024];
int saved_buf_shift = 0;

void recv_send(char *buffer, struct sockaddr_in *client_addr_old)
{
  int len, ret = 0;
  ssize_t nread;
  struct sockaddr_in client_addr;

  socklen_t client_addr_len = sizeof(client_addr);

  len = recvfrom(server_socket, buffer, BUFFER_SIZE, MSG_WAITALL, (struct sockaddr*)&client_addr, &client_addr_len);

  //printf("Adddr addr_len %d, len is %d\n", client_addr_len, len);
  //static struct sockaddr_in addr;
  //char info[32];
  //if(client_addr == NULL){
  //  printf("client addr is null");
  //}
  //else{
  //  memcpy(info, client_addr, client_addr_len);
  //  info[31] = '\0';
  printf("client addr as str"ANSI_COLOR_YELLOW"%d\n"ANSI_COLOR_RESET, client_addr.sin_port);
  //}
  //memcpy(&addr, client_addr, client_addr_len);

    

  //printf(ANSI_COLOR_YELLOW "Rcv len %d\n" ANSI_COLOR_RESET, len);
    char keystr[] = "upd";
    if (len > 0) {
    buffer[len] = '\0';
    }
    int name_i = (int)(strchr(buffer, ':')-buffer);
    char noname_buf[BUFFER_SIZE];
    strncpy(noname_buf, buffer+name_i+2, sizeof(keystr));
    //printf("Buf %s\n", noname_buf);
    int res = memcmp(keystr, noname_buf, sizeof(keystr));
    if(res == 0){
        printf("update str %d\n", res);
        if(check_usr_wait_read_flag(buffer)){
            sendto(server_socket, saved_buf, saved_buf_shift, 0, (struct sockaddr*)&client_addr, client_addr_len);
        }
        clr_usr_wait_read_flag(buffer);
        if(check_usrs_wait_read_flag(buffer)==0)
            saved_buf_shift = 0;
    }
    else{
        printf("str %s\n", buffer);
        add_usr(buffer);
        memcpy(&saved_buf[saved_buf_shift], buffer, strlen(buffer));
        saved_buf_shift += strlen(buffer);
        saved_buf[saved_buf_shift] = ' ';
        saved_buf_shift++;
        set_usrs_wait_read_flag(buffer);
    }

  if (len > 0) {
    buffer[len] = '\0';
    printf("Received: %s\n",
    buffer);

    memset(buffer, 0,
    sizeof(buffer));
    strncpy(buffer, "HELLO",
    strlen("HELLO") + 1);
    buffer[strlen(buffer) + 1] = '\0';

    //add_usr(client_addr);
    //ret = sendto(server_socket, buffer, strlen(buffer), 0, (struct sockaddr*)&client_addr, client_addr_len);

    if (ret < 0) {
       perror("sendto");
       exit(EXIT_FAILURE);
      }
    } else if (len < 0) {
        perror("recvfrom");
        exit(EXIT_FAILURE);
    }
    printf("Sentbuffer = %s\n",
    buffer);
}

struct sockaddr_in client_addr;
/// @brief 
/// @param argc 
/// @param argv 
/// @return 
int main(int argc, char *argv[])
{
  int ready_fds;
  int ret;
  struct sockaddr_in server_addr/*, client_addr*/;
  char buffer[BUFFER_SIZE];
  struct epoll_event 
  events[MAX_EVENTS];
  struct epoll_event event;

  register_signal_handler(SIGINT,  sigint_handler);

  if (argc != 2) {
    printf("%s <port-number>",
    argv[0]);
    exit(EXIT_FAILURE);
  }

  memset(&server_addr, 0, sizeof(server_addr));
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = INADDR_ANY;
  validate_convert_port(argv[1], &server_addr);

  server_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

  if (server_socket < 0) {
    perror("socket");
    return -1;
  }

  ret = bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr));

  if (ret < 0) {
    perror("bind");
    (void)close(server_socket);
    return -2;
  }

  epoll_fd = epoll_create1(0);
 
  if (epoll_fd < 0) {
    perror("Epoll creation failed");
    exit(EXIT_FAILURE);
  }
	
  event.events = EPOLLIN;
  event.data.fd = server_socket;
  
  ret = epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_socket, &event);
 
  if (ret < 0) {
    perror("Epoll_ctl failed");
    (void)close(epoll_fd);
    (void)close(server_socket);
    return -3;
  }

  while (1) {
    ready_fds = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
		printf("event, %d\n", ready_fds);
    if (ready_fds < 0) {
perror("Epoll wait failed");
(void)close(epoll_fd);
(void)close(server_socket);
   	  break;
    }

    if (events[0].data.fd == server_socket) {
recv_send(buffer, &client_addr);
    }
  }    

  (void)close(epoll_fd);
  (void)close(server_socket);
  
   return 0;
}
