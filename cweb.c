#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <strings.h>
#include <string.h>
#include <ctype.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <iconv.h>
#include <pthread.h>
#include <sys/select.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <poll.h>
#include <errno.h>
#include "csocket.h"

//多线程高并发服务器
void *pth_do_things(void *arg)
{
	int len,ret,i;
	char buf[BUFSIZ];
	int cfd = *(int*)arg;
	while (1) {
		/*读取客户端发送数据*/
		len = read(cfd, buf, sizeof(buf));
		if (len == 0) {
			printf("the client %d closed...\n", cfd);
			break;                                              //跳出循环,关闭cfd
		}
		ret = write(STDOUT_FILENO, buf, len);
		if (ret == -1) {
			perror("write err");
			return 0;
		}
		/*处理客户端数据*/
		for (i = 0; i < len; i++)
			buf[i] = toupper(buf[i]);

		ret = write(cfd, buf, len);
		if (ret == -1) {
			perror("write err");
			return 0;
		}
	}
	close(cfd);
}

int main01(void)
{
	int sfd, cfd;
	pthread_t ptd;
	sfd = Init_socket(AF_INET, SOCK_STREAM, 0);
	Set_port_multiplex(sfd);
	Make_server(sfd);
	printf("wait for client connect ...\n");
	while (1) {
		cfd = Accept_socket(sfd);
		pthread_create(&ptd, NULL, pth_do_things, &cfd);
		pthread_detach(ptd);
	}
	close(sfd);
	return 0;
}




//多路IO转接服务器  select
int main02(void)
{
	int sfd, i,j, maxi, maxfd, connfd,sockfd,n;
	fd_set rset, allset;
	int nready, client[FD_SETSIZE]; char buf[BUFSIZ];
	sfd = Init_socket(AF_INET, SOCK_STREAM, 0);
	Set_port_multiplex(sfd);
	Make_server(sfd);
	printf("wait for client connect ...\n");

	maxfd = sfd; 				/* 初始化 */
	maxi = -1;					/* client[]的下标 */

	memset(client, -1, sizeof(client));

	FD_ZERO(&allset);
	FD_SET(sfd, &allset); /* 构造select监控文件描述符集 */

	for (;;) {
		rset = allset;
		nready = select(maxfd + 1, &rset, NULL, NULL, NULL);
		if (nready < 0) {
			perror("select error");
			exit(1);
		}
		if (FD_ISSET(sfd, &rset)) { /* new client connection */
			connfd = Accept_socket(sfd);
			for (i = 0; i < FD_SETSIZE; i++) {
				if (client[i] < 0) {
					client[i] = connfd;   //保存accept返回的文件描述符到client[]里
					break;
				}
			}
			if (i == FD_SETSIZE) {
				fputs("too many clients\n", stderr);
				exit(1);
			}
			FD_SET(connfd, &allset);   //添加一个新的文件描述符到监控信号集里 
			if (connfd > maxfd)
				maxfd = connfd;   //select第一个参数需要
			if (i > maxi)
				maxi = i;    //更新client[]最大下标值
			if (--nready == 0)   //如果没有更多的就绪文件描述符继续回到上面select阻塞监听,负责处理未处理完的就绪文件描述符
				continue;
		}
		for (i = 0; i <= maxi; ++i) {    //检测哪个clients有数据就绪
			if ((sockfd = client[i]) < 0)
				continue;
			if (FD_ISSET(sockfd, &rset)) {
				if ((n = read(sockfd, buf, sizeof(buf))) == 0) {
					close(sockfd);
					FD_CLR(sockfd, &allset);
					client[i] = -1;
				}
				else if (n > 0) {
					for (j = 0; j < n; j++)
						buf[j] = toupper(buf[j]);
					write(sockfd, buf, n);
				}
				if (--nready == 0)
					break;
			}
		}
	}
	close(sfd);
	return 0;
}

#define MAXLINE 80
#define MAX_OPEN 1024
# define POLLRDNORM	0x040
//多路IO转接服务器  poll
int main03()
{
	int sfd, i, j, maxi, connfd, sockfd;
	ssize_t n;
	int nready;
	char buf[MAXLINE];
	struct pollfd client[MAX_OPEN];

	sfd = Init_socket(AF_INET, SOCK_STREAM, 0);
	Set_port_multiplex(sfd);
	Make_server(sfd);
	printf("wait for client connect ...\n");

	client[0].fd = sfd;
	client[0].events = POLLRDNORM;      //监听普通读事件

	for (i = 1; i < MAX_OPEN; i++)
		client[i].fd = -1;
	maxi = 0;

	for (;;) {
		nready = poll(client, maxi + 1, -1);
		if (nready < 0) {
			perror("poll error");
			exit(1);
		}
		if (client[0].revents & POLLRDNORM) {  //有客户端连接请求
			connfd = Accept_socket(sfd);
			for (i = 1; i < MAX_OPEN; i++) {
				if (client[i].fd < 0) {
					client[i].fd = connfd;
					break;
				}
			}
			if (i == MAX_OPEN) {
				fputs("too many clients\n", stderr);
				exit(1);
			}
			client[i].events = POLLRDNORM;
			if (i > maxi)
				maxi = i;
			if (--nready <= 0)
				continue;
		}
		for (i = 1; i <= maxi; i++) {
			if ((sockfd = client[i].fd) < 0)
				continue;
			if (client[i].revents &(POLLRDNORM | POLLERR)) {
				if ((n = read(sockfd, buf, sizeof(buf))) < 0) {
					if (errno == ECONNRESET) {
						printf("client[%d] aborted connection\n", i);
						close(sockfd);
						client[i].fd = -1;
					}
					else {
						perror("read error");
						exit(1);
					}
				}
				else if (n == 0) {
					printf("client[%d] closed connection\n", i);
					close(sockfd);
					client[i].fd = -1;
				}
				else {
					for (j = 0; j < n; j++)
						buf[j] = toupper(buf[j]);
					write(sockfd, buf, n);
				}
				if (--nready <= 0)
					break;
			}
		}
	}
	close(sfd);
	return 0;
}



#include <sys/epoll.h>
//多路IO转接服务器  epoll
int main04()
{
	int sfd, i, j, maxi, connfd, sockfd;
	ssize_t n;
	int nready,efd, res;
	char buf[MAXLINE];
	int client[MAX_OPEN];
	struct epoll_event tep, ep[MAX_OPEN];

	sfd = Init_socket(AF_INET, SOCK_STREAM, 0);
	Set_port_multiplex(sfd);
	Make_server(sfd);
	printf("wait for client connect ...\n");

	for (i = 0; i < MAX_OPEN; ++i)
		client[i] = -1;
	maxi = -1;

	efd = epoll_create(MAX_OPEN);   //创建红黑树树根
	if (efd == -1) {
		perror("epoll_create");
		exit(1);
	}

	tep.events = EPOLLIN;
	tep.data.fd = sfd;

	res = epoll_ctl(efd, EPOLL_CTL_ADD, sfd, &tep);
	if (res == -1) {
		perror("epoll_ctl");
		exit(1);
	}

	while (1) {
		nready = epoll_wait(efd, ep, MAX_OPEN, -1);  //阻塞监听
		if (nready == -1) {
			perror("epoll_wait");
			exit(1);
		}

		for (i = 0; i < nready; ++i) {
			if(!(ep[i].events &EPOLLIN))
				continue;
			if (ep[i].data.fd == sfd) {
				connfd = Accept_socket(sfd);

				for (j = 0; j < MAX_OPEN; ++j) {
					if (client[j] < 0) {
						client[j] = connfd;
						break;
					}
				}

				if (j == MAX_OPEN) {
					perror("to many clients");
					exit(1);
				}

				if (j > maxi)
					maxi = j;

				tep.events = EPOLLIN;
				tep.data.fd = connfd;

				res = epoll_ctl(efd, EPOLL_CTL_ADD, connfd, &tep);
				if (res == -1) {
					perror("epoll_ctl");
					exit(1);
				}
			}
			else {
				sockfd = ep[i].data.fd;
				n = read(sockfd, buf, sizeof(buf));
				if (n == 0) {
					for (j = 0; j <= maxi; ++j) {
						if (client[j] == sockfd) {
							client[j] = -1;
							break;
						}
					}
					res = epoll_ctl(efd, EPOLL_CTL_DEL, sockfd, NULL);
					if (res == -1) {
						perror("epoll_ctl");
						exit(1);
					}
					close(sockfd);
					printf("client[%d] closed connection\n", j);
				}
				else {
					for (j = 0; j < n; ++j)
						buf[j] = toupper(buf[j]);
					write(sockfd, buf, n);
				}
			}
		}
	}
	close(sfd);
	close(efd);
	return 0;
}



//基于网络C / S非阻塞模型的epoll ET触发模式
//server.c
#include <fcntl.h>

int main05()
{	
	int sfd,connfd;
	sfd = Init_socket(AF_INET, SOCK_STREAM, 0);
	Set_port_multiplex(sfd);
	Make_server(sfd);
	printf("wait for client connect ...\n");

	struct epoll_event event;
	struct epoll_event resevent[10];
	int res, len;
	int i,efd,flag;
	efd = epoll_create(10);
	event.events = EPOLLIN | EPOLLIN;   //ET边沿触发，默认是水平触发

	connfd = Accept_socket(sfd);

	flag = fcntl(connfd, F_GETFL);
	flag |= O_NONBLOCK;
	fcntl(connfd, F_SETFL, flag);

	event.data.fd = connfd;
	epoll_ctl(efd, EPOLL_CTL_ADD, connfd, &event);
	char buf[1024];

	while (1) {
		printf("epoll_wait begin\n");
		res = epoll_wait(efd, resevent, 10, -1);
		printf("epoll_wait end res %d\n", res);

		if (resevent[0].data.fd = connfd) {
			while ((len = read(connfd, buf, 5)) > 0)
				write(STDOUT_FILENO, buf, len);
		}
	}
	return 0;
}


























//char* html_str = "HTTP/1.1 200 OK\r\n\r\n";
//unsigned char text[1024] = { "hhhhhhhhhhhh" };
//char response[1024] = { 0 };
//sprintf(response, "%s%s%s%s", html_str, "<h1>", text, "<h1>");
//*处理完数据回写给客户端*/
//write(cfd, response, sizeof(response));




