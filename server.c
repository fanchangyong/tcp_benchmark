#include <ev.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/uio.h>
#include <pthread.h>
#include <fcntl.h>

int thread_num = 4;
int backlog = SOMAXCONN;
int port = 8888;

void err(char* err)
{
	perror(err);
	exit(1);
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
	else if(n==0)
	{
		printf("EOF\n");
		ev_io_stop(loop,w);
	}
}

static void write_cb(struct ev_loop* loop,ev_io* w,int events)
{
	int fd = w->fd;
	char buf[1024];
	write(fd,buf,1024);
}

static void io_cb(struct ev_loop* loop,ev_io* w,int events)
{
	if(events & EV_WRITE){
		write_cb(loop,w,events);
	}
	if(events & EV_READ){
		read_cb(loop,w,events);
	}
}

static void listen_cb(struct ev_loop* loop,ev_io* w,int events)
{
	int fd = w->fd;
	int clientfd = accept(fd,NULL,NULL);

	ev_io* io_watcher = malloc(sizeof(ev_io));
	ev_io_init(io_watcher,io_cb,clientfd,EV_WRITE|EV_READ);
	ev_io_start(loop,io_watcher);
}

void do_listen(struct ev_loop* loop,int fd)
{
	ev_io* listen_watcher = malloc(sizeof(ev_io));
	ev_io_init(listen_watcher,listen_cb,fd,EV_READ);
	ev_io_start(loop,listen_watcher);
}

void set_nonblock(int fd)
{
	int flags = fcntl(fd,F_GETFL);
	if(flags==-1){
		err("fcntl");
	}
	flags = fcntl(fd,F_SETFL,flags|O_NONBLOCK);
	if(flags==-1){
		err("fcntl");
	}
}

int create_listen_fd()
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

	return fd;
}

void* worker(void* p)
{
	int fd = *(int*)(p);
	struct ev_loop* loop = ev_loop_new(0);
	do_listen(loop,fd);
	ev_run(loop,0);
	printf("worker exiting\n");
	return NULL;
}

int main()
{
	int fd = create_listen_fd();
	pthread_t *threads = malloc(sizeof(pthread_t)*thread_num);
	int i;
	for(i=0;i<thread_num;i++){
		pthread_create(&threads[i],NULL,worker,&fd);
	}

	for(i=0;i<thread_num;i++){
		pthread_join(threads[i],NULL);
	}
	printf("Exiting....\n");

	return 0;
}
