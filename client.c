#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#define  MAXLINE 80
#define SERV_PORT 7890

int main()
{
	struct sockaddr_in servaddr;
	socklen_t cliaddr_len;
	int sockfd;
	char buf[MAXLINE];
	int n,ret;
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd == -1) {
		perror("socket err");
		return 0;
	}
	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(SERV_PORT);
	inet_pton(AF_INET, "192.168.1.7", &servaddr.sin_addr.s_addr);

	ret = connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr));
	if (ret != 0) {
		perror("connect err");
		return 0;
	}
	
	while(1){
		fgets(buf, sizeof(buf), stdin);
	    ret = write(sockfd, buf, strlen(buf));
		if (ret == -1) {
			perror("write err");
			return 0;
		}
		n = read(sockfd, buf, sizeof(buf));
		ret = write(STDOUT_FILENO, buf, n);
		if (ret == -1) {
			perror("write err");
			return 0;
		}
	}
	close(sockfd);
	return 0;
}