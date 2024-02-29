#include "defs.h"


int clientActive = 1;

void sigint_handler(int sig_num) {
    printf(RED"Server disconnected. Exiting...\n"RESET);
    exit(0);
}

void* getResponses(void* args){
    int* clientSock = (int*) args;
    char response[1024];

    // Set the socket to non-blocking mode
    int flags = fcntl(*clientSock, F_GETFL, 0);
    fcntl(*clientSock, F_SETFL, flags | O_NONBLOCK);

    while(clientActive == 1){
        fd_set set;
        struct timeval timeout;

        // Initialize the file descriptor set
        FD_ZERO(&set);
        FD_SET(*clientSock, &set);

        // Initialize the timeout
        timeout.tv_sec = 1;  // Timeout after 1 second
        timeout.tv_usec = 0;

        // Wait for data to be available on the socket
        int ret = select(*clientSock + 1, &set, NULL, NULL, &timeout);
        if(ret>0){
            ssize_t bytesRead = read(*clientSock, response, sizeof(response) - 1);
            if(bytesRead > 0){
                response[bytesRead] = '\0'; // Null-terminate the received string
                printf(YEL"\nServer Says: %s\n"RESET, response);

                // Check for shutdown command
                if(strcmp(response, "SHUTDOWN") == 0){
                    printf(RED"Server initiated shutdown. Exiting...\n"RESET);
                    clientActive = 0; 
                    close(*clientSock); 
                    exit(0); 
                }
            } else if (bytesRead == 0) {
                printf(RED"\nServer terminated the connection. Disconnecting....\n"RESET);
                clientActive = 0;
                close(*clientSock);
                exit(0);
            } else {
                perror("Read error");
                clientActive = 0;
                close(*clientSock);
                exit(1);
            }
        }else if (ret == 0) {
            // Timeout expired, no data available
            // printf("\nNODAATA AVAILABLE: %d\n", clientActive);
            continue;
        } else {
            // An error occurred
            perror("SELECT ERROR??");
            clientActive = 0;
            break;
        }
    }
}


int main(){
    signal(SIGINT, sigint_handler);
    int clientSock, status;
    struct sockaddr_in serv_addr;
    char* hello = "Hello from client";
    char buffer[1024] = { 0 };
    if((clientSock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("\n Socket creation error \n");
        return -1;
    }
 
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(SERVER_PORT);

    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
        printf("\nInvalid address/ Address not supported \n");
        return -1;
    }

    if ((status = connect(clientSock, (struct sockaddr*)&serv_addr, sizeof(serv_addr))) < 0) {
        printf("\nConnection Failed \n");
        return -1;
    }

    pthread_t responsesThread;
    pthread_create(&responsesThread, NULL, getResponses, &clientSock);

    while(clientActive == 1){
        sleep(1);
        printf("\n ========== MENU ========== \n");
        printf(" Option 1: Request Status \n");
        printf(" Option 2: Print Request \n");
        printf(" Option 3: EXIT\n");
        printf("==========      ========== \n");
        int userInp;
        printf(" \nEnter your option: \n");
        scanf("%d", &userInp);
        while ( getchar() != '\n' );
        switch (userInp){
            case STATUS_REQ:
                send(clientSock, "STATUS", strlen("STATUS"), 0);
                break;
            case PRINT_REQ:
                printf("Enter data to print: \n");
                char printData[PRINT_LIMIT];
                fgets(printData, PRINT_LIMIT, stdin);
                send(clientSock, printData, strlen(printData), 0);
                break;
            case QUIT_CLIENT:
                clientActive = 0;
                break;
            default:
                printf("ERROR: NOT A VALID OPTION!! \n");
                break;
        }
    }
    pthread_join(responsesThread, NULL);
    close(clientSock);
    return 0;
}