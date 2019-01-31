#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <time.h>
#include <strings.h>

#define PORTNUM 13000
#define HOSTLEN 128
#define oops(msg)  {perror(msg); exit(1);}

int main()
{
	struct sockaddr_in saddr;   //build our address here
	struct hostent *hp;        //this is part of our
	

	char hostname[HOSTLEN];     //address
	int sock_id, sock_fd;       //line id,file desc
	FILE *sock_fp;              //use socket as stream
	char *ctime();              //将秒转换为字符串
	time_t thetime;             //the time we report


	//ask kernel for a scoket
	sock_id = socket(PF_INET, SOCK_STREAM, 0);   //get a socket
	if (sock_id == -1)
		oops("socket");

	//bing address to socket address is host,port
	bzero((void*)&saddr, sizeof(saddr));   //clear out struct
	gethostname(hostname, HOSTLEN);          
	hp = gethostbyname(hostname);            //get info about host  

	bcopy((void*)hp->h_addr_list[0],(void *)&saddr.sin_addr.s_addr, hp->h_length);   //fill in host part

	saddr.sin_port = htons(PORTNUM);    //fill in socket port
	saddr.sin_family = AF_INET;         //fill in addr family

	if (bind(sock_id, (struct sockaddr *)&saddr, sizeof(saddr)) != 0)
		oops("bind");

	if (listen(sock_id, 1) != 0)
		oops("listen");

	int opt = 1;
	setsockopt(sock_id, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

	while (1) {
		sock_fd = accept(sock_id, NULL, NULL); 
		printf("Wow! got a  call\n");
		if (sock_fd == -1)
			oops("accept");

		sock_fp = fdopen(sock_fd, "w");
		if (sock_fp == NULL)
			oops("fdopen");

		thetime = time(NULL);

		fprintf(sock_fp, "The time here is ..");
		fprintf(sock_fp, ctime(&thetime));
		fclose(sock_fp);

	}

}
