#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h> 
#include <arpa/inet.h>
#include <signal.h>
#include <sys/epoll.h>
#include <pthread.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/wait.h>

#define MAX_LINE        256
#define BUF_SIZE        2048
#define MAX_CONN        16
#define MAX_EVENTS      32
#define CMD_TIMEOUT     10

typedef struct{
    char *cmd;
    int fd;
    struct sockaddr_in client_addr;
}cmdPrcData;

struct sockaddr_in server_addr;

void validate_convert_port(char *port_str, struct sockaddr_in *sock_addr);
void runner_run();
void* cmd_processor(void* args);

static void set_sockaddr(struct sockaddr_in *addr)
{
	bzero((char *)addr, sizeof(struct sockaddr_in));
	addr->sin_family = AF_INET;
	addr->sin_addr.s_addr = INADDR_ANY;
	if(addr->sin_port == 0)
        addr->sin_port = htons(8080);
}

static int setnonblocking(int sockfd)
{
	if (fcntl(sockfd, F_SETFL, fcntl(sockfd, F_GETFL, 0) | O_NONBLOCK) ==
	    -1) {
		return -1;
	}
	return 0;
}

static void epoll_ctl_add(int epfd, int fd, uint32_t events)
{
	struct epoll_event ev;
	ev.events = events;
	ev.data.fd = fd;
	if (epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &ev) == -1) {
		perror("epoll_ctl()\n");
		exit(1);
	}
}

int main(int argc, char *argv[])
{
    if (argc != 2) {
        printf("%s<port-number>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    memset(&server_addr, 0, sizeof(server_addr));
    validate_convert_port(argv[1],  &server_addr);

    runner_run();
}

void runner_run(){
    int n;
	int epfd;
	int nfds;
	int listen_sock;
	int conn_sock;
	int socklen;
	char buf[BUF_SIZE];
	struct sockaddr_in srv_addr;
	struct sockaddr_in cli_addr;
	struct epoll_event events[MAX_EVENTS];

    //execlp("/bin/sh","/bin/sh", "-c", "ls -l", (char *)NULL);

    listen_sock = socket(AF_INET, SOCK_STREAM, 0);

	set_sockaddr(&srv_addr);
	bind(listen_sock, (struct sockaddr *)&srv_addr, sizeof(srv_addr));

	setnonblocking(listen_sock);
	listen(listen_sock, MAX_CONN);

	epfd = epoll_create(1);
	epoll_ctl_add(epfd, listen_sock, EPOLLIN | EPOLLOUT | EPOLLET);

	socklen = sizeof(cli_addr);
	for (;;) {
		nfds = epoll_wait(epfd, events, MAX_EVENTS, -1);
		for (int i = 0; i < nfds; i++) {
			if (events[i].data.fd == listen_sock) {
				/* handle new connection */
				conn_sock =
				    accept(listen_sock,
					   (struct sockaddr *)&cli_addr,
					   &socklen);

				inet_ntop(AF_INET, (char *)&(cli_addr.sin_addr),
					  buf, sizeof(cli_addr));
				printf("[+] connected with %s:%d\n", buf,
				       ntohs(cli_addr.sin_port));

				setnonblocking(conn_sock);
				epoll_ctl_add(epfd, conn_sock,
					      EPOLLIN | EPOLLET | EPOLLRDHUP |
					      EPOLLHUP);
			} else if (events[i].events & EPOLLIN) {
				/* handle EPOLLIN event */
				for (;;) {
					bzero(buf, sizeof(buf));
					n = read(events[i].data.fd, buf,
						 sizeof(buf));
					if (n <= 0 /* || errno == EAGAIN */ ) {
						break;
					} else {
                        pthread_t cmd_prc_thread;
                        cmdPrcData* threadData = 
                            (cmdPrcData*)malloc(sizeof(cmdPrcData));
                        char *cmd = (char*)malloc(sizeof(buf));
                        memcpy(cmd, buf, sizeof(buf));
                        threadData->cmd = cmd;
                        threadData->fd = events[i].data.fd;
                        pthread_create(&cmd_prc_thread, NULL, cmd_processor, threadData);
					}
				}
			} else {
				printf("[+] unexpected\n");
			}
			/* check if the connection is closing */
			if (events[i].events & (EPOLLRDHUP | EPOLLHUP)) {
				printf("[+] connection closed\n");
				epoll_ctl(epfd, EPOLL_CTL_DEL,
					  events[i].data.fd, NULL);
				close(events[i].data.fd);
				continue;
			}
		}
	}
}

void* cmd_processor(void* args){
    cmdPrcData* threadData = (cmdPrcData*)args;

	printf("[+] data: %s\n", threadData->cmd);
	//write(threadData->fd, threadData->cmd, strlen(threadData->cmd));

    int fd_out[2], fd_err[2];
    pid_t   pid;
    int ch_status;

    ssize_t read_size;
    char read_buffer[BUF_SIZE];
    ssize_t output_shift = 0;
    char output_buffer[BUF_SIZE];
    ssize_t error_shift = 0;
    char error_buffer[BUF_SIZE];

    char cl_buffer[BUF_SIZE];

    if((pipe(fd_out) !=0)||(pipe(fd_err) != 0)){
        perror("pipe() error\n");
        exit(1);
    }
    
    if((pid = fork()) == -1)
    {
        perror("fork() error");
        exit(1);
    }

    if(pid == 0)
    {
        alarm(CMD_TIMEOUT);
        /* Потомок закрывает вход */
        close(fd_out[0]);
        close(fd_err[0]);

        dup2(fd_out[1], STDOUT_FILENO);
        dup2(fd_err[1], STDERR_FILENO);

        /* Посылаем "string" через выход канала */
        printf("Exec cmd: %s\n", threadData->cmd);
        execlp("/bin/sh","/bin/sh", "-c", threadData->cmd, (char *)NULL);
        //execl("ls", NULL);
        close(fd_out[1]);
        close(fd_err[1]);
        exit(-1);
    }
    else{
        close(fd_out[1]);
        close(fd_err[1]);
    }

    waitpid(pid, &ch_status, 0);

if (WIFSIGNALED(ch_status)) {
        char *err_msg =  "Error: Command timed out";
        write(threadData->fd, err_msg, sizeof(err_msg));

        close(fd_out[0]);
        close(fd_err[0]);

        free(threadData->cmd);
        free(threadData);

        printf("Thread ended\n");
        pthread_exit(0);
    }

    while ((read_size = read(fd_out[0], read_buffer, sizeof(read_buffer))) > 0) {
        printf("Readed size %d\n", read_size);
        read_buffer[read_size] = '\0';
        memcpy(&output_buffer[output_shift],read_buffer, read_size+1);
        output_shift = read_size + 1; 
    }

    while ((read_size = read(fd_err[0], read_buffer, sizeof(read_buffer))) > 0) {
        read_buffer[read_size] = '\0';
        memcpy(&error_buffer[error_shift], read_buffer, read_size+1);
        error_shift = read_size + 1; 
    }

    printf("Write rslts of %s\n Size: %d\n", output_buffer, output_shift);

    

    if(error_shift > 0){
        snprintf(cl_buffer, BUF_SIZE, "%s%d%s%s", "Exit code: ", WIFSIGNALED(ch_status), "\n",error_buffer);
        write(threadData->fd, cl_buffer, strlen(cl_buffer));
    }
    else{
        //write(threadData->fd, output_buffer, output_shift);
        snprintf(cl_buffer, BUF_SIZE, "%s%d%s%s", "Exit code: ", WIFSIGNALED(ch_status), "\n", output_buffer);
        write(threadData->fd, cl_buffer, strlen(cl_buffer));
    }

    free(threadData->cmd);
    free(threadData);

    close(fd_out[0]);
    close(fd_err[0]);

    printf("Thread ended\n");
    pthread_exit(0);
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
