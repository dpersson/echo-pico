#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <netinet/in.h>
#include <unistd.h>
#include <netinet/tcp.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/uio.h>

#include "picoev.h"

#define PORT_NO 3033
#define BUFFER_SIZE 1024
#define MAX_FDS 1024
#define TIMEOUT_SECS 10

static void close_conn(picoev_loop* loop, int fd);
static void read_cb(picoev_loop* loop, int fd, int events, void* cb_arg);
static void accept_cb(picoev_loop* loop, int fd, int events, void* cb_arg);

int main()
{
	picoev_loop* loop;
	int sd, flag, on = 1;
	struct sockaddr_in listen_addr;
  
	/* listen to port */

	if((sd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{
		perror("socket error");
		return -1;
	}

	flag = 1;
	if(setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(flag)) < 0)
	{
		perror("setsockopt error");
		return -1;
	}

	listen_addr.sin_family = AF_INET;
	listen_addr.sin_port = htons(PORT_NO);
	listen_addr.sin_addr.s_addr = INADDR_ANY;

	if(bind(sd, (struct sockaddr*)&listen_addr, sizeof(listen_addr)) < 0)
	{
		perror("bind error");
		return -1;
	}

	if(listen(sd, 5) < 0)
	{
		perror("listen error");
		return -1;
	}

	if(setsockopt(sd, IPPROTO_TCP, TCP_NODELAY, &on, sizeof(on)) < 0)
  	{
		perror("setsockopt error");
		return -1;
  	}
  	
  	if(fcntl(sd, F_SETFL, O_NONBLOCK) < 0)
  	{
  		perror("fcntl error");
		return -1;
  	}

	picoev_init(MAX_FDS);
	loop = picoev_create_loop(60);
	picoev_add(loop, sd, PICOEV_READ, 0, accept_cb, NULL);

	while (1) {
		fputc('.', stdout); fflush(stdout);
		picoev_loop_once(loop, 10);
	}

	picoev_destroy_loop(loop);
	picoev_deinit();

	return 0;
}

static void close_conn(picoev_loop* loop, int fd)
{
	picoev_del(loop, fd);
  	close(fd);
	printf("closed: %d\n", fd);
}

static void read_cb(picoev_loop* loop, int fd, int events, void* cb_arg)
{
	if ((events & PICOEV_TIMEOUT) != 0)
	{
    	close_conn(loop, fd);
  	}
  	else if ((events & PICOEV_READ) != 0)
  	{
	    /* update timeout, and read */
	    char buf[BUFFER_SIZE];
	    ssize_t r;
	    picoev_set_timeout(loop, fd, TIMEOUT_SECS);
	    r = read(fd, buf, sizeof(buf));
	    switch (r)
	    {
	    	case 0: /* connection closed by peer */
	      		close_conn(loop, fd);
	      		break;
	    	case -1: /* error */
	      		if (errno == EAGAIN || errno == EWOULDBLOCK)
	      		{ /* try again later */
					break;
	      		}
	      		else
	      		{ /* fatal error */
					close_conn(loop, fd);
	      		}
	      		break;
	    	default: /* got some data, send back */
	      		if (write(fd, buf, r) != r)
	      		{
					close_conn(loop, fd); /* failed to send all data at once, close */
	      		}
	      		break;
	    }
  	}
}

static void accept_cb(picoev_loop* loop, int fd, int events, void* cb_arg)
{
	int newfd = accept(fd, NULL, NULL);
	int on = 1;

	if (newfd != -1)
	{
		printf("connected: %d\n", newfd);
		if(setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &on, sizeof(on)) < 0)
	  	{
			perror("setsockopt error");
			return;
	  	}
	  	
	  	if(fcntl(fd, F_SETFL, O_NONBLOCK) < 0)
	  	{
	  		perror("fcntl error");
			return;
	  	}
		picoev_add(loop, newfd, PICOEV_READ, TIMEOUT_SECS, read_cb, NULL);
	}
}
