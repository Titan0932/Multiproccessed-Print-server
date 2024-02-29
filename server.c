#include "defs.h"

int processActive = 1;

void cleanupMem(Request** request){
    // free request memory
    free(*request);
    *request=NULL;
}

int listen_for_requests(int* serverfd, int* clientSocket, struct sockaddr_in* address, socklen_t* addrlen, Request** request){
    if ((*clientSocket = accept(*serverfd, (struct sockaddr *)address, addrlen)) < 0) {
        perror("Server Error: accept");
        return(EXIT_FAILURE);
    }
    (*request)->clientSock = *clientSocket;
    (*request)->sessionStart = time(NULL);
    return EXIT_SUCCESS;
}

void* printerHandler(void* args){
    socks* printer_client_sock = (socks*) args;
    char printerResponse[PRINT_LIMIT];
    while(processActive){
         ssize_t bytesRead = recv(printer_client_sock->serverSock, printerResponse, sizeof(printerResponse) - 1, 0);
         if(bytesRead > 0){
            send(printer_client_sock->clientSock, printerResponse, sizeof(printerResponse),0);
         }else if(bytesRead == 0){
            processActive = 0;
            break;
         }else{
            perror("Error recv in handling printer response!!!");
            processActive = 0;
            break;
         }
    }
}

void handle_request(Request** request) {
    int printerSock, status;
    struct sockaddr_in printer_addr;

    if((printerSock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf(RED"\n Socket creation error \n"RESET);
        return;
    }
 
    printer_addr.sin_family = AF_INET;
    printer_addr.sin_port = htons(PRINTER_PORT);

    if (inet_pton(AF_INET, "127.0.0.1", &printer_addr.sin_addr) <= 0) {
        printf(RED"\nPrinter: Invalid address/ Address not supported \n"RESET);
        return;
    }

    if ((status = connect(printerSock, (struct sockaddr*)&printer_addr, sizeof(printer_addr))) < 0) {
        printf("\nPrinter: Connection Failed \n");
        send((*request)->clientSock, "PRINTER CONNECTION FAILED!", 27, 0);
        return;
    }

    socks print_client_socks;
    print_client_socks.clientSock= (*request)->clientSock;
    print_client_socks.serverSock= printerSock;
    pthread_t printerHandler_t;
    pthread_create(&printerHandler_t, NULL, printerHandler, &print_client_socks);

    // Set the file descriptor to non-blocking mode
    int flags = fcntl((*request)->clientSock, F_GETFL, 0);
    fcntl((*request)->clientSock, F_SETFL, flags | O_NONBLOCK);

    while(processActive == 1){
        ssize_t bytesRead = read((*request)->clientSock, (*request)->reqType, sizeof((*request)->reqType) - 1);
       
        if (bytesRead == -1 && errno == EAGAIN) {
            continue; // Skip to the next iteration
        }

        if (bytesRead == 0) {
            processActive = 0;
            break;
        }

        printf(CYN"HANDLING REQUEST!!\n");
        printf("Client: %d | Message: %s\n"RESET, (*request)->clientSock, (*request)->reqType);
        (*request)->reqType[bytesRead] = '\0';
        
        if(strcmp((*request)->reqType, "STATUS") == 0 ){
            send(printerSock, "STATUS", strlen("STATUS") + 1, 0);
        }else{
            send(printerSock, (*request)->reqType, strlen((*request)->reqType) + 1, 0);
        }
    }

    // After finishing the communication or if an error occurred
    // Sendd SHUTDOWN message to the client
    const char* shutdownMessage = "SHUTDOWN";
    send((*request)->clientSock, shutdownMessage, strlen(shutdownMessage), 0);

    close((*request)->clientSock);
    printf(RED"Sent SHUTDOWN message and closed the client socket.\n"RESET);

    close(printerSock);

    // Exiting the thread or child process
    pthread_cancel(printerHandler_t); 
}

// if a child has terminated, remove counter of active requests
void* reap_children(void* arg) {
    int* activeReqs = (int*) arg;
    while(1){
        while (waitpid(-1, NULL, WNOHANG) > 0) {  // check if child processes have terminated.No hang if no change in states.
            (*activeReqs)--;
            printf(MAG"UPDATED: active requests: %d\n"RESET, *activeReqs);
        }
    }
}

// Server Process
int main() {
    int activeReqs = 0;
    int serverfd, clientSocket;
    struct sockaddr_in address;
    int opt = 1;
    socklen_t addrlen = sizeof(address);
    if((serverfd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
        perror("Server failed to initialize!");
        exit(EXIT_FAILURE);
    }
    if (setsockopt(serverfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(SERVER_PORT);
    if (bind(serverfd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        perror("Server Error: bind failed");
        exit(EXIT_FAILURE);
    }

    printf(GRN"Server successfully initialized!\n"RESET);
    
    if (listen(serverfd, 3) < 0) {
        perror("Server Error: listen");
        return(EXIT_FAILURE);
    }

    pthread_t reaper_thread;
    pthread_create(&reaper_thread, NULL, reap_children, &activeReqs);

    while (1) {
        // Listen for print requests
        Request* newRequest = (Request*) calloc(1, sizeof(Request));
        if(newRequest == NULL){
            perror("Server Error: Memory allocation error -> request");
            continue;
        }
        int requestStatus = listen_for_requests(&serverfd, &clientSocket, &address, &addrlen, &newRequest);
        if (requestStatus == EXIT_SUCCESS) {
            if(activeReqs == MAX_ACTIVE_REQS){
                // send response to client that server full and to wait
                // TODO
                printf(RED"SERVER TOO BUSY! \n"RESET);
                send(clientSocket, SERVEROVERLOAD, strlen(SERVEROVERLOAD),0);
                close(clientSocket);
                cleanupMem(&newRequest);
            }else{
                // Create a new child process to handle the request
                ++activeReqs;
                printf(MAG"NEW ACTIVE reqs: %d\n"RESET, activeReqs);
                pid_t pid = fork();

                if (pid == 0) {
                    // This is the child process
                    handle_request(&newRequest);
                    cleanupMem(&newRequest);
                    close(clientSocket);
                    printf(RED"CLOSING CLIENT! BYEEE \n"RESET);
                    exit(EXIT_SUCCESS);
                } else if (pid > 0) {
                    // This is the parent process
                    // Continue listening for more print requests
                    cleanupMem(&newRequest);
                    continue;
                } else {
                    // Fork failed
                    perror("Server Erorr: fork -> failed to create request handling process");
                    cleanupMem(&newRequest);
                    send(newRequest ->clientSock, "SERVER ERROR!", 15, 0);
                    printf(RED"EXITING!\n"RESET);
;                    break;
                }
            }
        }else{
            cleanupMem(&newRequest);
        }
    }
    pthread_join(reaper_thread, NULL);
    close(serverfd);
    return 0;
}


