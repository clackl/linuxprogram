#pragma once
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
#include <sys/select.h>
#include <sys/time.h>
//创建socket套接字
int Init_socket(int domain, int type, int protocol);
//端口复用
void Set_port_multiplex(int socket_fd);
//创建服务器
void Make_server(int socket_fd);
//接收客户端连接
int Accept_socket(int socket_fd);

