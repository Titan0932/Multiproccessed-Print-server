#include <pthread.h>
#include <string.h>

#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <unistd.h>
#include <wait.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <errno.h>
#include <signal.h>
#include <fcntl.h>

#define MAX_ACTIVE_REQS 2
#define SERVER_PORT 8000
#define PRINTER_PORT 9000

#define SESSION_TIMEOUT 15     // timeout for each session

#define STATUS_REQ 1
#define PRINT_REQ 2
#define QUIT_CLIENT 3
#define PRINT_LIMIT 1024
#define SUCCESS 1
#define FAILURE -1

#define RED "\x1B[31m"
#define GRN "\x1B[32m"
#define YEL "\x1B[33m"
#define BLU "\x1B[34m"
#define MAG "\x1B[35m"
#define CYN "\x1B[36m"
#define WHT "\x1B[37m"
#define RESET "\x1B[0m"

typedef struct{
    char reqType[30];
    int clientSock;
    char printData[PRINT_LIMIT];
    time_t sessionStart;  // stores timestamp in seconds
}Request;

const char SERVEROVERLOAD[50] = "SERVER TOO BUSY! TRY AGAIN LATER";

typedef struct {
    int clientSock;
    char document[PRINT_LIMIT];
} PrintRequest;

typedef struct{
    int clientSock;
    int serverSock;
}socks;