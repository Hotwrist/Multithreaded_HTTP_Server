# HTTP Server
 
## Description

  This is a multithreaded HTTP server that constructs an HTTP response based on the
  client request and send the response to the client.
  
  The server uses TCP to handle connections with clients by creating a socket for each
  client it talks to. 
  
  The server maintains a threadpool in threadpool.c by creating a limited number of thread 
  that connects with the client. It does this by creating the pool of threads in advance, and 
  each time it needs a thread to handle a client connection, it enqueues the request so that an 
  available thread in the pool will handle it.

## Usage

  Command line usage: server <port> <pool-size> <max-number-of-request>.
  **<Port>** is the port number the server will listen on, **<pool-size>** is the number of threads in the pool, 
  and **<max-number-of-request>** is the maximum number of requests the server will handle before it
  destroys the pool.

Requirements
------------

  You need GCC installed to run the server.

Quick start
-----------

    $ gcc server.c threadpool.c -o server -lpthread
    $ ./server 8080 13 7

## Options

	8080 - the port the server will listen on.
	13   - Pool size.
	7	 - Maximum number of requests.


## How does it work?

   The server serves html files located in the **public** subdirectory.
   
## Threadpool.c
   
   This file handles the threadpool creation, destruction, and thread tasks. The threadpool
   starts N threads and each of them picks up a task from a queue of work_t tasks.
   
   The functions available in the file are:
   
   **create_threadpool** : Creates a fixed-sized threadpool and initialize the threadpool structure. It also
   initializes a thread with the function *do_work*
   **do_work** : Runs in an endless loop. It is the work function of the threads. 
   **dispatch** : This creates the work_t structure and initializes it. It adds new item (i.e tasks or jobs) to the queue.  
   **destroy_threadpool** : It kills the threadpool causing all threads in it to commit suicide. It then frees all the
   memory associated with the threadpool. 
   
## Server.c

   This is the server file. It handles socket connection from clients, accepts the client connection and server
   the requested file to the client.
	
   The functions available in the file are:
	
   **date_func** : This handles the creation of date and time.
   **response** : Constructs the server response message.
   **send_302** : Sends a *302 Found* message to the client when the path is a directory but does not end with a /.
   **send _400** : Sends a *400 Bad Request* message to the client when the client gives a bad request.
   **send_403** : Sends a *403 Forbidden message* when the client tries to access a file he/she has no read permission on.
   **send_404** : Sends a *404 Not Found* message when the requested path does not exist.
   **send_500** : Sends a *500 Internal Server Error* when there is a failure after connection with a client
   and if the error is due to some server side error.
   **send_501** : Sends a *501 Not Supported* message when the method is not a GET method. This is because
   the server only supports the GET method.
   **send_dir_content** : Sends a message by displaying the directory content when the index.html file is not found. 
   **get_mime_type** : Gets the file type based on the file extension.
   **create_socket** : This creates a listening socket for the server to listen for incoming client connections.
   **read_client_request** : Reads the client request into a request buffer.
   **serve_resource** : This function serves the requested file based on the client request to the client.
   **handle_socket_thread** : This function handles the socket thread. This function is passed to the dispatch function, which
   is then added to the work_t structure as a task or job awaiting to be handled by an available thread.
