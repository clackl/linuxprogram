#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <dirent.h>
#include "socklib.h"

void read_til_crnl(FILE *fp);
void process_rq(char *rq, int fd, char *pastname);
void header(FILE *fp, char *content_type);
void cannot_do(int fd);
void do_404(char *item, int fd);
int isadir(char *f);
int not_exist(char *f);
void do_ls(char *dir, int fd);
char* file_type(char *f);
int ends_in_cgi(char *f);
void do_exec(char *prog, int fd);
void do_cat(char *f, int fd);
void readfilename(char *dir, int fd);

void do_404(char *item, int fd)
{
	FILE *fp = fdopen(fd, "w");
	fprintf(fp, "HTTP/1.0 404 Not Found\r\n");
	fprintf(fp, "Content-type: text/plain\r\n");
	fprintf(fp, "\r\n");
	fprintf(fp, "The item you requested: %s\r\nis not found\r\n", item);
	fclose(fp);
}

int isadir(char *f)
{
	struct stat info;
	return (stat(f, &info) != -1 && S_ISDIR(info.st_mode));
}

int not_exist(char *f)
{
	struct stat info;
	return (stat(f, &info) == -1);
}

void readfilename(char *dir, int fd)
{
	printf("dir: %s\n", dir);
	FILE *fp;
	fp = fdopen(fd, "w");
	header(fp, "text/html");
	fprintf(fp, "\r\n");
	fprintf(fp, "<!DOCTYPE html><html>");
	DIR *dirp;
	struct dirent *dp;

	dirp = opendir(dir); //打开目录指针
	while ((dp = readdir(dirp)) != NULL) { //通过目录指针读目录
		fprintf(fp,"<a href=\"%s\">%s</a><br />",dp->d_name,dp->d_name);
	}
	(void)closedir(dirp); //关闭目录
	fprintf(fp, "</html>");
	fclose(fp);
}

void do_ls(char *dir, int fd)
{
	FILE *fp;
	fp = fdopen(fd, "w");
	header(fp, "text/plain");
	fprintf(fp, "\r\n");
	fflush(fp);

	dup2(fd, 1);
	dup2(fd, 2);
	close(fd);
	execlp("ls", "ls", "-a", dir, NULL);
	perror(dir);
	exit(1);
}

char* file_type(char *f)
{
	char *cp = strrchr(f, '.');
	if (cp != NULL)  
		return cp + 1;
	return "";
}


int ends_in_cgi(char *f)
{
	return(strcmp(file_type(f), "cgi") == 0);
}

void do_exec(char *prog, int fd)
{
	FILE *fp;
	fp = fdopen(fd, "w");
	header(fp, NULL);
	fflush(fp);
	dup2(fd, 1);
	dup2(fd, 2);
	close(fd);
	execl(prog, prog, NULL);
	perror(prog);
}

//
void do_cat(char *f, int fd)
{
	char *extension = file_type(f);
	char *content = "text/plain";
	FILE *fpsock, *fpfile;
	int c;

	if (strcmp(extension, "html") == 0)
		content = "text/html";
	else if (strcmp(extension, "gif") == 0)
		content = "image/gif";
	else if (strcmp(extension, "jpg") == 0)
		content = "image/jpg";
	else if (strcmp(extension, "jpeg") == 0)
		content = "image/jpeg";

	fpsock = fdopen(fd, "w");
	fpfile = fopen(f, "r");

	if (fpsock != NULL && fpfile != NULL) {
		header(fpsock, content);
		fprintf(fpsock, "\r\n");
		while ((c = getc(fpfile)) != EOF)
			putc(c, fpsock);
		fclose(fpfile);
		fclose(fpsock);
	}
	exit(0);
}


void read_til_crnl(FILE *fp)
{
	char buf[BUFSIZ];
	while (fgets(buf, BUFSIZ, fp) != NULL && strcmp(buf, "\r\n") != 0);
}




void process_rq(char *rq, int fd, char *pastname)
{
	char cmd[BUFSIZ], arg[BUFSIZ] = { 0 };
	printf("pastname: %s\n", pastname);
	//create a new process and return if not the child
	if (fork() != 0)
		return;

	strcpy(arg, pastname);     //precede args with

	int ret = sscanf(rq, "%s%s", cmd, arg + strlen(pastname));
	if (ret == NULL)
	{
		printf("%s", "error------------\n");
		return;
	}
	else if (not_exist(arg)) {
		do_404(arg, fd);
	}
	else if (isadir(arg)) {
		readfilename(arg, fd);
	}
	else if (ends_in_cgi(arg)) {
		do_exec(arg, fd);
	}
	else {
		do_cat(arg, fd);
	}
}


//void process_rq(char *rq,int fd,char *pastname)
//{
//	char cmd[BUFSIZ], arg[BUFSIZ] = {0};
//	printf("pastname: %s\n",pastname);
//	//create a new process and return if not the child
//	if (fork() != 0)
//		return;
//	
//	strcpy(arg, pastname);     //precede args with
//
//	int ret = sscanf(rq, "%s%s", cmd, arg + strlen(pastname));
//	if(ret ==NULL)
//	{
//		printf("%s","error------------\n");
//		return;
//	}
//	else if (not_exist(arg)) {
//		do_404(arg, fd);
//	}
//	else if (isadir(arg)) {
//		readfilename(arg, fd);
//	}
//	else if (ends_in_cgi(arg)) {
//		do_exec(arg, fd);
//	}
//	else {
//		do_cat(arg, fd);
//	}
//}

void header(FILE *fp, char *content_type)
{
	fprintf(fp, "HTTP/1.0 200 OK\r\n");
	if (content_type)
		fprintf(fp, "Content-type: %s\r\n", content_type);
}

void cannot_do(int fd)
{
	FILE *fp = fdopen(fd, "w");
	fprintf(fp, "HTTP/1.0 501 Not Implemented\r\n");
	fprintf(fp, "Content-type: text/plain\r\n");
	fprintf(fp, "\r\n");
	fprintf(fp, "That command is not yet implemented\r\n");
	fclose(fp);
}


int main(int ac, char *av[])
{
	int sock, fd;
	FILE *fpin;
	char request[BUFSIZ];

	if (ac == 2) {
		fprintf(stderr, "usage: ws portnum  dir\n");
		exit(1);
	}

	sock = make_server_socket(atoi(av[1]));
	if (sock == -1)  exit(2);


	while (1) {
		fd = accept(sock, NULL, NULL);

		fpin = fdopen(fd, "r");

		fgets(request, BUFSIZ, fpin);
		printf("got a call: request = %s", request);
		read_til_crnl(fpin);

		process_rq(request, fd,av[2]);
		fclose(fpin);
	}
}
