#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <libgen.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include "ftree.h"
#include "hash.h"

#ifndef PORT
    #define PORT 30000
#endif

struct client {
    int fd;
    struct in_addr ipaddr;
    struct client *next;
    int state;
    int loop;
    struct request new_request;
};

int handleclient(struct client *p, struct client *top);
int setup(struct sockaddr_in their_addr);
int bindandlisten(void);
struct request request_filler(char *src);
static struct client *addclient(struct client *top, int fd, struct in_addr addr);
static struct client *removeclient(struct client *top, int fd);
int check_similarity(struct request check_me);
int write_request_to_server(int main_soc, struct request new_request);



/*rcopy_client connects to server and sends requests of files that need to be copied
depending on server response client either moves on to next file or forks() a child
process to transmit data*/

int rcopy_client(char *source, char *host, unsigned short port){

/*Step 1 initializing socket */
int main_soc;
struct hostent *host_info;
struct sockaddr_in their_addr; /* connector's address information  */

/*extract host info */
if ((host_info=gethostbyname(host)) == NULL) {
    perror("gethostbyname");
    exit(1);
}

if ((main_soc = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
    perror("socket");
    exit(1);
}

their_addr.sin_family = AF_INET;    /* host byte order */
their_addr.sin_port = htons(PORT);  /* short, network byte order */
their_addr.sin_addr = *((struct in_addr *)host_info->h_addr);
bzero(&(their_addr.sin_zero), 8);   /* zero the rest of the struct */

/*step 2: connect */
if (connect(main_soc, (struct sockaddr *)&their_addr,
sizeof(struct sockaddr)) == -1) {
    perror("connect");
    exit(1);
}


struct request new_request = request_filler(source);

/*writing struct request to socket */
if( write_request_to_server(main_soc, new_request) == 1){
  printf("%s\n", "Error in main client while writing request" );
}
//Settin max_fd
int max_fd = main_soc;
fd_set all_fds, listen_fds, write_fds, copy_writefds;
FD_ZERO(&all_fds);
FD_ZERO(&write_fds);
FD_SET(main_soc, &all_fds);
FD_SET(main_soc, &write_fds);

while (1) {
  listen_fds = all_fds;
  copy_writefds = write_fds;
  int nready = select(max_fd + 1, &listen_fds, &copy_writefds, NULL, NULL);
  if (nready == -1) {
      perror("server: select");
      exit(1);
  }

//server has recieved struct from main client, read response and act accordingly
    if(FD_ISSET(main_soc, &listen_fds)){
    int temp;
    int num_read = read(main_soc, &temp, sizeof(int));
    if(num_read != sizeof(int)){
      perror("could not read server response");
      exit(1);
    }
    int server_response = ntohl(temp);
    printf("Response from server %d\n", server_response );
    if(new_request.type == REGDIR){
      if(server_response == OK){
        printf("Open dir and start recursion\n");
      }
      if(server_response == ERROR){
        fprintf(stderr, "%s\n", "Directory mismatch occured" );
        exit(1);
      }
    }
    if(new_request.type == REGFILE){
      if(server_response == OK){
        close(main_soc);
      }
      if(server_response == ERROR){
        fprintf(stderr, "%s\n", "Server responded with ERROR");
        close(main_soc);
      }
      //fork starts
    if(server_response == SENDFILE){
        int ret_fork = fork();
        /*parent forks a child and waits for it to exit */
                if (ret_fork > 0){
                    int status;
                    if (wait(&status) == -1){
                        perror("wait");
                      }
                    }

                else if( ret_fork == 0){
                  /*child process */
                    //1.send same struct but with type transfile
                    int child_soc = setup(their_addr);
                    new_request.type = htonl(TRANSFILE);
                    if(write_request_to_server(child_soc, new_request) == 1){
                      fprintf(stderr, "%s\n", "Error while writing request in client child" );
                    }
                    //2.without waiting for response send data
                    char buf[MAXDATA];
                    FILE *fsrc;
                    for(int i=0; i < MAXDATA; i++){ //initialize to NULL
                      buf[i] = '\0';
                    }
                    if((fsrc = fopen(new_request.path, "rb")) == NULL){
                      perror("fopen");
                      exit(1);
                    }
                    while(feof(fsrc) == 0){
                      fread(buf, MAXDATA , sizeof(char), fsrc);
                      if(write(child_soc, buf, MAXDATA) != MAXDATA){
                        perror("Error while writing file data");
                      }
                    }
                    //3.Wait for OK then close
                    int ok;
                    if(read(child_soc, &ok, sizeof(int)) != sizeof(int)){
                      perror("Error in read response in child");
                      exit(1);
                    }
                  if(ok == OK){
                    close(child_soc);
                    exit(0);
                  }
                  else{
                    printf("Error while transfering file rooted at %s\n", new_request.path );
                    close(child_soc);
                    exit(1);
                  }
                }

                else{
                  perror("fork");
                  exit(1);
                    }

    }
    }
}
}
close(main_soc);
return 0;
}



//////////////////////
/*function rcopy_server accepts multiple connections, it reads request from client
and compares it to files present at PATH, if files need to be copied it sends
appropriate response to client. Server does not close after copy is completed */


void rcopy_server(unsigned short port){

  int clientfd, maxfd, nready;
  struct client *p;
  struct client *head = NULL;
  socklen_t len;
  struct sockaddr_in q;
  fd_set allset;
  fd_set rset;

  int i;
  int listenfd = bindandlisten();
  // initialize allset and add listenfd to the
  // set of file descriptors passed into select
  FD_ZERO(&allset);
  FD_SET(listenfd, &allset);
  maxfd = listenfd;


      while (1) {
          // make a copy of the set before we pass it into select
          rset = allset;
          nready = select(maxfd + 1, &rset, NULL, NULL, NULL);

          if (nready == -1) {
              perror("select");
              continue;
          }

          if (FD_ISSET(listenfd, &rset)){
              printf("a new client is connecting\n");
              len = sizeof(q);
              if ((clientfd = accept(listenfd, (struct sockaddr *)&q, &len)) < 0) {
                  perror("accept");
                  exit(1);
              }
              FD_SET(clientfd, &allset);
              if (clientfd > maxfd) {
                  maxfd = clientfd;
              }
              printf("connection from %s\n", inet_ntoa(q.sin_addr));
              head = addclient(head, clientfd, q.sin_addr);
          }

          for(i = 0; i <= maxfd; i++) {
              if (FD_ISSET(i, &rset)) {
                  for (p = head; p != NULL; p = p->next) {
                      if (p->fd == i) {
                          int result = handleclient(p, head);
                          if(result == 0){
                            continue;
                          }
                          if(result == 2){
                            //result 2 implies that server is ready to recieve
                            //data for file that needs to be copied
                            char buf[MAXDATA];
                            int read_file;
                              int save_permissions = (p->new_request.mode) & 0777;
                            if((read_file = read(p->fd, buf, MAXDATA)) > 0){
                            if(p->loop == 0){
                              //if loop 0, then first iteration of read
                              //use fopen in "w" mode to over ride existing data
                              FILE *f;
                              if((f = fopen((p->new_request.path), "wb")) == NULL){
                                perror("fopen");
                                exit(1);
                              }
                              if (read_file != MAXDATA) {
                                  perror("file not read");
                                  exit(1);
                              }
                              fwrite(buf, MAXDATA , sizeof(char), f);
                              p->loop = 1; //update loop
                            }
                            if(p->loop == 1){
                              //open file in append mode so write continues where
                              //first read left of
                              FILE *f;
                              if((f = fopen((p->new_request.path), "ab")) == NULL){
                                perror("fopen");
                                exit(1);
                              }
                              if (read_file != MAXDATA) {
                                  perror("file not read");
                                  exit(1);
                              }
                              fwrite(buf, MAXDATA , sizeof(char), f);
                            }
                          }
                          else{ //read == 0
                              if(chmod(p->new_request.path, save_permissions) == -1){
                                perror("chmod");
                              }
                              int save = htonl(OK);
                              int check = write(p->fd, &save, sizeof(save));
                              if(check != sizeof(save)){
                                fprintf(stderr, "%s\n", "Could not send response to child");
                              }
                          }
                          }
                          if (result == -1) {
                              int tmp_fd = p->fd;
                              head = removeclient(head, p->fd);
                              FD_CLR(tmp_fd, &allset);
                              close(tmp_fd);
                          }
                          break;
                      }
                  }
              }
          }
      }

}


int bindandlisten(void) {
    struct sockaddr_in r;
    int listenfd;

    if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket");
        exit(1);
    }
    int yes = 1;
    if ((setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int))) == -1) {
        perror("setsockopt");
    }
    memset(&r, '\0', sizeof(r));
    r.sin_family = AF_INET;
    r.sin_addr.s_addr = INADDR_ANY;
    r.sin_port = htons(PORT);

    if (bind(listenfd, (struct sockaddr *)&r, sizeof r)) {
        perror("bind");
        exit(1);
    }

    if (listen(listenfd, 5)) {
        perror("listen");
        exit(1);
    }
    return listenfd;
}

static struct client *addclient(struct client *top, int fd, struct in_addr addr) {
    struct client *p = malloc(sizeof(struct client));
    if (!p) {
        perror("malloc");
        exit(1);
    }

    printf("Adding client %s\n", inet_ntoa(addr));

    p->fd = fd;
    p->ipaddr = addr;
    p->next = top;
    p -> state = 0;
    p -> loop = 0;
    top = p;
    return top;
}



static struct client *removeclient(struct client *top, int fd) {
    struct client **p;

    for (p = &top; *p && (*p)->fd != fd; p = &(*p)->next)
        ;
    // Now, p points to (1) top, or (2) a pointer to another client
    // This avoids a special case for removing the head of the list
    if (*p) {
        struct client *t = (*p)->next;
        printf("Removing client %d %s\n", fd, inet_ntoa((*p)->ipaddr));
        free(*p);
        *p = t;
    } else {
        fprintf(stderr, "Trying to remove fd %d, but I don't know about it\n",
                 fd);
    }
    return top;
}
