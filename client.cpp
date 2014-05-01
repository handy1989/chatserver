#include <pthread.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define MAX_LINE_LEN 4096
#define __DEBUG__ 1
#define DEBUG(argv, format...)do{\
	if (__DEBUG__){\
		fprintf(stderr, argv, ##format);\
	}\
}while(0)

void *recv(void *arg)
{
	int sockfd = *(int *)arg;
	int n = 0;
	char recv_line[MAX_LINE_LEN];
	while (true)
	{
		n = recv(sockfd, recv_line, MAX_LINE_LEN, 0);
		if (n <= 0)
		{
			fprintf(stderr, "recieve from server failed!\n");
			break;
		}
		recv_line[n] = 0;
		printf(">>%s\n", recv_line);
        fflush(stdout);
	}
}

void *send(void *arg)
{
	int sockfd = *static_cast<int *>(arg);
	int n = 0;
	char sendline[MAX_LINE_LEN];
	while (true)
	{
		fgets(sendline, MAX_LINE_LEN, stdin);
		sendline[strlen(sendline) - 1] = 0;
		if (strcmp(sendline, "quit") == 0)
		{
				break;
		}
		if (send(sockfd, sendline, strlen(sendline), 0) < 0)
		{
				printf("send msg error: %s(errno: %d)\n", strerror(errno), errno);
				break;
		}
	}
}

int initSock(char *ip, int port)
{
	struct sockaddr_in servaddr;
	int sockfd = socket(AF_INET, SOCK_STREAM, 0);
	DEBUG("sockfd: %d\n", sockfd);
	if (sockfd < 0)
	{
		printf("create socket error: %s(errno: %d\n)\n", strerror(errno), errno);
		return -1;
	}
	memset(&servaddr, 0, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(port);
	if (inet_pton(AF_INET, ip, &servaddr.sin_addr) <= 0)
	{
		printf("inet_pton error for %s\n", ip);
		return -1;
	}
	if (connect(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0)
	{
		printf("connect error: %s(errno: %d)\n", strerror(errno), errno);
		return -1;
	}
    return sockfd;
}

int main(int argc, char *argv[])
{
	int sockfd;
	if (argc != 3)
	{
		printf("usage: %s <ipaddress> <port>\n", argv[0]);
		return 1;
	}
    if ((sockfd = initSock(argv[1], atoi(argv[2]))) < 0)
    {
        return 1;
    }

	pthread_t thread_recv, thread_send;
	pthread_create(&thread_recv, NULL, recv, &sockfd);
	pthread_create(&thread_send, NULL, send, &sockfd);
	pthread_join(thread_recv, NULL);
	pthread_join(thread_send, NULL);
	
    close(sockfd);
	return 0;
}
