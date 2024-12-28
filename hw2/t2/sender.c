#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h> 
#include <arpa/inet.h>
#include <signal.h>
#include <sys/epoll.h>
#include <pthread.h>
#include <errno.h>

#define MAX_LINE 256
#define BUF_SIZE 2048

struct sockaddr_in server_addr;

void validate_convert_port(char *port_str, struct sockaddr_in *sock_addr);
void sender_run();

static void set_sockaddr(struct sockaddr_in *addr)
{
	bzero((char *)addr, sizeof(struct sockaddr_in));
	addr->sin_family = AF_INET;
	addr->sin_addr.s_addr = INADDR_ANY;
	if(addr->sin_port == 0)
        addr->sin_port = htons(8080);
}

int main(int argc, char *argv[])
{
    if (argc != 2) {
        printf("%s<port-number>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    memset(&server_addr, 0, sizeof(server_addr));
    validate_convert_port(argv[1],  &server_addr);

    sender_run();
}

void sender_run(){
	int sockfd;
    int n;
	char buf[BUF_SIZE];
    char cmd[MAX_LINE];

    sockfd = socket(AF_INET, SOCK_STREAM, 0);

	set_sockaddr(&server_addr);

	if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
		perror("connect()");
		exit(1);
	}

    for (;;) {
		printf("input: ");
		fgets(cmd, sizeof(cmd), stdin);
		cmd[strlen(cmd) - 1] = '\0';
		write(sockfd, cmd, strlen(cmd));
		bzero(cmd, sizeof(cmd));
      
    recv(sockfd, buf, sizeof(buf), 0);
		printf("result: %s\n", buf);
    bzero(buf, sizeof(buf));

    printf("End of cmd!\n");
	}
	close(sockfd);
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
