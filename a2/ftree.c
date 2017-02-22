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

/*a recursive function that creates fills in TreeNode, given a starting node
*/
struct TreeNode *fill_node(const char* path){

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
			struct dirent *dp;
			DIR *dirp = opendir(path);
				if(dirp == NULL) {
					perror("opendir");
					exit(1);
				} else {
					dp = readdir(dirp); //if path is not null read first element in dir
	 				if(dp != NULL && ((dp -> d_name)[0] != '.')){
	 	               int path_len = strlen(path);
	 	               int name_len = strlen(dp -> d_name);
	 						int required_length = path_len + name_len + 5;
	 						char new_path[required_length];
	 						strcat(new_path, path);
	 						strcat(new_path, '/'); //do i need escape char
	 						strcat(new_path, (dp-> d_name));
	 						new_node -> contents = fill_node(new_path);
					  }dp = readdir(dirp);
					  if(dp != NULL && (dp -> d_name)[0]) != '.' ){
	 						int required_length = strlen(path) + strlen(dp -> d_name) + 5;
	 						char new_path[required_length];
	 						strcat(new_path, path);
	 						strcat(new_path, '/'); //do i need escape char
	 						strcat(new_path, (dp-> d_name));
	 						(new_node -> contents) -> next = fill_node(new_path);
				 }
			}
			/*set next*/
		   new_node -> next = NULL;
	}
}

/*
 * Returns the FTree rooted at the path fname.
 */
struct TreeNode *generate_ftree(const char *fname) {
    // Your implementation here.
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
