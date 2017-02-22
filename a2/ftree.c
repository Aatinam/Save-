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

/*generates path for next */

char *generate_path(struct dirent* next_node, const char* path){
	 
	 if(next_node != NULL && (next_node -> d_name)[0] != '.'){
	 	               int path_len = strlen(path);
	 	               int name_len = strlen(next_node -> d_name);
	 						int required_length = path_len + name_len + 3;
	 						char new_path[required_length];
	 						strcat(new_path, path);
	 						strcat(new_path, '/'); //do i need escape char
	 						strcat(new_path, (new_node -> d_name));
	 					}	
	  return new_path;
	  }
	  
/*Assign n1 -> next = n2*/
void assign_next(struct TreeNode *n1, struct TreeNode *n2){
	n1 -> next = n2;
}

/*
 * Returns the FTree rooted at the path fname.
 */
struct TreeNode *generate_ftree(const char *fname) {

struct stat *buff;
	if(lstat(path, buff) != 0){ //error
		printf("%s\n", strerror(errno));
	}else{
		
		struct TreeNode *new_node = malloc(sizeof(struct TreeNode));
		/*set name */
		char *extract_name; 
		extract_name = strrchr(path, '/'); //last occurrence of '/'
		int path_len = strlen(path);
		int num_char_to_cpy = &(path[path_len - 1]) - extract_name + 1; //calculating length of name 
		strncpy((new_node->fname), (extract_name + 1), num_char_to_cpy);	
	
		/*set permissions*/
		new_node -> permissions = ((buff -> st_mode) & 0777); 	
		
		/*	set contents if directory, set hash if file or link*/
		if(S_ISREG(buff -> st_mode) || S_ISLNK(buff -> st_mode)){ //file or link 
			new_node -> contents = NULL;
			FILE *f;			
			if((f= fopen(path, "r")) != NULL){
			new_node -> hash = hash(f);
			}else{
				printf("Incorrect File Path");
				}	 //strcpy or is this okay??
		}if(S_ISDIR(buff -> st_mode)) {
			new_node -> hash = NULL;
			struct dirent dirs[MAXDIRS];
			struct dirent *dp;
			DIR *dirp = opendir(path);
				if(dirp == NULL) {
					perror("opendir");
					exit(1);
				} else {
				 int i = 0;
	 			 while((dp = readdir(dirp)) != NULL){
	 			if(((dp -> d_name)[0]) != '.' ){
	 				dirs[i] = *dp;
	 				i++;
	 			}if(i == MAXDIRS && dp != NULL) {
				fprintf(stderr, 
		    	"Error: program does not support more than %d entries\n", MAXDIRS);
				}
			}	
				closedir(dirp);
				int j;
				struct TreeNode* tn_array[i + 1];
				for(j = 0; j < (i + 1); j++){
				char *new_path = generate_path(dirs[j], path);
				Tn_array[j] = generate_ftree(new_path);
				new_node -> contents = tn_array[0];
				int k;
				for(k = 0; k < i; k ++){
				assign_next(Tn_array[k], Tn_array[k+1]);
				}
			}
			/*set next*/
		   new_node -> next = NULL;
	}
    return NULL;
}


/*
 * Prints the TreeNodes encountered on a preorder traversal of an FTree.
 */
void print_ftree(struct TreeNode *root) {
    // Here's a trick for remembering what depth (in the tree) you're at
    // and printing 2 * that many spaces at the beginning of the line.
    static int depth = 0;
    printf("%*s", depth * 2, "");

    // Your implementation here.
}
