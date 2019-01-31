#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include "dgram.h"
static int pid = -1;                 //Our PID
static int sd = -1;                  //Our communications socket
static struct sockaddr serv_addr;    //server address
static socklen_t serv_alen;          //length of address
static char ticket_buf[128];         //Buffer to hold our ticket
static have_ticket = 0;              //set when we have a ticket

#define MSGLEN 128                   //size of our datagrams
#define SERVER_PORTNUM 7890          //our server's port number
#define HOSTLEN 512
#define oops(p) {perror(p);exit(1);}


char *do_transaction(char *);
void setup();
void shut_down();
int get_ticket();
int release_ticket();
void syserr(char *msg1);
void narrate(char *msg1, char *msg2);


void setup()
{
	char hostname[BUFSIZ];
	pid = getpid();
	sd = make_dgram_clinet_socket();
	if (sd == -1)
		oops("cannot create socket");
	gethostname(hostname, HOSTLEN);    //server on same host
	make_internet_address(hostname, SERVER_PORTNUM, &serv_addr);
	serv_alen = sizeof(serv_addr);
}

void shut_down()
{
	close(sd);
}

int get_ticket()
{
	char *response;
	char buf[MSGLEN];

	if (have_ticket)    //don't be greedy
		return 0;

	sprintf(buf, "HELO %d", pid);

	if ((response = do_transaction(buf)) == NULL)
		return -1;

	if (strncmp(response, "TICK", 4) == 0) {
		strcpy(ticket_buf, response + 5);    //grab ticket -id
		have_ticket = 1;
		narrate("got ticket", ticket_buf);
		return 0;
	}

	if (strncmp(response, "FAIL", 4) == 0)
		narrate("Could not get ticket", response);
	else
		narrate("unkonwn message:", response);
	return -1;
}

int release_ticket()
{
	char buf[MSGLEN];
	char *response;

	if (!have_ticket)
		return 0;
	sprintf(buf, "GBYE %s", ticket_buf);

	if ((response = do_transaction(buf)) == NULL)
		return -1;

	if (strncmp(response, "THNX", 4) == 0) {
		narrate("released ticket OK", "");
		return 0;
	}
	if (strncmp(response, "FAIL", 4) == 0)
		narrate("Could not get ticket", response);
	else
		narrate("unkonwn message:", response);
	return -1;

}

char *do_transaction(char *msg)
{
	static char buf[MSGLEN];
	struct sockaddr retaddr;
	socklen_t addrlen = sizeof(retaddr);
	int ret;

	ret = sendto(sd, msg, strlen(msg), 0, &serv_addr, serv_alen);
	if (ret == -1) {
		syserr("sendto");
		return NULL;
	}

	ret = recvfrom(sd, buf, MSGLEN, 0, &retaddr, &addrlen);
	if (ret == -1) {
		syserr("recvfrom");
		return NULL;
	}

	return buf;

}

void narrate(char *msg1, char *msg2)
{
	fprintf(stderr, "CLIENT[%d]: %s %s\n", pid, msg1, msg2);
}

void syserr(char *msg1)
{
	char buf[MSGLEN];
	sprintf(buf, "CLIENT[%d]: %s", pid, msg1);
	perror(buf);
}


void do_regular_work()
{
	printf("SuperSleep version 1.0 Running - Licensed Software\n");
	sleep(10);
}

int main(int ac, char *av[])
{
	setup();
	if (get_ticket() != 0)
		exit(1);
	do_regular_work();
	release_ticket();
	shut_down();
}