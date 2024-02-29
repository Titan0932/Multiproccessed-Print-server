#include "defs.h"

//TODO: Create a thread pool
pthread_mutex_t queueLock;

PrintRequest* printQueue[MAX_ACTIVE_REQS+1];
int queueSize = 0;


void showQueue(){
    printf("\nPRINTING\n");
    for(int i=0; i<queueSize; i++){
        printf("\nClient: %d\n", printQueue[i]->clientSock);
    }
}

int enqueue(PrintRequest** request) {
    pthread_mutex_lock(&queueLock);
    if (queueSize < MAX_ACTIVE_REQS) {
        printQueue[queueSize++] = *request;
        pthread_mutex_unlock(&queueLock);
        return SUCCESS;
    } else {
        pthread_mutex_unlock(&queueLock);
        return FAILURE;
    }
}

int dequeue() {
    pthread_mutex_lock(&queueLock);
    if (queueSize > 0) {
        PrintRequest* request = printQueue[0];
        int clientSock = request->clientSock;  // Save clientSock before unlocking the mutex
        for (int i = 0; i < queueSize - 1; i++) {
            printQueue[i] = printQueue[i + 1];
        }
        queueSize--;
        free(request);
        pthread_mutex_unlock(&queueLock);
        printf("\nDeleted job for client: %d\n", clientSock);
        return SUCCESS;
    } else {
        pthread_mutex_unlock(&queueLock);
        return FAILURE;
    }
}

int findQueuePos(int clientSocket){
    pthread_mutex_lock(&queueLock);
    for(int i=0; i<queueSize; i++){
        if(printQueue[i]->clientSock == clientSocket){
            pthread_mutex_unlock(&queueLock);
            return i;
        }
    }
    pthread_mutex_unlock(&queueLock);
    return -1;
}

void* printDoc(void* args){
    while(1){
        if(queueSize > 0){
            // pthread_mutex_lock(&queueLock);
            if(printQueue[0] != NULL){
                printf(GRN"\n ============== Printing!! Pleasse wait! ================= \n");
                sleep(10);    /// take 10 secons to print for testing purposes
                printf("%s",printQueue[0]->document);
                printf("\n ==============            ================= \n"RESET);
                // SEND MESSAGE TO CLIENT THAT DOCUMENT WAS PRINTED!
                send(printQueue[0]->clientSock, "PRINTER: DOCUMENT PRINTED!!!", 29, 0);
            }else{printf("\nINVALID PRINT REQUEST! Skipping...\n");}
            if(dequeue() == FAILURE){
                //handle error
                printf("DEQUEUE failure!!\n");
            }
            // pthread_mutex_unlock(&queueLock);
            sleep(1);
        }else{
            sleep(2);
        }
    }
}

void* handleRequests(void* args){
    int* clientSock = (int*) args; 
    char req[PRINT_LIMIT];
    while(1){
        int bytesRead = read(*clientSock, req, sizeof(req) - 1);
        if(bytesRead > 1){
            if(strcmp(req,"STATUS") == 0){
                int pos = findQueuePos(*clientSock);
                if(pos == -1){
                    send(*clientSock, "PRINTER: NO REQUESTS IN QUEUE!", 31, 0);
                }else{
                    char message[30];
                    sprintf(message, "PRINTER: Postion in queue: %d", pos+1);
                    send(*clientSock, message, 30, 0);
                }
            }else{
                PrintRequest* request = (PrintRequest*) calloc(1, sizeof(PrintRequest));
                request->clientSock = *clientSock;
                strcpy(request->document, req);
                if(enqueue(&request) == SUCCESS){
                    send(request->clientSock, "PRINTER: Added document to printjob!", 37, 0);
                }else{
                    send(request->clientSock, "PRINTER: Queue is full. Try again later!", 41, 0);
                }
            }
        }else if(bytesRead == 0){
            printf("\nLetting go of %d\n", *clientSock);
            break;
        }else{
            send(printQueue[0]->clientSock, "PRINTER:ERROR! Disconnecting...", 32, 0);
            perror("Printer ERROR: read");
            break;
        }
    }
    close(*clientSock);
    free(clientSock);
    clientSock = NULL;
}

int listen_for_requests(int* printerfd, int* clientfd, struct sockaddr_in* address, socklen_t* addrlen){
    if ((*clientfd = accept(*printerfd, (struct sockaddr *)address, addrlen)) < 0) {
        perror("Printer Error: accept");
        return(EXIT_FAILURE);
    }
    return EXIT_SUCCESS;
}


// Printer Process
int main() {
    pthread_mutex_init(&queueLock, NULL);
    pthread_t printingThread;
    pthread_create(&printingThread, NULL, printDoc, NULL);
    pthread_t listenThread;


    int printerFd;
    struct sockaddr_in address;
    int opt = 1;
    socklen_t addrlen = sizeof(address);
    if((printerFd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
        perror("Server failed to initialize!");
        exit(EXIT_FAILURE);
    }
    if (setsockopt(printerFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PRINTER_PORT);
    if (bind(printerFd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        perror("Printer Error: bind failed");
        exit(EXIT_FAILURE);
    }

    printf(GRN"Printer successfully initialized!\n"RESET);
    
    if (listen(printerFd, 3) < 0) {
        perror("Printer Error: listen");
        return(EXIT_FAILURE);
    }
    while (1) {
        int* clientfd = (int*) calloc(1, sizeof(int));
        int requestStatus = listen_for_requests(&printerFd, clientfd, &address, &addrlen);
        if(requestStatus == EXIT_FAILURE){
            free(clientfd);
            continue;
        }else{
            pthread_create(&listenThread, NULL, handleRequests, clientfd);
        }
    }
    pthread_mutex_destroy(&queueLock);
    pthread_join(printingThread, NULL);
    close(printerFd);
}