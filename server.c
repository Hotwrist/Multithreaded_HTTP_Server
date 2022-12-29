#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include "threadpool.h"

#define IS_VALID_SOCKET(s) ((s) >= 0) // On UNIX, returns a negative number on failure
#define CLOSE_SOCKET(s) close(s)
#define SOCKET int
#define GET_SOCKET_ERRNO() (errno)

#define NUM_CMDLINE_ARG 4
#define	BUFFER_SIZE	1024

char req_buffer[BUFFER_SIZE];	// For client requests
char buffer[BUFFER_SIZE];	// For server response

//--------------------for the Date header-------------------
#define RFC1123FMT "%a, %d %b %Y %H:%M:%S GMT"
char time_buf[128];

char* date_func()
{
	time_t now = time(NULL);
	strftime(time_buf, sizeof(time_buf),RFC1123FMT, gmtime(&now));
	
	return time_buf;
}
//----------------------------------------------------------

// This function uses the message format in file.txt to construct
// the server response message.
void response(const char *content_type, const size_t content_len)
{
    sprintf(buffer, "HTTP/1.0 200 OK\r\n");
    sprintf(buffer + strlen(buffer), "Serrver: webserver/1.0\r\n");
    sprintf(buffer + strlen(buffer), "Date: %s\r\n", time_buf);
    sprintf(buffer + strlen(buffer), "Location: \r\n");
    sprintf(buffer + strlen(buffer), "Content-Type: %s\r\n", content_type);
    sprintf(buffer + strlen(buffer), "Content-Length: %u\r\n", content_len); 
    sprintf(buffer + strlen(buffer), "Last Modified: %s\r\n", time_buf); 
    sprintf(buffer + strlen(buffer), "Connection: close\r\n");
    sprintf(buffer + strlen(buffer), "\r\n");
}

// Sends a message Found back to the client when the path is a directory but does not
// end with a /.
void send_302(int sock_client_fd, const char *path) 
{
	char buffer[2048];
	
	sprintf(buffer, "HTTP/1.1 302 Found\r\n");
	sprintf(buffer + strlen(buffer), "Server: webserver/1.0\r\n");
	sprintf(buffer + strlen(buffer), "Date: %s\r\n", time_buf);
	sprintf(buffer + strlen(buffer), "Location: %s/\r\n", path);
	sprintf(buffer + strlen(buffer), "Content-Type: text/html\r\n");
	sprintf(buffer + strlen(buffer), "Content-Length: 123\r\n");
	sprintf(buffer + strlen(buffer), "Connection: close\r\n");
	sprintf(buffer + strlen(buffer), "\r\n");
	sprintf(buffer + strlen(buffer), "<html><head><title>302 Found</title></head>\r\n");
	sprintf(buffer + strlen(buffer), "<body><h4>302 Found</h4>\r\n");
	sprintf(buffer + strlen(buffer), "Directories must end with a slash.\r\n");
	sprintf(buffer + strlen(buffer), "</body></html>");
	sprintf(buffer + strlen(buffer), "\r\n");
	
    send(sock_client_fd, buffer, strlen(buffer), 0);
}

// Sends a Bad Request message to the client in the even of a bad request from the client.
void send_400(int sock_client_fd) 
{
	char buffer[2048];
	
	sprintf(buffer, "HTTP/1.1 400 Bad Request\r\n");
	sprintf(buffer + strlen(buffer), "Server: webserver/1.0\r\n");
	sprintf(buffer + strlen(buffer), "Date: %s\r\n", time_buf);
	sprintf(buffer + strlen(buffer), "Content-Type: text/html\r\n");
	sprintf(buffer + strlen(buffer), "Content-Length: 113\r\n");
	sprintf(buffer + strlen(buffer), "Connection: close\r\n");
	sprintf(buffer + strlen(buffer), "\r\n");
	sprintf(buffer + strlen(buffer), "<html><head><title>400 Bad Request</title></head>\r\n");
	sprintf(buffer + strlen(buffer), "<body><h4>400 Bad Request</h4>\r\n");
	sprintf(buffer + strlen(buffer), "Bad Request.\r\n");
	sprintf(buffer + strlen(buffer), "</body></html>");
	sprintf(buffer + strlen(buffer), "\r\n");
	
    send(sock_client_fd, buffer, strlen(buffer), 0);
}

// Sends a Forbidden message to the client when he/she tries to access a file he/she
// has no read permission on.
void send_403(int sock_client_fd) 
{
	char buffer[2048];
	
	sprintf(buffer, "HTTP/1.1 403 Forbidden\r\n");
	sprintf(buffer + strlen(buffer), "Server: webserver/1.0\r\n");
	sprintf(buffer + strlen(buffer), "Date: %s\r\n", time_buf);
	sprintf(buffer + strlen(buffer), "Content-Type: text/html\r\n");
	sprintf(buffer + strlen(buffer), "Content-Length: 111\r\n");
	sprintf(buffer + strlen(buffer), "Connection: close\r\n");
	sprintf(buffer + strlen(buffer), "\r\n");
	sprintf(buffer + strlen(buffer), "<html><head><title>403 Forbidden</title></head>\r\n");
	sprintf(buffer + strlen(buffer), "<body><h4>403 Forbidden</h4>\r\n");
	sprintf(buffer + strlen(buffer), "Access denied.\r\n");
	sprintf(buffer + strlen(buffer), "</body></html>");
	sprintf(buffer + strlen(buffer), "\r\n");
	
    send(sock_client_fd, buffer, strlen(buffer), 0);
}

// Sends a 404 Not Found message when the requested path does not exist.
void send_404(int sock_client_fd) 
{
	char buffer[2048];
	
	sprintf(buffer, "HTTP/1.1 404 Not Found\r\n");
	sprintf(buffer + strlen(buffer), "Server: webserver/1.0\r\n");
	sprintf(buffer + strlen(buffer), "Date: %s\r\n", time_buf);
	sprintf(buffer + strlen(buffer), "Content-Type: text/html\r\n");
	sprintf(buffer + strlen(buffer), "Content-Length: 112\r\n");
	sprintf(buffer + strlen(buffer), "Connection: close\r\n");
	sprintf(buffer + strlen(buffer), "\r\n");
	sprintf(buffer + strlen(buffer), "<html><head><title>404 Not Found</title></head>\r\n");
	sprintf(buffer + strlen(buffer), "<body><h4>404 Not Found</h4>\r\n");
	sprintf(buffer + strlen(buffer), "File not found.\r\n");
	sprintf(buffer + strlen(buffer), "</body></html>");
	sprintf(buffer + strlen(buffer), "\r\n");
	
    send(sock_client_fd, buffer, strlen(buffer), 0);
}

// Sends a 500 Internal Server Error when there is a failure after connection with a client
// and if the error is due to some server side error.
void send_500(int sock_client_fd) 
{
	char buffer[2048];
	
	sprintf(buffer, "HTTP/1.1 500 Internal Server Error\r\n");
	sprintf(buffer + strlen(buffer), "Server: webserver/1.0\r\n");
	sprintf(buffer + strlen(buffer), "Date: %s\r\n", time_buf);
	sprintf(buffer + strlen(buffer), "Content-Type: text/html\r\n");
	sprintf(buffer + strlen(buffer), "Content-Length: 144\r\n");
	sprintf(buffer + strlen(buffer), "Connection: close\r\n");
	sprintf(buffer + strlen(buffer), "\r\n");
	sprintf(buffer + strlen(buffer), "<html><head><title>500 Internal Server Error</title></head>\r\n");
	sprintf(buffer + strlen(buffer), "<body><h4>500 Internal Server Error</h4>\r\n");
	sprintf(buffer + strlen(buffer), "Some server side error.\r\n");
	sprintf(buffer + strlen(buffer), "</body></html>");
	sprintf(buffer + strlen(buffer), "\r\n");
	
    send(sock_client_fd, buffer, strlen(buffer), 0);
}

// Sends a 501 Not Supported message when the method is not a GET method. This is because
// the server only supports the GET method. 
void send_501(int sock_client_fd) 
{
	char buffer[2048];
	
	sprintf(buffer, "HTTP/1.1 501 Not Supported\r\n");
	sprintf(buffer + strlen(buffer), "Server: webserver/1.0\r\n");
	sprintf(buffer + strlen(buffer), "Date: %s\r\n", time_buf);
	sprintf(buffer + strlen(buffer), "Content-Type: text/html\r\n");
	sprintf(buffer + strlen(buffer), "Content-Length: 129\r\n");
	sprintf(buffer + strlen(buffer), "Connection: close\r\n");
	sprintf(buffer + strlen(buffer), "\r\n");
	sprintf(buffer + strlen(buffer), "<html><head><title>501 Not Supported</title></head>\r\n");
	sprintf(buffer + strlen(buffer), "<body><h4>501 Not Supported</h4>\r\n");
	sprintf(buffer + strlen(buffer), "Method is not supported.\r\n");
	sprintf(buffer + strlen(buffer), "</body></html>");
	sprintf(buffer + strlen(buffer), "\r\n");
	
    send(sock_client_fd, buffer, strlen(buffer), 0);
}

// Sends a message by displaying the directory content when the index.html file is not found.
void send_dir_content(int sock_client_fd, const char *path)
{
	char buffer[9000];
	
	sprintf(buffer, "HTTP/1.1 200 OK\r\n");
	sprintf(buffer + strlen(buffer), "Server: webserver/1.0\r\n");
	sprintf(buffer + strlen(buffer), "Date: %s\r\n", time_buf);
	sprintf(buffer + strlen(buffer), "Content-Type: text/html\r\n");
	sprintf(buffer + strlen(buffer), "Content-Length: 700\r\n");
	sprintf(buffer + strlen(buffer), "Last Modified: %s\r\n", time_buf);
	sprintf(buffer + strlen(buffer), "Connection: close\r\n");
	sprintf(buffer + strlen(buffer), "\r\n");
	sprintf(buffer + strlen(buffer), "<html><head><title>Index of %s</title></head>\r\n", path);
	sprintf(buffer + strlen(buffer), "<body><h4>Index of %s</h4>\r\n", path);
	sprintf(buffer + strlen(buffer), "<table cellspacing=8>\r\n");
	sprintf(buffer + strlen(buffer), "<tr><th>Name</th><th>Last Modified</th></tr>\r\n");
	
	DIR *folder;
	struct dirent *entry;
	folder = opendir("./public");
	
	while(entry = readdir(folder))
	{
		sprintf(buffer + strlen(buffer), "<tr><td><a href=\"%s\">%s</a></td><td>%s</td></tr>\r\n", entry->d_name, entry->d_name, date_func());
	}
	closedir(folder);
	
	sprintf(buffer + strlen(buffer), "</table>\r\n");
	sprintf(buffer + strlen(buffer), "<hr>\r\n");
	sprintf(buffer + strlen(buffer), "<address>webserver/1.0</address>\r\n");
	sprintf(buffer + strlen(buffer), "</body></html>");
	sprintf(buffer + strlen(buffer), "\r\n");
	
    send(sock_client_fd, buffer, strlen(buffer), 0);
}

// Gets the file type based on the file extension
char *get_mime_type(char *name)
{
	char *ext = strrchr(name, '.');
	
	if(!ext) return NULL;
	if(strcmp(ext, ".html") == 0 || strcmp(ext, ".htm") == 0) return "text/html";
	if(strcmp(ext, ".jpg") == 0 || strcmp(ext, ".jpeg") == 0) return "image/jpeg"; 
	if(strcmp(ext, ".gif") == 0) return "image/gif";
	if(strcmp(ext, ".png") == 0) return "image/png";
	if(strcmp(ext, ".css") == 0) return "text/css";
	if(strcmp(ext, ".au") == 0) return "audio/basic";
	if(strcmp(ext, ".wav") == 0) return "audio/wav";
	if(strcmp(ext, ".avi") == 0) return "video/x-msvideo";
	if(strcmp(ext, ".mpeg") == 0 || strcmp(ext, ".mpg") == 0) return"video/mpeg"; 
	if(strcmp(ext, ".mp3") == 0) return "audio/mpeg";

	return NULL;
}

// This creates a listening socket for the server to listen for incoming client connections.
SOCKET create_socket(const char* host, const char *port)
{
    printf("Configuring local address...\n");
    
    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;	// Same as  htonl(INADDR_ANY)

    struct addrinfo *bind_address;
    getaddrinfo(host, port, &hints, &bind_address);	// Host here is NULL(0) meaning it can listen on any address.

    printf("Creating socket...\n");
    SOCKET socket_listen;
    socket_listen = socket(bind_address->ai_family, bind_address->ai_socktype, bind_address->ai_protocol);
    
    if (!IS_VALID_SOCKET(socket_listen)) 
    {
        fprintf(stderr, "socket() failed. (%d)\n", GET_SOCKET_ERRNO());
        exit(1);
    }

    printf("Binding socket to local address...\n");
    if (bind(socket_listen, bind_address->ai_addr, bind_address->ai_addrlen)) 
    {
        fprintf(stderr, "bind() failed. (%d)\n", GET_SOCKET_ERRNO());
        exit(1);
    }
    
    freeaddrinfo(bind_address);

    printf("Listening...\n");
    if (listen(socket_listen, 10) < 0)
    {
        fprintf(stderr, "listen() failed. (%d)\n", GET_SOCKET_ERRNO());
        exit(1);
    }

    return socket_listen;
}

// Reads the client request into a request buffer.
char *read_client_request(int socket_client_fd)
{
	int received_bytes = recv(socket_client_fd, req_buffer, (BUFFER_SIZE - 1), 0);
	
	if(received_bytes < 1)
	{
		fprintf(stderr, "Error reading from socket.\n");
		CLOSE_SOCKET(socket_client_fd);
	}
	
	req_buffer[received_bytes] = '\0';
	return req_buffer;
}

// This function serves the requested file based on the client request to the client.
void serve_resource(int socket_fd, const char *path) 
{
	printf("serving_resource %s\n", path);
	
	// If path is a directory but does not end with /
	char *forward_slash = strrchr(path, '/');
	
	if(forward_slash == NULL) send_302(socket_fd, path);
	
	// Serve default page, index.html
    if (strcmp(path, "/") == 0) path = "/index.html";

    if (strlen(path) > 500) 
    {
        send_400(socket_fd);
        return;
    }

	// For security reasons, user should not execute the command '..' else he/she will gain
	// access to the parent directory.
    if (strstr(path, "..")) 
    {
        send_404(socket_fd);
        return;
    }

    char full_path[128];
	sprintf(full_path, "public%s", path);	// All our pages will be placed in a 'public' folder or directory which should be created.
	
	//-------------------------------------------------------------------------------------
	// Open the public directory( since its the directory where all our files are stored.
	// Search for index.html
	DIR *folder;
	struct dirent *entry;
	folder = opendir("./public");
	
	int found = 0;
	
	// Search for index.html
	while(entry = readdir(folder))
	{
		if(strcmp(entry->d_name, "index.html") == 0) 
		{
			found = 1;
			break;
		}
		else continue;
	}
	
	closedir(folder);
	
	//index.html was not found, display directory content
	if(found == 0) 
	{
		send_dir_content(socket_fd, path);
		return;
	}
	//---------------------------------------------------------------------------------

    FILE *fp = fopen(full_path, "rb");

    if (!fp) // fp fails, meaning index.html does not exist OR read permission not allowed.
    {
        send_403(socket_fd);
        
        return;
    }

	else
	{
		// We need to get the content length of the resource file.
		// fseek reads to the end of the file. The value of the content length
		// is gotten by passing fp, file pointer, to ftell() to give us the value.
		fseek(fp, 0L, SEEK_END);
		size_t content_len = ftell(fp);
		rewind(fp);	// Get back to the begining of the file. i.e from SEEK_END

		const char *content_type = get_mime_type(full_path);
		
		response(content_type, content_len);
		send(socket_fd, buffer, strlen(buffer), 0);

		int read = fread(buffer, 1, BUFFER_SIZE, fp);
		while (read) 
		{
			send(socket_fd, buffer, read, 0);
			read = fread(buffer, 1, BUFFER_SIZE, fp);
		}

		fclose(fp);
	}
}

// This function handles the socket thread by parsing the client request using threads.
// In other words, a thread is allocated to handle this functionality.
int handle_socket_thread(void *socket_fd_arg)
{
	int socket_fd = *((int*)socket_fd_arg);
	
	char *sockclient_req_text = read_client_request(socket_fd);
	printf("Request from client: %s\n\n", sockclient_req_text);
	
	// HTTP header and body is separated by a blank line '\r\n\r\n'. Look for it
	char *end_of_http_header = strstr(sockclient_req_text, "\r\n\r\n");
	
	if(end_of_http_header)
	{
		*end_of_http_header = 0;
		
		if(strncmp("GET", sockclient_req_text, 3)) send_501(socket_fd);
		
		if(strncmp("GET /", sockclient_req_text, 5)) 
		{
			send_400(socket_fd);
		}
		else
		{
			 char *path = sockclient_req_text + 4;
             char *end_path = strstr(path, " ");
             
             if (!end_path) 
             {
				send_404(socket_fd);
             } 
             else 
             {
				*end_path = 0;
                serve_resource(socket_fd, path);
             }
		}
	}
}

int main(int argc, char *argv[]) 
{
    if (argc < NUM_CMDLINE_ARG) 
    {
        fprintf(stderr, "usage: server <port> <pool-size> <max-number-of-request>\n");
        return 1;
    }

	SOCKET server = create_socket(0, argv[1]);
	
	threadpool *pool = create_threadpool(atoi(argv[2]));
	
	int max_number_of_requests = atoi(argv[3]);
	int request_counter = 1;
	
	struct sockaddr_storage client_address;
	socklen_t client_len = sizeof(client_address);
	
	while(request_counter <= max_number_of_requests)
	{
		if(request_counter != max_number_of_requests)
		{
			int new_socket_fd = accept(server, (struct sockaddr*) &client_address, (socklen_t *) &client_len);
			if (!IS_VALID_SOCKET(new_socket_fd)) 
			{
				fprintf(stderr, "accept() connection failed. (%d)\n", GET_SOCKET_ERRNO());
				return 1;
			}
			
			char address_buffer[100];
			getnameinfo((struct sockaddr*)&client_address, client_len, address_buffer, sizeof(address_buffer), 0, 0, NI_NUMERICHOST);
			printf("New connection from: %s\n", address_buffer);
			
			int *arg = &new_socket_fd;
			
			dispatch(pool, handle_socket_thread, arg);
		}
		else
			destroy_threadpool(pool);
	}

	CLOSE_SOCKET(server);
	
	return 0;
}



