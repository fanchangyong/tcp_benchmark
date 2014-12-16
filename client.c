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
#include <pthread.h>

const char* host="127.0.0.1";
unsigned short port=8888;

int rate = 1000; // packet per second
int connections = 800; // tcp connections
int packet_size = 1024; // packet size
int run_time = 5; // runnint time
int threads = 1; // threads

void do_connect(struct ev_loop* loop,int connections,
								const char* host,unsigned short port);

void err(char* str)
{
	printf("errno:%d\n",errno);
	perror(str);
	abort();
}

static void write_cb(struct ev_loop* loop,ev_io* w,int nevents)
{
	//printf("writable\n");
	int fd = w->fd;
	char* buf = malloc(packet_size);

	for(;;){
		ssize_t n = write(fd,buf,packet_size);
		if(n==0){
			printf("write EOF\n");
		}else if(n<0){
			//printf("errno:%d\n",errno);
			if(errno==EAGAIN || errno==EWOULDBLOCK){
				break;
			}else{
				close(fd);
				ev_io_stop(loop,w);
				do_connect(loop,1,host,port);
			}
		}else{
			//printf("written:%ld\n",n);
		}
	}
}

static void read_cb(struct ev_loop* loop,ev_io* w,int nevents)
{
	//printf("readable\n");
	int fd = w->fd;
	static char buf[10240];
	for(;;){
		int n = read(fd,buf,sizeof(buf));
		if(n==0){
			ev_io_stop(loop,w);
		}else if(n<0){
			if(errno==EAGAIN || errno==EWOULDBLOCK){
				break;
			}else{
				err("read");
			}
		}else
		{
			printf("read:%s\n",buf);
		}
	}
}

static void io_cb(struct ev_loop* loop,ev_io* w,int nevents)
{
	//printf("watcher cb,events:%d:%d:%d:%d\n",w->events,EV_WRITE,EV_READ,nevents);
	if(nevents & EV_WRITE){
		write_cb(loop,w,nevents);
	}
	if(nevents & EV_READ){
		read_cb(loop,w,nevents);
	}
}

static void conn_cb(struct ev_loop* loop,ev_io* w,int nevents)
{
	int fd = w->fd;
	printf("conn cb:%d,thread_id:%ld\n",fd,pthread_self());

	struct sockaddr_in addr;
	addr.sin_family = AF_INET;
	inet_aton(host,&addr.sin_addr);
	addr.sin_port = htons(port);
	socklen_t socklen = sizeof(struct sockaddr_in);
	if(connect(fd,(struct sockaddr*)&addr,socklen)==-1){
		printf("errno:%d,fd:%d\n",errno,fd);
		err("connect2");
	}
	printf("conn cb connected:%d\n",fd);
	ev_io_stop(loop,w);

	ev_io *watcher = malloc(sizeof(ev_io));
	ev_io_init(watcher,io_cb,fd,EV_WRITE|EV_READ);
	ev_io_start(loop,watcher);
}

static void print_options()
{
	printf("Connections: %d\n",connections);
	printf("Rate: %d\n",rate);
	printf("Packet size: %d\n",packet_size);
	printf("Run time: %d\n",run_time);
}

void parse_option(int argc,char** argv)
{
	int ch;
	while((ch=getopt(argc,argv,"c:r:b:t:l:"))!=-1)
	{
		switch(ch)
		{
			case 'c':
			{
				connections = atoi(optarg);
				break;
			}
			case 'r':
			{
				rate = atoi(optarg);
				break;
			}
			case 'b':
			{
				packet_size = atoi(optarg);
				break;
			}
			case 't':
			{
				run_time = atoi(optarg);
				break;
			}
			case 'l':
			{
				threads = atoi(optarg);
				break;
			}
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
	for(i=0;i<connections;i++){
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

		if(connect(fd,(struct sockaddr*)&addr,socklen)==-1){
			if(errno!=EINPROGRESS) {
				err("connect");
			}
			//printf("inprogress:%d\n",fd);
			ev_io* watcher = malloc(sizeof(ev_io));
			ev_io_init(watcher,conn_cb,fd,EV_WRITE);
			ev_io_start(loop,watcher);
		}else{
			ev_io *watcher = malloc(sizeof(ev_io));
			ev_io_init(watcher,io_cb,fd,EV_WRITE|EV_READ);
			ev_io_start(loop,watcher);
		}
	}

}

void timeout_cb(struct ev_loop* loop,ev_timer* w,int nevents)
{
	printf("timeout break\n");
	ev_break(EV_A_ EVBREAK_ALL);
}

void* worker(void* p)
{
	struct ev_loop* loop = ev_loop_new(0);
	do_connect(loop,connections,host,port);
	ev_run(loop,0);
}

int main(int argc,char** argv)
{
	parse_option(argc,argv);
	print_options();

	signal(SIGPIPE,SIG_IGN);

	int i;
	for(i=0;i<threads;i++){
		pthread_t t;
		pthread_create(&t,NULL,worker,NULL);
	}

	struct ev_loop* loop = ev_default_loop(0);

	ev_timer timeout_watcher;
	ev_timer_init(&timeout_watcher,timeout_cb,run_time,0);
	ev_timer_start(loop,&timeout_watcher);
	ev_run(loop,0);

	return 0;
}
