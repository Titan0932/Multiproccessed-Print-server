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

// void* handleClientSession(void* args){
//         Request* request = (Request *) args;
//         printf("SESS START = %ld\n", request->sessionStart);
//         while(processActive == 1){
//             time_t curTime = time(NULL);
//             if(curTime - request->sessionStart >= SESSION_TIMEOUT){
//                 printf("\nCLIENT SESSION TIMEOUT! Exiting!\n");
//                 send((request)->clientSock, "SESSION TIMEOUT! RECONNECT AGAIN!", (34), 0);
//                 close(request->clientSock);
//                 // cleanupMem(&request);
//                 processActive = 0;
//                 break;
//             }
//         // sleep(SESSION_TIMEOUT/2);
//         }
//         printf("\nEXITING SESSIONN THREAD!\n");
// }


void handle_request(Request** request) {
    // pthread_t sessionThread;
    // pthread_create(&sessionThread, NULL, handleClientSession, *request);

    // Set the file descriptor to non-blocking mode
    int flags = fcntl((*request)->clientSock, F_GETFL, 0);
    fcntl((*request)->clientSock, F_SETFL, flags | O_NONBLOCK);

    while(processActive == 1){
        ssize_t bytesRead = read((*request)->clientSock, (*request)->reqType, sizeof((*request)->reqType) - 1);
       
        // If read() returns -1 and errno is EAGAIN, the operation would have blocked
        if (bytesRead == -1 && errno == EAGAIN || EBADF) {
            continue; // Skip to the next iteration
        }

        // If read() returns 0, the client has closed the connection
        if (bytesRead == 0) {
            processActive = 0;
        }else{

            printf("HANDLING REQUEST!!\n");
            printf("Client: %d | Message: %s\n", (*request)->clientSock, (*request)->reqType);
            // Null-terminate the request type string
            (*request)->reqType[bytesRead] = '\0';
            
            if(strcmp((*request)->reqType, "STATUS") == 0 ){
                send((*request)->clientSock, "YOUR STATUS IS =?", (18), 0);
            }else{
                send((*request)->clientSock, "Print request sent!", (20), 0);
            }
        }
    }
    // pthread_join(sessionThread, NULL);
    printf("\nEXITING THE HANDLERRR FUNCSSS\n");
    // TODO
    // Send the request to the printer and wait for a response
    // int response = send_request_to_printer(*request);

    // Send the response to the client
    // send_response_to_client(response);
}


// void handle_request(Request** request) {
//     pthread_t sessionThread;
//     pthread_create(&sessionThread, NULL, handleClientSession, *request);
//     while(processActive == 1){
//         ssize_t bytesRead = read((*request)->clientSock, (*request)->reqType, sizeof((*request)->reqType) - 1);
       
//         // If read() returns 0, the client has closed the connection
//         if (bytesRead == 0) {
//             processActive = 0;
//         }

//         printf("HANDLING REQUEST!!\n");
//         printf("Client: %d | Message: %s\n", (*request)->clientSock, (*request)->reqType);
//         // Null-terminate the request type string
//         (*request)->reqType[bytesRead] = '\0';
        
//         if(strcmp((*request)->reqType, "STATUS") == 0 ){
//             send((*request)->clientSock, "YOUR STATUS IS =?", (18), 0);
//         }else{
//             send((*request)->clientSock, "Print request sent!", (20), 0);
//         }
//     }
//     pthread_join(sessionThread, NULL);
//     // TODO
//     // Send the request to the printer and wait for a response
//     // int response = send_request_to_printer(*request);

//     // Send the response to the client
//     // send_response_to_client(response);

// }

// if a child has terminated, remove counter of active requests
void* reap_children(void* arg) {
    int* activeReqs = (int*) arg;
    while(1){
        while (waitpid(-1, NULL, WNOHANG) > 0) {  // check if child processes have terminated.No hang if no change in states.
            (*activeReqs)--;
            printf("CHANGED ACTIVE reqs: %d\n", *activeReqs);
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

    printf("Server successfully initialized!\n");
    
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
                printf("SERVER TOO BUSY! TRY AGAIN LATER! \n");
                send(clientSocket, SERVEROVERLOAD, strlen(SERVEROVERLOAD),0);
                close(clientSocket);
                cleanupMem(&newRequest);
            }else{
                // Create a new child process to handle the request
                ++activeReqs;
                printf("NEW ACTIVE reqs: %d\n", activeReqs);
                pid_t pid = fork();

                if (pid == 0) {
                    // This is the child process
                    printf("\n CHILD PROCESS: %d\n", pid);
                    handle_request(&newRequest);
                    cleanupMem(&newRequest);
                    close(clientSocket);
                    printf("CLOSING CLIENT! BYEEE \n");
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
                    printf("EXITING!\n");
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


