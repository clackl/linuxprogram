#include "csocket.h"
#define SERV_PORT 7890
#define MAX_LISTEN 64
int Init_socket(int domain, int type, int protocol)
{
	int sfd = socket(domain, type, protocol);
	if (sfd < 0){
		perror("socket error");
		exit(1);
	}
}


void Set_port_multiplex(int socket_fd)
{
	int opt = 1;
	setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
}


void Make_server(int socket_fd)
{
	struct sockaddr_in serv_addr;
	/*初始化一个地址结构 man 7 ip 查看对应信息*/
	bzero(&serv_addr, sizeof(serv_addr));           //将整个结构体清零
	serv_addr.sin_family = AF_INET;                 //选择协议族为IPv4
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);  //监听本地所有IP地址
	serv_addr.sin_port = htons(SERV_PORT);          //绑定端口号    

	/*绑定服务器地址结构*/
	int ret = bind(socket_fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
	if (ret < 0) {
		perror("bind error");
		exit(1);
	}
	/*设定链接上限,注意此处不阻塞*/
	listen(socket_fd, MAX_LISTEN);                                //同一时刻允许向服务器发起链接请求的数量
}

int Accept_socket(int socket_fd)
{
	int cfd;
	char clie_IP[BUFSIZ];
	socklen_t clie_addr_len;
	struct sockaddr_in  clie_addr;
	/*获取客户端地址结构大小*/
	clie_addr_len = sizeof(clie_addr_len);
	/*参数1是sfd; 参2传出参数, 参3传入传入参数, 全部是client端的参数*/
	cfd = accept(socket_fd, (struct sockaddr *)&clie_addr, &clie_addr_len);           /*监听客户端链接, 会阻塞*/
	if (cfd == -1) {
		perror("accept err");
		return 0;
	}
	printf("client IP:%s\tport:%d\n",
		inet_ntop(AF_INET, &clie_addr.sin_addr.s_addr, clie_IP, sizeof(clie_IP)),
		ntohs(clie_addr.sin_port));
	return cfd;
}
