/* A simple server in the internet domain using TCP
   The port number is passed as an argument
   This version runs forever, forking off a separate
   process for each connection
*/
#include <stdio.h>
#include <sys/types.h>   // definitions of a number of data types used in socket.h and netinet/in.h
#include <sys/socket.h>  // definitions of structures needed for sockets, e.g. sockaddr
#include <netinet/in.h>  // constants and structures needed for internet domain addresses, e.g. sockaddr_in
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/wait.h>	/* for the waitpid() system call */
#include <signal.h>	/* signal name macros, and the kill() prototype */
#include <unistd.h>
#include <sys/stat.h>


   void error(char *msg)
   {
    perror(msg);
    exit(1);
  }

// Primitive http request format; should be enough for the spec in this class
  struct request {
    char *type;
    char *file;
    char *host;
    char *connection;
    char *accept;
    char *user_agent;
  };

// function to form the request struct from an incoming string
  struct request *parse_http_request(char* in) {
    struct request *r = malloc(sizeof(struct request));
    char *t = strstr(in, "GET");
    if (!t) return NULL;
    r->type = strndup(t, 3);

  char* f = strstr(in, "/"); //get the requested file, always at least "/"
  if (!f) return NULL;
  size_t length = 1;
  while (*(f+length) != ' ') {
    length++; //iterate until find a space, indicating end of filename
  }
  r->file = strndup(f, length);

  char* h = strstr(in, "Host: ");
  if (!h) return NULL;
  length = strlen("Host: ");
  while (*(h+length) != '\n') {
    length++;
  }
  r->host = strndup((h+strlen("Host: ")), (length-strlen("Host: ")));

  char* c = strstr(in, "Connection: ");
  if (!c) return NULL;
  length = strlen("Connection: ");
  while (*(c+length) != '\n') {
    length++;
  }
  r->connection = strndup((c+strlen("Connection: ")), (length-strlen("Connection: ")));

  char* a = strstr(in, "Accept: ");
  if (!a) return NULL;
  length = strlen("Accept: ");
  while (*(a+length) != '\n') {
    length++;
  }
  r->accept = strndup((a+strlen("Accept: ")), (length-strlen("Accept: ")));

  char* u = strstr(in, "User-Agent: ");
  if (!u) return NULL;
  length = strlen("User-Agent: ");
  while (*(u+length) != '\0') {
    length++;
  }
  r->user_agent = strndup((u+strlen("User-Agent: ")), (length-strlen("User-Agent: ")));
  return r;
}

int main(int argc, char *argv[])
{
 int sockfd, newsockfd, portno, pid;
 socklen_t clilen;
 struct sockaddr_in serv_addr, cli_addr;

 if (argc < 2) {
   fprintf(stderr,"ERROR, no port provided\n");
   exit(1);
 }
     sockfd = socket(AF_INET, SOCK_STREAM, 0);	//create socket
     if (sockfd < 0)
      error("ERROR opening socket");
     memset((char *) &serv_addr, 0, sizeof(serv_addr));	//reset memory
     //fill in address info
     portno = atoi(argv[1]);
     serv_addr.sin_family = AF_INET;
     serv_addr.sin_addr.s_addr = INADDR_ANY;
     serv_addr.sin_port = htons(portno);

     if (bind(sockfd, (struct sockaddr *) &serv_addr,
      sizeof(serv_addr)) < 0)
      error("ERROR on binding");

     listen(sockfd,5);	//5 simultaneous connection at most

     //accept connections
     newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);

     if (newsockfd < 0)
       error("ERROR on accept");

     int n;
     char buffer[256];

   	 memset(buffer, 0, 256);	//reset memory

 		 //read client's message
   	 n = read(newsockfd,buffer,255);
   	 if (n < 0) error("ERROR reading from socket");
      struct request *formattedRequest = parse_http_request(buffer);
     // we have formatted the request, now we need to get the filename to open
      char* filename = formattedRequest->file;
      char filepath[256];
     getcwd(filepath, sizeof(filepath)); //first, get the pwd
     strcat(filepath, filename); //next, the name of the file
     printf("File path: %s\n", filepath);

     // get the file from the filesystem.
     FILE *fp;
     fp = fopen(filepath, "r");

     char b[1024];
     int filesize;

     if (!fp) {
      printf("Couldn't access the file\n");
      strcpy(b, "<!doctype html><html><head><title>404</title></head><body><h1>404: Page not found.</h1></body></html>\0");
      filesize = strlen(b);
    }
    else {
      printf("Found the file! ");
        // need to check if a directory
      struct stat st_buf;

      int status = stat (filepath, &st_buf);
      if (status != 0) {
        printf ("Error");
        return 1;
      }
        // directory
      if (S_ISDIR (st_buf.st_mode)) {
        printf ("Directory.\n");
        strcpy(b, "<!doctype html><html><head><title>404</title></head><body><h1>Trying to access a directory.</h1></body></html>\0");
        filesize = strlen(b);
      }
        // regular file. Serve it buddy
      if (S_ISREG (st_buf.st_mode)) {
        printf ("Regular file.\n");


        filesize = 0;
        int rc = getc(fp);
        for (rc; rc != EOF; rc = getc(fp)) {
          b[filesize] = (char)rc;
          filesize++;
        }
        b[filesize] = '\0';
      }
    }
    printf("%s\n", b);
    printf("Here is the message: %s\n",buffer);

   	 //reply to client
   	 //n = write(newsockfd,"I got your message",18);
    n = write(newsockfd, b, filesize);
    if (n < 0) error("ERROR writing to socket");


     close(newsockfd);//close connection
     close(sockfd);

     return 0;
   }

