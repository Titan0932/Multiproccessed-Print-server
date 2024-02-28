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

#define SESSION_TIMEOUT 15     // timeout for each session

#define STATUS_REQ 1
#define PRINT_REQ 2
#define QUIT_CLIENT 3
#define PRINT_LIMIT 50

typedef struct{
    char reqType[20];
    int clientSock;
    char printData[PRINT_LIMIT];
    time_t sessionStart;  // stores timestamp in seconds
}Request;

const char SERVEROVERLOAD[50] = "SERVER TOO BUSY! TRY AGAIN LATER";