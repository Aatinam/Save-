#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <libgen.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/wait.h>
#include "hash.h"

/*Returns a pointer to a dynamically allocated new_path
the new_path is created by appending the name of the file to be copied to the
destination path it needs to be copied to */
char *the_path_extractor(const char*src, const char* dest){
  char *new_path = malloc(strlen(dest) + 25);
  char extract_name[strlen(src) + 1];
  strcpy(extract_name, src);
  strcpy(new_path, dest);
  strcat(new_path, "/");
  strcat(new_path, basename(extract_name));
  return new_path;
}

/*A recursive function that copies file tree at src to file tree at dest. */

int copy_ftree(const char *src, const char *dest){
  struct stat buff;
  static int exit_value = 0;

  if(lstat(src, &buff) == -1){
    perror("lstat error");
    exit(1);
  }
  mode_t save_permissions = buff.st_mode & 0777;

  if(S_ISREG(buff.st_mode)){
    FILE *fsrc;
    FILE *fdest;

    char store[80]; //initialize to NULL
    for(int i=0; i < 80; i++){
      store[i] = '\0';
    }

    if((fsrc = fopen(src, "r")) == NULL){
      perror("fopen");
      exit(1);
    }
    /*create path for fdest*/
    char *new_path;
    new_path = the_path_extractor(src, dest);
    struct stat dest_stat;
    int check_return = lstat(new_path, &dest_stat);
    if( check_return == -1){ //file does not exist at destination create new
      if((fdest = fopen(new_path, "w+")) == NULL){
        perror("fopen dest1");
        exit(1);
      } //set permissions
      if(chmod(new_path, save_permissions) == -1){
        perror("chmod when new file created");
        exit(1);
      } //write to file
      while(feof(fsrc) == 0){ //end of file hasn't been reached
        fread(store, 80 , sizeof(char), fsrc);
        fwrite(store, 80 , sizeof(char), fdest);
      }
      fclose(fdest);
    }

    else{ //file exists at destination
      if(S_ISDIR(dest_stat.st_mode)){
        fprintf(stderr, "%s\n", "Type mismatch occured!" );
      }
      else{
      mode_t dest_perm = dest_stat.st_mode & 0777;
      if(dest_perm != save_permissions){
        if(chmod(new_path, save_permissions) == -1){
          perror("chmod");
          exit(1);
        }
      }
      //compare size
      if(dest_stat.st_size != buff.st_size){
        if((fdest = fopen(new_path, "w")) == NULL){
          perror("fopen dest");
        }
        while(feof(fsrc) == 0){ //end of file hasn't been reached
          fread(store, 80 , sizeof(char), fsrc);
          fwrite(store, 80 , sizeof(char), fdest);
        }
        fclose(fdest);
      } else{ //size is same compute hash
        char *src_hash ;
        char *dest_hash;
        if((fdest = fopen(new_path, "r+")) == NULL){ //open for reading and writing
          perror("fopen dest");
        }
        src_hash = hash(fsrc);
        dest_hash = hash(fdest);
        int i, j;
        j = 0;
        for(i = 0; i < 9; i ++){
          if(src_hash[i] != dest_hash[i]){
            j++;
          }
        }
        if(j > 0){ //different hashes
          rewind(fsrc);
          rewind(fdest);
          while(feof(fsrc) == 0){ //end of file hasn't been reached
            fread(store, 80 , sizeof(char), fsrc);
            fwrite(store, 80 , sizeof(char), fdest);
          }
        }
        fclose(fdest);
        }
      }
    }
    fclose(fsrc);
  }

   if(S_ISDIR(buff.st_mode)){ //src is directory
      DIR *src_dir;
      if((src_dir = opendir(src)) == NULL){
        perror("opendir");
        exit(1);
      }
      char *dir_path;
      dir_path = the_path_extractor(src, dest);
      struct stat dir_buff;
      if(lstat(dir_path, &dir_buff) == -1){ //directory does not exist create directory
        if(mkdir(dir_path, save_permissions) == -1){
          perror("mkdir");
          exit(1);
        }
      }
      if(S_ISREG(dir_buff.st_mode)){
        fprintf(stderr, "%s\n", "Type mismatch occurred!" );
      }
      else{
      mode_t dir_permissions = dir_buff.st_mode & 0777;
      if(dir_permissions != save_permissions){
        chmod(dir_path, save_permissions);
      }
      struct dirent *copyme;
      struct stat check_files;
      char path_name[150]; //design decsision, assuming no path is longer than 150 char
      while((copyme = readdir(src_dir)) != NULL){
       if((copyme ->d_name)[0] != '.'){
         strcpy(path_name, src);
         strcat(path_name, "/");
         strcat(path_name, copyme -> d_name);
         if(lstat(path_name, &check_files) == -1){
           perror("Did not create pathname properly");
           exit(1);
         }
         if(S_ISREG(check_files.st_mode)){
           copy_ftree(path_name, dir_path);
         }
         if(S_ISDIR(check_files.st_mode)){
           int ret_fork;
           ret_fork = fork();

            if (ret_fork > 0){
                int status;
                if (wait(&status) == -1){
                 perror("wait");
               } else {
                      if(WIFEXITED(status)){ //positive exit value
                        if(exit_value < 0){
                        exit_value--;
                        }
                        else if(exit_value == 0){ //exit_value >= 0
                          exit_value++;
                        }
                        else{
                          exit_value++;
                        }
                      } else { //terminated with negative value
                          if(exit_value < 0){
                                exit_value--;
                            }
                          else { //exit_value >= 0
                            exit_value = -1 * exit_value; //make exit_value negative
                              exit_value--;
                          }
                        }
                      }
                    }
                   else if( ret_fork == 0){ //child process
                      copy_ftree(path_name, dir_path);
                      exit(1);
                    } else{
                        perror("fork");
                        exit(1);
                      }
              }
            }
          }
          }
        }
  return exit_value;
}
