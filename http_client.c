// Author Name: Dionysius Xaverio

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/stat.h>

#define BUFFER_SIZE 4096

int main(int argc, char* argv[]) {
    if (argc != 4) {
        fprintf(stderr, "Usage: %s <host> <port> <path>\n", argv[0]);
        return 1;
    }

    const char* host = argv[1];
    const char* port_str = argv[2];
    const char* path = argv[3];

    // Resolve host name to IP address
    struct addrinfo hints, * result;
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_INET;      // IPv4
    hints.ai_socktype = SOCK_STREAM;

    if (getaddrinfo(host, port_str, &hints, &result) != 0) {
        //perror("getaddrinfo");
        return 1;
    }

    int sockfd = socket(result->ai_family, result->ai_socktype, 0);
    if (sockfd == -1) {
        //perror("socket");
        return 1;
    }

    if (connect(sockfd, result->ai_addr, result->ai_addrlen) == -1) {
        //perror("connect");
        return 1;
    }

    // Prepare the HTTP GET request
    char request[256];
    snprintf(request, sizeof(request), "GET %s HTTP/1.1\r\nHost: %s\r\n\r\n", path, host);

    // Send the HTTP request
    if (send(sockfd, request, strlen(request), 0) == -1) {
        //perror("send");
        return 1;
    }

    // Receive the HTTP response
    char buffer[4096];
    
	int bytes_received = recv(sockfd, buffer, sizeof(buffer) - 1, 0);

    if (bytes_received == -1) {
        //perror("recv");
        return 1;
    }
    
	
	buffer[bytes_received] = '\0';
	

    // Check the response code
    if (strncmp(buffer, "HTTP/1.1 200 OK", 15) != 0) {
        // Print the first line of the response and exit
        fprintf(stdout, "Response: ");
		int idx = 0;

		char* fline = strtok(strdup(buffer), "\n");
		fprintf(stdout, "%s", fline);

		fprintf(stdout, "\n");
        close(sockfd);
        freeaddrinfo(result);
        return 1;
    }

	char* ret;
	ret = strstr(buffer, "Content-Length: ");
	int cl = atoi(strchr(strdup(ret), ' '));
		
	if (cl == 0) {
		//printf("Error: could not download the requested file (file length unknown)");
		return 1;
	}
  	
	//printf("Content-Length: %d", cl);

	char filename[256];
	snprintf(filename, sizeof(filename), "%s", strrchr(path, '/') + 1);
	FILE* file_ptr = fopen(filename, "wb");
	if (file_ptr == NULL) {
		//perror("fopen");
		return 1;
	}

	char* header_end_marker = strstr(buffer, "\r\n\r\n");
	int header_offset = header_end_marker - buffer + 4;
	fwrite(buffer + header_offset, 1, bytes_received - header_offset, file_ptr);

	while ((bytes_received = recv(sockfd, buffer, sizeof(buffer), 0)) > 0) {
		fwrite(buffer, 1, bytes_received, file_ptr);
	}

	if (bytes_received == -1) {
		//perror("recv");
		fclose(file_ptr);
		return 1;
	}

    // Clean up, close the socket, and close the file
    fclose(file_ptr);
    close(sockfd);
    freeaddrinfo(result);

    //printf("File '%s' downloaded successfully.\n", filename);

    return 0;
}
