#include<stdio.h>
#include <curses.h>
#include <signal.h>
#include <aio.h>
#include <sys/time.h>
#include <unistd.h>
#include <string.h>
#define MESSAGE "hello"
#define BLANK "   "

int row = 10;
int col = 0;
int dir = 1;
int delay = 200;
int done = 0;

struct aiocb kbcbuf;   //an aio control buf

int main()
{
	void on_alarm();      //
	void on_input();      //handler for keybd
	void setup_aio_buffer();
	int set_ticker(int);
	initscr();
	crmode();
	noecho();
	clear();

	signal(SIGIO, on_input);  //install a handler   //SIGIO  异步IO信号
	setup_aio_buffer();      //initialize aio ctrl buff
	aio_read(&kbcbuf);       //place a read request

	signal(SIGALRM, on_alarm);
	set_ticker(delay);

	mvaddstr(row, col, MESSAGE);

	//compute_pi();
	while (!done)
		pause();
	endwin();
}

void on_input()
{
	int c;
	char *cp = (char*)kbcbuf.aio_buf;  //cast to char

	if (aio_error(&kbcbuf) != 0)
		perror("reading failed");
	else
		if (aio_return(&kbcbuf) == 1) {
			c = *cp;
			if (c == 'Q' || c == EOF)
				done = 1;
			else if (c == ' ')
				dir = -dir;
		}
	aio_read(&kbcbuf);
}


void on_alarm()
{
	mvaddstr(row, col, BLANK);
	col += dir;
	mvaddstr(row, col, MESSAGE);
	refresh();

	if (dir == -1 && col <= 0)
		dir = 1;
	else if (dir == 1 && col + strlen(MESSAGE) >= COLS)
		dir = -1;
}

void setup_aio_buffer()
{
	static char input[1];
	kbcbuf.aio_fildes = 0;   //standard  input
	kbcbuf.aio_buf = input;  //buffer
	kbcbuf.aio_nbytes = 1;   //number to read
	kbcbuf.aio_offset = 0;   //offset in file

	kbcbuf.aio_sigevent.sigev_notify = SIGEV_SIGNAL;
	kbcbuf.aio_sigevent.sigev_signo = SIGIO;  //send sIGIO
}



int set_ticker(int n_msecs)
{
	struct itimerval new_timeset;
	long n_sec, n_usecs;

	n_sec = n_msecs / 1000;
	n_usecs = (n_msecs % 1000) * 1000L;

	new_timeset.it_interval.tv_sec = n_sec;
	new_timeset.it_interval.tv_usec = n_usecs;

	new_timeset.it_value.tv_sec = n_sec;
	new_timeset.it_value.tv_usec = n_usecs;
	return setitimer(ITIMER_REAL, &new_timeset, NULL);
}