## Introduction and note!;
    The purpose of this project was to practice and use concurrency with multi-processing and multi-threading, and IPC.
    
    The design and architecture might be impractivally too complex and the server is not as centralized as it practically would be. But, this was intentionally complicated such that I could practice concurrency more! However, it does have a lot of potential as a lot of stuff can be still built on it!

    This project was made in 1.5 days. SO DISCLAIMER: it is not perfect in terms of code cleanliness and organization or super robust error handling (it does handle them though!). 

-- This is a multi-proccessed and multi-threaded print server.


## Instructions: 
-- To compile:
    make all

-- To remove executables and object files:
    make clean

-- To run:
    ./server -> to run the server
    ./client -> to run the client
    ./printer -> to run the printer


## NOTE: MAKE SURE TO RUN THE SERVER AND THE PRINTER BEFORE YOU RUN THE CLIENT!!

Here's a very brief overview of what the different components of the project do and look like:


1. A server that manages printing requests:
    1. It listens for print requests.
    2. Creates a child process for each client to handle the requests separately and asynchronously.
    3. Keeps track of the number of child processes handling requests to balance server load - using a separate async thread
2. A client that can simply send two requests:
    1. A print request - Request to print a specific document: a string in this case for simplicity
    2. A printer queue request - Check number in the queue
    3. It also uses a thread that listens and displays incoming messages to the client from the server.
3. A printer that is used in the simulation that:
    1. Has a queue to store the print requests.
    2. Communicates with the request handler processes when a print was successful/failure using pipes.
    3. Has 2 threads: 
        1. A thread to print the documents in order.
        2. A thread to handle each request
            1. When a print request is received, it adds the document and the client to the queue
            2. When a status request is received, it sends the status to the handler that sent the request.
4. A print request handler process:
    1. It’s a child of the main server that handles the requests and correspondence for each client.
    2. Has a thread to listen to the printer’s responses and relays them to the clients asynchronously.
    3. A thread to listen to client requests and relay them to the printer
