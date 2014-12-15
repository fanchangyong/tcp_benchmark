all:server client

server:server.c
	cc server.c -lev -lpthread -o server
client:client.c
	cc client.c -lev -lpthread -g -o client

.PHONY:clean

clean:
	rm -f client server
	rm -rf client.dSYM
