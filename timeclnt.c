#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>

#define oops(msg)   {perror(msg);exit(1);}


int main(int ac,char *av[])
{
	struct sockaddr_in servadd;
	struct hostent *hp;
	int sock_id, sock_fd;
	char message[BUFSIZ];
	int messlen;

	//get a socket

	sock_id = socket(AF_INET, SOCK_STREAM, 0);
	if (sock_id == -1)
		oops("socket");


	//connecct to server
	bzero(&servadd, sizeof(servadd));

	hp = gethostbyname(av[1]);   //lookup hosts ip
	if (hp == NULL)
		oops(av[1]);


	bcopy(hp->h_addr_list[0], (struct sockaddr *)&servadd.sin_addr, hp->h_length);

	servadd.sin_port = htons(atoi(av[2]));   //find in port number

	servadd.sin_family = AF_INET;   //fill in socket type

	if (connect(sock_id, (struct sockaddr *)&servadd, sizeof(servadd)) != 0)
		oops("connetc");


	//transfer data from server ,then hangup
	messlen = read(sock_id, message, BUFSIZ);
	if (messlen == -1)
		oops("read");
	if (write(1, message, messlen) != messlen)
		oops("write");

	close(sock_id);



}
