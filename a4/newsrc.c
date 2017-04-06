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
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <netdb.h>
#include "ftree.h"
#include "hash.h"


/*The function takes in path for file and fills out struct request in
client, changing all integers to htnol for transfer */

struct request request_filler(char *src){
struct request new_request;
struct stat buff;
if(lstat(src, &buff) == -1){
  perror("lstat error");
  exit(1);
}

if(S_ISREG(buff.st_mode)){
  new_request.type = htonl(REGFILE);
  FILE *f;
  if((f = fopen(src, "r")) == NULL){
    perror("fopen file");
  }
  char *hash_val = hash(f);
  for (int index = 0; index < BLOCKSIZE; index++) {
      new_request.hash[index] = hash_val[index];
  }
  new_request.size = htonl(buff.st_size);
}

if(S_ISDIR(buff.st_mode)){
  new_request.type = htonl(REGDIR);
  /*if directory initialize hash to null*/
  for (int index = 0; index < BLOCKSIZE; index++) {
      new_request.hash[index] = '\0';
  }

  new_request.size = 0;
}
/*extract and save basename of source to transfer to server */
int basename_length = strlen(basename(src)) + 1;
strncpy(new_request.path, basename(src), basename_length);

new_request.mode = htonl(buff.st_mode);

return new_request;
}

//////////////////////////////////////////////////////////
/*Server calls check_similarity struct request with type REGFILE
and returns the appropriate response; one of OK, ERROR or TRANSFILE */

int check_similarity(struct request check_me){
int src_perm = check_me.mode & 0777;

struct stat buff;

    if(lstat(check_me.path, &buff) == -1){
      //file doesn't exist: SENDFILE
      return SENDFILE;
    }
  int dest_perm = buff.st_mode & 0777;
    if(check_me.type == REGFILE ){
      if (!S_ISREG(buff.st_mode)){
        fprintf(stderr, "%s\n", "Type mismatch: dir at dest vs. file at src" );
        return ERROR;
      }
      else{
        /*check and update permissions if required */
        if(src_perm != dest_perm){
          if(chmod(check_me.path , src_perm) == -1){
            perror("chmod");
            exit(1);
          }
        }
        /*check size, if size differs send transfer file signal */
        if(check_me.size != buff.st_size){
          return SENDFILE;
        } else{ //size same compute hash

              FILE *f = fopen(check_me.path, "r");
              char *dest_hash = malloc(BLOCKSIZE);
              dest_hash = hash(f);
              for(int i = 0; i < BLOCKSIZE; i++){
                if(check_me.hash[i] != dest_hash[i]){
                  return SENDFILE;
                }
              }
              //if we reach here that implies hash is same, no need to copy
              return OK;
        }
    }
  }
      else{ //should not reach here
        fprintf(stderr, "%s\n", "Should not reach here" );
        return ERROR;

      }
}
////////////////////////////////////////////////////

/*This function writes struct request to server */

int write_request_to_server(int main_soc, struct request new_request){
  int write_type = write(main_soc, &(new_request.type), sizeof(int));
  if (write_type != sizeof(int)) {
      perror("request.type not written");
      close(main_soc);
      exit(1);
  }
  int write_path = write(main_soc, new_request.path, MAXPATH) ;
  if (write_path != MAXPATH) {
      perror("path not written");
      close(main_soc);
      exit(1);
  }
  int write_mode = write(main_soc, &(new_request.mode), sizeof(new_request.mode));
  if (write_mode != sizeof(new_request.mode)) {
      perror("mode not written");
      close(main_soc);
      exit(1);
  }
  int write_hash = write(main_soc, new_request.hash, BLOCKSIZE);
  if (write_hash != BLOCKSIZE) {
      perror("hash not written");
      close(main_soc);
      exit(1);
  }
  int write_size  = write(main_soc, &(new_request.size), sizeof(int));
  if (write_size != sizeof(int)) {
      perror("size not written");
      close(main_soc);
      exit(1);
  }

return 0;
}
/////////////////////////////////////////////////
