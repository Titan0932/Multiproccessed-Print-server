#include "defs.h"


int clientActive = 1;

void sigint_handler(int sig_num) {
    printf("Server disconnected. Exiting...\n");
    exit(0);
}

void* getResponses(void* args){
    int* clientSock = (int*) args;
    char response[1024];

    while(clientActive == 1){
        ssize_t bytesRead = read(*clientSock, response, sizeof(response) - 1);
        if(bytesRead > 0){
            response[bytesRead] = '\0'; // Null-terminate the received string
            printf("\nServer Says: %s\n", response);

            // Check for shutdown command
            if(strcmp(response, "SHUTDOWN") == 0){
                printf("Server initiated shutdown. Exiting...\n");
                clientActive = 0; 
                close(*clientSock); 
                exit(0); 
            }
        } else if (bytesRead == 0) {
            printf("\nServer terminated the connection. Disconnecting....\n");
            clientActive = 0;
            close(*clientSock);
            exit(0);
        } else {
            perror("Read error");
            clientActive = 0;
            close(*clientSock);
            exit(1);
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
                scanf("%s", printData);
                while ( getchar() != '\n' );
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