SOURCE = server.c client.c
OBJ = client.o server.o
EXECS = server client

all: server client

server: server.o
		gcc -g -o server server.o

server.o: server.c
			gcc -c server.c


client: client.o
		gcc -g -o client client.o

client.o: client.c
			gcc -c client.c


clean: 
		rm -f ${EXECS} ${OBJ}