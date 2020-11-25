#include<stdio.h>
#include<string.h>

#define HEADER_NAME_MAX_SIZE 512
#define HEADER_VALUE_MAX_SIZE 1024
#define MAX_HEADERS 32

typedef struct {
	char name[HEADER_NAME_MAX_SIZE];
	char value[HEADER_VALUE_MAX_SIZE];
} http_header;

/* Input: request, a string
 * Output: 1 if request is a complete HTTP request, 0 otherwise
 * */
int is_complete_request(const char *request) {
	
	// check for all the bits
	if(strstr(request, "GET") == NULL)
	{
		return 0;
	}
	else if (strstr(request, "Host:") == NULL)
	{
		return 0;
	}
	else if (strstr(request, "User-Agent:") == NULL)
	{
		return 0;
	}
	else if (strstr(request, "Accept:") == NULL)
	{
		return 0;
	}
	return 1;
}

/* Parse an HTTP request, and copy each parsed value into the
 * corresponding array as a NULL-terminated string.
 * Input: request - string containing the original request;
 *        should not be modifed.
 * Input: method, hostname, port, uri - arrays to which the
 *        corresponding parts parsed from the request should be
 *        copied, as strings.  The uri is the "file" part of the requested URL.
 *        If no port is specified, the port array should be populated with a
 *        string specifying the default HTTP port.
 * Input: headers - an array of http_headers, each of which should be
 *        populated with the corresponding name/value of a header.
 * Output: return the number of headers in the request.
 * */
int parse_request(const char *request, char *method,
		char *hostname, char *port, char *uri, http_header *headers) {
	char copy[HEADER_VALUE_MAX_SIZE];
	char hostCopy[500];
//	char head[200];
	//copy request
	strcpy(copy, request);

	//Method
	strcpy(method, strtok(copy, " "));
	
	//Parse till beginning of uri
	strtok(NULL, "//");
	strtok(NULL, "/");
	
	//uri
	strcpy(uri, strtok(NULL," "));
	
	//end of line 1
	strtok(NULL, "\n");
	
	//Parse line two
	strtok(NULL, " ");
	strcpy(hostCopy, strtok(NULL, "\n"));
	
	//Seperate host and port
	if(strstr(hostCopy, ":") != NULL)
	{
		strcpy(hostname, strtok(hostCopy, ":"));
		strcpy(port, strtok(NULL, "\n"));
	}
	else
	{
		strcpy(hostname, hostCopy);
		strcpy(port, "80");
	}	
	
	//re-copy request
	char reCopy[HEADER_VALUE_MAX_SIZE];
	strcpy(reCopy, request);
	
	//Parsing for headers
	//char * token; 
	int i = 0;
	strtok(reCopy, "\n");
	while(1)
	{
		char * val;
		char * name;	
		name = strtok(NULL, ":");
		val = strtok(NULL, "\n");	

		if(val == NULL)
		{
			break;
		}

		memcpy(headers[i].name, name, strlen(name));
		memcpy(headers[i].value, val, strlen(val));
		i++;	
	}
	return i;
}

/* Iterate through an array of headers, and return the value
 * (as a char *) corresponding to the name passed.  If there is no
 * header with the name passed, return NULL.
 * Input: name - the name of the header whose value is being sought.
 * Input: headers - the array of http_headers to be searched.
 * Input: num_headers - the number of headers in the headers array.
 * */
char *get_header_value(const char *name, http_header *headers, int num_headers) 
{
	for(int i = 0; i < num_headers; i++)
	{
		if(strstr(headers[i].name, name) != NULL)
		{
			return headers[i].value;
		}
	}
	return NULL;
}
