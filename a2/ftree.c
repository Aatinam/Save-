#include <stdio.h>
// Add your system includes here.
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <stdlib.h>
#include <string.h>
#include <error.h>
#include <errno.h>

#include "ftree.h"
#include "hash.h"
#define MAXDIRS 100


/*
 * Returns the FTree rooted at the path fname.
 */
struct TreeNode *generate_ftree(const char *fname) {

/*lstat*/
    struct stat *buff;
    buff = (struct stat *) malloc(sizeof(struct stat));
    if(lstat(fname, buff) != 0){ //error
        printf("%s\n", strerror(errno));
            exit(1);
    }

    /*lstat worked, declare TreeNode*/
    struct TreeNode *node;
    node = (struct TreeNode *) malloc(sizeof(struct TreeNode));


    /*set name */
    char * last_slash = strrchr(fname, '/');
 	 int path_len = strlen(fname);
  	 int num_to_cpy = &(fname[path_len - 1]) - last_slash + 1; //calculating length of name + 1 for null terminator
    node -> fname = (char *) malloc(num_to_cpy);
    strncpy((node-> fname), last_slash + 1, num_to_cpy);
 
    /*set permissions*/
    node -> permissions = ((buff -> st_mode) & 0777);

    /*    set contents if directory, set hash if file or link*/
    if(S_ISREG(buff -> st_mode) || S_ISLNK(buff -> st_mode)){ //file or link
        /*set contents to Null since file or link*/
        (node -> contents) = NULL;

        /*set hash*/
        FILE *f;
        f= fopen(fname, "r");
        if( f != NULL){
            node -> hash = hash(f);
         } //hash assigned if block ended
         else{
            printf("Incorrect File Path");
            }
            }

	 
	 /*case 2; IS directory */
    if(S_ISDIR(buff -> st_mode)){ 
              (node -> hash) = NULL;
            /*need to set contents recursively, require readdir to open directory*/
            DIR *dir_ptr = opendir(fname);
                            if(dir_ptr == NULL) {
                                    perror("opendir");
                                    exit(1);
                                }
            int size = 0;
            struct dirent *element;
            struct dirent dirs[MAXDIRS];
            while((element = readdir(dir_ptr)) != NULL){
                     if(((element -> d_name)[0]) != '.' ){
                                 dirs[size] = *element;
                                 size++;
                     }if(size == MAXDIRS && element != NULL) {
                                 fprintf(stderr,
                                 "Error: program does not support more than %d entries\n", MAXDIRS);
                         }
                    }
                 closedir(dir_ptr);
                
                if(size == 0) {//empty directory
                    node -> contents = NULL;
			          return node;
                }
                else {
                    //assignment of contents
                    int length = strlen(fname) + strlen(dirs[0].d_name) + 2;
                    char *new_path;
                    new_path = (char *) malloc(length);
                    strcpy(new_path,(const char *) fname);
                    strncat(new_path, "/", 1);
                    strncat(new_path, (dirs[0].d_name), strlen(dirs[0].d_name));
                    node -> contents = generate_ftree(new_path); //set first element to contents
          			  free(new_path);

                } 
                struct TreeNode *prev_node;
                prev_node = node -> contents;
                int j ;
                for (j = 1; j < size; j++) { //we want it to start at one since dirs[0] is set as contents
                int length = strlen(fname) + strlen(dirs[j].d_name) + 2; //+ 2 for null and /
                char *path_two;
                path_two = (char *) malloc(length);
                strcpy(path_two, fname);
                strncat(path_two, "/", 1);
                strncat(path_two, (dirs[j].d_name), strlen(dirs[j].d_name));
        			 prev_node -> next = generate_ftree(path_two);
        			 prev_node = prev_node -> next; 
            } 
            prev_node = NULL; 
            return prev_node;
        } 
     
    return node;
}


/*
 * Prints the TreeNodes encountered on a preorder traversal of an FTree.
 */
void print_ftree(struct TreeNode *root) {
    // Here's a trick for remembering what depth (in the tree) you're at
    // and printing 2 * that many spaces at the beginning of the line.
    static int depth = 0;
    
    if((root -> hash) == NULL){//i.e empty string hence, directory
        printf("%*s", depth * 2, "");
            printf("===== %s (%d) =====\n",(root -> fname), (root -> permissions));
            
        if((root -> contents)!= NULL){
          depth++;
          print_ftree(root -> contents);
        }
        
  } else{//file or link
        printf("%*s", depth * 2, "");
        printf("%s (%d)\n",(root -> fname), (root -> permissions));
        while(root -> next != NULL){
          print_ftree(root -> next);
        }
    }


}

