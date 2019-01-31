#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <string.h>

#define oops(m,x) {perror(m);exit(x);}

int make_dgram_clinet_socket();
int make_internet_address(char *, int, struct sockaddr_in *);
void say_who_called(struct sockaddr_in *);
int main(int ac, char *av[])
{
	int sock;    //use this socket to send
	char msg[BUFSIZ];   //send this message
	struct sockaddr_in saddr;   //put sender's address here

	if (ac != 3) {
		fprintf(stderr, "usage: dgsend host port 'message'\n");
		exit(1);
	}

	if ((sock = make_dgram_clinet_socket()) == -1)
		oops("cannot make socket", 2);

	make_internet_address(av[1], atoi(av[2]), &saddr);
	
	size_t msglen;
	socklen_t saddrlen;
	struct sockaddr_in saddr_recv;
	while (1) {
		printf("please input str:\n");
		fgets(msg, BUFSIZ, stdin);
		if (strcmp(msg, "q") == 0)
			break;
		if (sendto(sock, msg, strlen(msg), 0, &saddr, sizeof(saddr)) == -1)
			oops("sendto failed", 3);
		bzero(msg, 20);
		if ((msglen = recvfrom(sock, msg, BUFSIZ, 0, &saddr_recv, &saddrlen)) > 0) {
			msg[msglen] = '\0';
			printf("recv message: %s\n", msg);
			say_who_called(&saddr_recv);
		}
	}
	return 0;
}

int make_dgram_clinet_socket()
{
	return socket(PF_INET, SOCK_DGRAM, 0);
}

int make_internet_address(char *hostname, int port, struct sockaddr_in *addrp)
{
	struct hostent *hp;
	bzero((void*)addrp, sizeof(struct sockaddr_in));

	hp = gethostbyname(hostname);
	if (hp == NULL) return -1;

	bcopy((void *)hp->h_addr_list[0], (void*)&addrp->sin_addr, hp->h_length);
	addrp->sin_port = htons(port);
	addrp->sin_family = AF_INET;
	return 0;
}

void say_who_called(struct sockaddr_in * addrp)
{
	char host[BUFSIZ];
	int port;
	strncpy(host, inet_ntoa(addrp->sin_addr), BUFSIZ);
	port = ntohs(addrp->sin_port);
	printf("from: %s: %d\n", host, port);
}