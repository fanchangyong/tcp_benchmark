#include <ev.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/uio.h>

int backlog = 1;
int port = 8888;

void err(char* err)
{
	perror(err);
}

static void read_cb(struct ev_loop* loop,ev_io* w,int events)
{
	int fd = w->fd;
	char buf[1024];
	int n = read(fd,buf,sizeof(buf));
	if(n<0)
	{
		perror("read");
	}

}

static void write_cb(struct ev_loop* loop,ev_io* w,int events)
{
	int fd = w->fd;
}

static void listen_cb(struct ev_loop* loop,ev_io* w,int events)
{
	int fd = w->fd;
	int clientfd = accept(fd,NULL,NULL);

	ev_io* write_watcher = malloc(sizeof(ev_io));
	ev_io_init(write_watcher,write_cb,clientfd,EV_WRITE);
	ev_io_start(loop,write_watcher);

	ev_io* read_watcher = malloc(sizeof(ev_io));
	ev_io_init(read_watcher,read_cb,clientfd,EV_READ);
	ev_io_start(loop,read_watcher);
}

void do_listen(struct ev_loop* loop)
{
	int fd = socket(AF_INET,SOCK_STREAM,0);
	if(fd==-1) err("socket");

	struct sockaddr_in listen_addr;
	listen_addr.sin_family = AF_INET;
	listen_addr.sin_addr.s_addr = INADDR_ANY;
	listen_addr.sin_port = htons(port);

	socklen_t len = sizeof(struct sockaddr_in);
	if(bind(fd,(struct sockaddr*)&listen_addr,len)
			==-1) err("bind");

	if(listen(fd,backlog)==-1) err("listen");

	ev_io* listen_watcher = malloc(sizeof(ev_io));
	ev_io_init(listen_watcher,listen_cb,fd,EV_READ);
	ev_io_start(loop,listen_watcher);

}

int main()
{
	struct ev_loop *loop = ev_default_loop(0);
	do_listen(loop);

	ev_run(loop,0);
	return 0;
}
