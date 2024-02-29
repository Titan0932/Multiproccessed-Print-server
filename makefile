SOURCE = server.c client.c printer.c
OBJ = client.o server.o printer.o
EXECS = server client printer

all: server client printer

server: server.o
		gcc -g -o server server.o

server.o: server.c
			gcc -c server.c


client: client.o
		gcc -g -o client client.o

client.o: client.c
			gcc -c client.c

printer: printer.o
			gcc -g -o printer printer.o

printer.o: printer.c
			gcc -c printer.c


clean: 
		rm -f ${EXECS} ${OBJ}