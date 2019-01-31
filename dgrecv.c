#include<stdio.h>
#include<stdlib.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include <netdb.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>

#define oops(m,x)  {perror(m);exit(x);}
#define HOSTLEN 256

int make_dgram_server_socket(int);
int get_internet_address(char *, int, int*, struct sockaddr_in*);
void say_who_called(struct sockaddr_in *);
int make_internet_address(char *, int , struct sockaddr_in *);

int main(int ac, char *av[])
{
	int port;
	int sock;
	char buf[BUFSIZ];          //receive data here
	size_t msglen;             //store its length
	struct sockaddr_in saddr;   //put sender's address
	socklen_t saddrlen;     // sender length


	if (ac == 1 || (port = atoi(av[1])) < 0) {
		fprintf(stderr, "usage: dgrecv portnumber\n");
		exit(1);
	}
	
	if ((sock = make_dgram_server_socket(port)) == -1)
		oops("cannot make socket", 2);

	saddrlen = sizeof(saddr);
	while ((msglen = recvfrom(sock, buf, BUFSIZ, 0, &saddr, &saddrlen)) > 0) {
		buf[msglen] = '\0';
		printf("dgrecv: got a message :%s\n", buf);
		say_who_called(&saddr);
	}
	return 0;
}


void say_who_called(struct sockaddr_in * addrp)
{
	char host[BUFSIZ];
	int port;

	get_internet_address(host, BUFSIZ, &port, addrp);
	printf("from: %s: %d\n", host, port);
}

int make_dgram_server_socket(int portnum)
{
	struct sockaddr_in saddr;  //build our address here
	char hostname[HOSTLEN];

	int sock_id;

	sock_id = socket(PF_INET, SOCK_DGRAM, 0);
	if (sock_id == -1) return -1;

	gethostname(hostname, HOSTLEN);

	make_internet_address(hostname, portnum, &saddr);

	if (bind(sock_id, (struct sockaddr *) &saddr, sizeof(saddr)) != 0)
		return -1;

	return sock_id;
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


int get_internet_address(char *host, int len, int* portp, struct sockaddr_in* addrp)
{
	strncpy(host, inet_ntoa(addrp->sin_addr), len);
	*portp = ntohs(addrp->sin_port);
	return 0;
}