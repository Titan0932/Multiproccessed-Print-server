#include "defs.h"


int clientActive = 1;


// void* getResponses(void* args){
//     int* clientSock = (int*) args; 
//     char response[50];
//     while(clientActive){
//         ssize_t bytesRead = read(*clientSock, response, sizeof(response) - 1);
//         if(bytesRead > 0){
//             printf("\nServer Says: %s\n", response);
//         }else if(bytesRead == 0){
//             printf("\nConnection closed by server: Disconnecting....\n");
//             clientActive = 0;
//             break;
//         }else{
//             perror("READ ERROR??");
//             clientActive = 0;
//             break;
//         }
//     }
// }

void* getResponses(void* args){
    int* clientSock = (int*) args; 

    // Set the socket to non-blocking mode
    int flags = fcntl(*clientSock, F_GETFL, 0);
    fcntl(*clientSock, F_SETFL, flags | O_NONBLOCK);

    char response[50];
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

        if (ret > 0) {
            ssize_t bytesRead = read(*clientSock, response, sizeof(response) - 1);
            if(bytesRead > 0){
                printf("\nServer Says: %s\n", response);
            }else if(bytesRead == 0){
                printf("\nDisconnecting....\n");
                clientActive = 0;
                printf("\nUpdated val: %d\n", clientActive);
                break;
            }else{
                perror("READ ERROR??");
                clientActive = 0;
                break;
            }
        } else if (ret == 0) {
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
        printf("LOOP VAL: %d", clientActive);
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