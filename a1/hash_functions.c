#include <stdio.h>

// Complete these two functions according to the assignment specifications


void hash(char *hash_val, long block_size) {
int i;
		for(i = 0; i < block_size; i++){
			hash_val[i] = '\0';
			} 
char store_char;
int j = 0;
		while(scanf("%c", &store_char) != EOF ){
				if(j < block_size){ //first wrap around
						hash_val[j]= (hash_val[j])^store_char; 
						j++;
				} else{	// j == block_size
						j = 0;			
						hash_val[j] = (hash_val[j])^store_char;
						j++;
				 }
			}
}

int check_hash(const char *hash1, const char *hash2, long block_size) {

    // A place holder so that the file will compile without error.
	// Replace with the correct return value.
    return -1;
}
