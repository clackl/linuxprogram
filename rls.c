#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#define oops(msg)  {perror(msg);exit(1);}

#define PORTNUM    15000

int main(int ac,char *av[])
{
	struct sockaddr_in servadd;   //the number to call
	struct hostent *hp;       //used to get number
	int sock_id, sock_fd;   //the socket and fd
	char buffer[BUFSIZ];   //to receive message
	int n_read;


	if (ac != 3) exit(1);

	sock_id = socket(AF_INET, SOCK_STREAM, 0);
	if (sock_id == -1)
		oops("socket");


	bzero(&servadd, sizeof(servadd));
	hp = gethostbyname(av[1]);

	if (hp == NULL)
		oops(av[1]);

	bcopy(hp->h_addr_list[0], (struct sockaddr *)&servadd.sin_addr, hp->h_length);
	servadd.sin_port = htons(PORTNUM);
	servadd.sin_family = AF_INET;

	if (connect(sock_id, (struct sockaddr*)&servadd, sizeof(servadd)) != 0)
		oops("connect");

	if (write(sock_id, av[2], strlen(av[2])) == -1)
		oops("write1");

	if (write(sock_id, "\n", 1) == -1)
		oops("write2");

	while ((n_read = read(sock_id, buffer, BUFSIZ)) > 0)
		if (write(1, buffer, n_read) == -1)
			oops("write3");

	close(sock_id);


}