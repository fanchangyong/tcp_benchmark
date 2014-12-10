all:server client

server:server.c
	cc server.c -lev -o server
client:client.c
	cc client.c -lev -g -o client

.PHONY:clean

clean:
	rm -f client server
	rm -rf client.dSYM
