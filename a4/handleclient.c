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

struct client {
    int fd;
    struct in_addr ipaddr;
    struct client *next;
    int state;
    int loop;
    struct request new_request;
};

int check_similarity(struct request check_me);

/*This function takes care of reading from client sockets, it fills out the
struct request and updates state in client so after each read call a different field
of the struct is filled. If AWAITING_DATA stage is reached it checks to see
if request.type is directory or file. If its a directory it checks if directory is in destination
if it is it writes OK to client, if its not then it creates directory */
int handleclient(struct client *p, struct client *top) {

    if( (p ->state) == AWAITING_TYPE){
      int save;
      int read_type = read( p->fd, &save, sizeof(int));
      if (read_type != sizeof(int)) {
          perror("request.type not read");
          return -1;
      }
      (p-> new_request.type) = ntohl(save);
      (p->state)++;
      return 0;
    }
    if( (p -> state) == AWAITING_PATH){
      int read_path = read( p->fd, (p->new_request.path), MAXPATH);
      if (read_path != MAXPATH) {
          perror("request.path not read");
          return -1;
      }
      (p->state)++;
      return 0;
    }
    if( (p -> state) == AWAITING_SIZE){
      int save;
      int read_size = read( p-> fd, &save , sizeof(int));
      if (read_size != sizeof(int)) {
          perror("request.size not read");
          return -1;
      }
      (p ->new_request.size) = ntohl(save);
      (p->state)++;
      return 0;
    }
    if((p -> state) == AWAITING_PERM){
      int save;
      int read_mode = read(p->fd, &save, sizeof(mode_t));
      if (read_mode != sizeof(mode_t)) {
          perror("request.mode not read");
          return -1;
      }
      (p->new_request.mode )= ntohl(save);
      (p->state)++;
      return 0;
    }
    if( (p ->state) == AWAITING_HASH){
      char temp[BLOCKSIZE];
      int read_hash = read(p->fd, temp, BLOCKSIZE);
      if (read_hash != BLOCKSIZE) {
          perror("request.hash not read");
          return -1;
      }
      for(int i = 0; i < BLOCKSIZE; i++){
        (p->new_request.hash)[i] = temp[i];
      }
      (p->state)++;
      return 0;
    }


    if ((p->state) == AWAITING_DATA){
      if((p -> new_request.type) == REGFILE){
        printf("%s\n","Reach2" );
        //send response OK or SENDFILE
          int ret = htonl(check_similarity(p->new_request));
          if((write(p->fd, &ret, sizeof(ret))) != sizeof(ret)){
            perror("could not write response to client");
            close(p->fd);
            return -1;
          }
      return 0;
      }
      if ((p -> new_request.type) == REGDIR){
          struct stat dir_buff;
          int save_permissions = (p->new_request.mode) & 0777;
          if(lstat((p->new_request.path), &dir_buff) == -1){ //directory does not exist create directory
            if(mkdir((p->new_request.path), save_permissions) == -1){
              perror("mkdir");
              return -1;
            }
        }
        else{ //dir exists
          if(!S_ISDIR(dir_buff.st_mode)){
            fprintf(stderr, "%s\n", "Type mismatch: file at dest vs. dir at src" );
            int save = htonl(ERROR);
            if(write(p->fd, &save, sizeof(save)) != sizeof(save)){
              perror("Error while writing response");
              return -1;
          }
        }
        //is dir
        mode_t dir_permissions = dir_buff.st_mode & 0777;
        if(dir_permissions != save_permissions){
          chmod((p->new_request.path), save_permissions);
        }
      }
      int save = htonl(OK);
      if(write(p->fd, &save, sizeof(save)) != sizeof(save)){
        perror("Error while writing response");
        return -1;
      }
      return 0;
    }
      if((p->new_request.type) == TRANSFILE){ //handle this case in main function
          return 2;

        }
    }


     else { // shouldn't happen
        perror("read in ");
        return -1;
        }
    return 0;
}

/*Creates socket for child process and connect to port with same address as parent */
int setup(struct sockaddr_in their_addr){
  int child_soc;
  struct sockaddr_in make_copy = their_addr;
  if ((child_soc = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
      perror("socket");
      exit(1);
  }

  if (connect(child_soc, (struct sockaddr *)&make_copy,
  sizeof(struct sockaddr)) == -1) {
      perror("connect");
      exit(1);
  }
  return child_soc;
}
