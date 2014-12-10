#include <stdio.h>
#include <ev.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>

const char* host="127.0.0.1";
unsigned short port=80;

int rate = 1000; // packet per second
int connections = 800; // tcp connections
int size = 1024; // packet size
int persist_time = 1000; // persist time

void err(char* str)
{
	perror(str);
	exit(1);
}

static void write_cb(struct ev_loop* loop,ev_io* w,int nevents)
{
	//printf("writable\n");
	int fd = w->fd;
	char req_str[]="GET / HTTP/1.1\r\n\r\n";
	int n = write(fd,req_str,sizeof(req_str)+1);
	if(n==0)
	{
		printf("write EOF\n");
	}
	else if(n<0)
	{
		printf("errno:%d\n",errno);
		err("write");
	}
	else
	{
		//printf("written:%d\n",n);
	}
	//printf("has written:%d\n",n);
}

static void read_cb(struct ev_loop* loop,ev_io* w,int nevents)
{
	//printf("readable\n");
	int fd = w->fd;
	char buf[10240];
	int n = read(fd,buf,sizeof(buf));
	if(n==0)
	{
		//printf("EOF\n");
		ev_io_stop(loop,w);
	}
	else if(n<0)
	{
		printf("read errno:%d\n",errno);
		if(errno!=35) err("read");
	}
	else
	{
		printf("read:%s\n",buf);
	}
}

static void watcher_cb(struct ev_loop* loop,ev_io* w,int nevents)
{
	//printf("watcher cb,events:%d:%d:%d:%d\n",w->events,EV_WRITE,EV_READ,nevents);
	if(nevents & EV_WRITE)
	{
		write_cb(loop,w,nevents);
	}
	if(nevents & EV_READ)
	{
		read_cb(loop,w,nevents);
	}
}

static void conn_cb(struct ev_loop* loop,ev_io* w,int nevents)
{
	//printf("connect cb\n");

	int fd = w->fd;

	ev_io* watcher = malloc(sizeof(ev_io));
	ev_io_init(watcher,watcher_cb,fd,EV_WRITE|EV_READ);
	ev_io_start(loop,watcher);


	/*ev_io *write_watcher = malloc(sizeof(ev_io));*/
	/*ev_io_init(write_watcher,write_cb,fd,EV_WRITE);*/
	/*ev_io_start(loop,write_watcher);*/

	/*ev_io *read_watcher = malloc(sizeof(ev_io));*/
	/*ev_io_init(read_watcher,read_cb,fd,EV_READ);*/
	/*ev_io_start(loop,read_watcher);*/

	ev_io_stop(loop,w);
}

void parse_option(int argc,char** argv)
{
	int ch;
	while((ch=getopt(argc,argv,""))!=-1)
	{
		switch(ch)
		{

		}
	}
	if(optind<argc)
	{
		host = strdup(argv[optind++]);
	}
	if(optind<argc)
	{
		port = (unsigned short)atoi(argv[optind++]);
	}
}

void do_connect(struct ev_loop* loop,int connections,
								const char* host,unsigned short port)
{
	int i;
	for(i=0;i<connections;i++)
	{
		int fd = socket(AF_INET,SOCK_STREAM,0);
		if(fd==-1) err("socket");

		struct sockaddr_in addr;
		addr.sin_family = AF_INET;
		inet_aton(host,&addr.sin_addr);
		addr.sin_port = htons(port);

		socklen_t socklen = sizeof(addr);

		int flags = fcntl(fd,F_GETFL);
		if(flags==-1) err("fcntl");
		fcntl(fd,F_SETFL,flags|O_NONBLOCK);

		if(connect(fd,(struct sockaddr*)&addr,socklen)==-1)
		{
			if(errno!=EINPROGRESS) err("connect");
		}
		else
		{
			printf("connected!\n");
		}

		ev_io *conn_watcher = malloc(sizeof(ev_io));
		ev_init(conn_watcher,conn_cb);
		ev_io_set(conn_watcher,fd,EV_WRITE);
		ev_io_start(loop,conn_watcher);
	}

}

int main(int argc,char** argv)
{
	parse_option(argc,argv);

	signal(SIGPIPE,SIG_IGN);
	struct ev_loop* loop = ev_default_loop(0);
	do_connect(loop,connections,host,port);

	ev_run(loop,0);
	return 0;
}
