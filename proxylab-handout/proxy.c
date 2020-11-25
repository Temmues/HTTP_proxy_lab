#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <string.h>
#include "http_parser.h"
#include <unistd.h>
/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

/* You won't lose style points for including this long line in your code */
static const char *user_agent_hdr = "Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 Firefox/10.0.3";


/* Creates a socket for specified port 
	Input: port
	Output: socket file descriptor
*/


/* read from client or server into a buffer, 0 for client, 1 for server */

int read_fs(int fd, char * buf, int size, int client_or_server)
{
	int current_size = 0;
	int nread;
	buf[size] = '\0';
	while(1)
	{
		nread = read(fd, &buf[current_size], size);
		current_size += nread; 
		if(nread == -1)
		{
			fprintf(stderr, "Read error\n");
			return -1;
		}
		if(client_or_server == 0)
		{
			if (strstr(buf, "\r\n\r\n") != NULL)
			{
				return current_size;
			} 
		}
		else if(nread == 0)
		{
			return current_size;
		}
	}
}

/* writes to outward file descriptor */ 
int write_fs(int fd, char * buf, int size)
{
	int total_written = 0;
	int written;
	while(1)
	{
		written = write(fd, &buf[total_written], size);

		total_written += written;
		size -= written;

		if(written == -1)
		{
			fprintf(stderr, "write error\n");
			return -1;
		}
		else if (written == 0)
		{
			return total_written; 
		}
	}

}


/*
	reads, parses request into headers, modifies input host, and returns size of parsed request.  
*/
int read_and_parse_request(int connfd, char * out_head, char * host, char * port)
{
	int head_num;
	http_header header[100];
	char buf[500];
	char method[10];
	char r_port[100];
	char uri[100];
	char r_host[100];				

	if (read_fs(connfd, buf, 500, 0) == -1)
	{
		exit(-1);	
	}	
		
	if(!is_complete_request(buf))
	{
		fprintf(stderr, "invalid format header\n");
		exit(-1);
	}	

	//parsing initial header
	if((head_num = parse_request(buf, method, r_host, r_port, uri, header)) < 1)
	{
		fprintf(stderr, "parse failed\n");
		exit(-1);
	}
	
	strcpy(host, r_host);
	strcpy(port, r_port);

	int len = 0;
	char * out_head_track = out_head;
	len += sprintf(out_head_track,"%s /%s HTTP/1.0 \r\n", method, uri);
	out_head_track = out_head + len;
	len += sprintf(out_head_track,"Host: %s:%s\r\n", r_host, r_port);
	out_head_track = out_head + len;
	for(int i = 0; i < head_num; i++)
	{
		char * name = header[i].name;
		if((strcmp(name, "Host") != 0) && (strcmp(name, "User-Agent") != 0) && (strcmp(name, "Connection") != 0) && (strcmp(name, "Proxy-Connection") != 0))
		{

			len += sprintf(out_head_track, "%s: %s\r\n", name, header[i].value);
			out_head_track = out_head + len;

		}

	}
	len += sprintf(out_head_track,"User-Agent: %s\r\n", user_agent_hdr);
	out_head_track = out_head + len;
	len += sprintf(out_head_track,"Connection: close\r\n");
	out_head_track = out_head + len;
	len += sprintf(out_head_track,"Proxy-Connection: close\r\n");
	out_head_track = out_head + len;
	len += sprintf(out_head_track,"\r\n");
	out_head_track = out_head + len;

	return len;
}

/* forwards request to outward server, outputs size of server response stored in answer */
int forward_request(char * request, int len, char * answer, char * port, char * host)
{
	struct addrinfo hints;
	struct addrinfo *result, * rp;
	int sfd, s;
	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = 0;
	hints.ai_protocol = 0;
	
					
	port[strlen(port) - 1] = '\0';	//remove trailing \n
	s = getaddrinfo(host, port, &hints, &result);	

	if (s != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(s));
		exit(EXIT_FAILURE);
	}

	for (rp = result; rp != NULL; rp = rp->ai_next) 
	{
		sfd = socket(rp->ai_family, rp->ai_socktype,
				rp->ai_protocol);
		if(sfd == -1)
			continue;
		if(connect(sfd, rp->ai_addr, rp->ai_addrlen) != -1)
			break;
		close(sfd);
	} 	
	
	if (rp == NULL) {
		fprintf(stderr, "Could not connect\n");
		exit(-1);
	}
	
	freeaddrinfo(result);

	//write request to server
	write_fs(sfd, request, len);

	//read answer	
	int total_bytes = read_fs(sfd, answer, MAX_OBJECT_SIZE, 1);
	printf("this read failed fd: %d\n", sfd);	
	close(sfd);
	return total_bytes;
			
}

/* sets up listening socket returns a file descriptor */
int establish_initial_connection(struct sockaddr_in ip4addr)
{
	int listenfd;
	//socket	
	if((listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) 
	{
		fprintf(stderr, "socket error\n");
		exit(-1);
	}
	
	//bind
	if(bind(listenfd, (struct sockaddr *)&ip4addr, sizeof(struct sockaddr_in)) < 0)
	{
		close(listenfd);
		fprintf(stderr, "bind error\n");
		exit(-1);	
	}
	
	//establish listening
	if (listen(listenfd, 100) < 0) 
	{
		close(listenfd);
		fprintf(stderr, "listen error\n");
		exit(-1);
	}
	return listenfd; 	

}

void *thread(void *fd)
{


}

int main(int argc, char **argv)
{
	
	if(argc < 2)
	{
    		printf("Usage: client_port_num\n");
		exit(0);
 	}
	char * port = argv[1];
	char request[1000];
	char answer[MAX_OBJECT_SIZE];
	char host[100];
	char r_port[100];
	int listenfd;
	int connfd;
	socklen_t clientlen;
	struct sockaddr_in ip4addr;
	struct sockaddr_storage clientaddr;
	

	//listen for incoming port connection on specified port	
	ip4addr.sin_family = AF_INET;
	ip4addr.sin_port = htons(atoi(port));
	ip4addr.sin_addr.s_addr = INADDR_ANY;	

	listenfd = establish_initial_connection(ip4addr);
//close(listenfd);
	
	while(1)
	{
		clientlen = sizeof(struct sockaddr_storage);
		connfd = accept(listenfd, (struct sockaddr *)&clientaddr, &clientlen);
	
		//2. Read and parse request 
		int len = read_and_parse_request(connfd, request, host, r_port);
	   	
		//3. Send forward request and get response
		int total_bytes = forward_request(request, len, answer, r_port, host);
	
		write_fs(connfd, answer, total_bytes);
		close(connfd);
	}	
    	exit(0);
}
