#include <stdio.h>


/* Computes hash for data provided via STDIN and stores it in hash_val,
*  an array of char. The hash implementation is based on xor and the
*  computed hash is at most block_size bytes. 
*/

void hash(char *hash_val, long block_size) { 
 int i;
		for(i = 0; i < block_size; i++) {//initializing block_size bytes of hash_val to value 0
			hash_val[i] = '\0';
			} 
			
 char store_char;
 int j = 0;		
		while(scanf("%c", &store_char) != EOF ){
				if(j < block_size) { 
						hash_val[j]= (hash_val[j])^store_char; 
						j++;
					} 
				else { // j == block_size, computation wraps around  
						j = 0;			
						hash_val[j] = (hash_val[j])^store_char;
						j++;
				   }
			}
}

/* Compares two hash values hash1 and hash2, which are arrays of char,
*  and returns the first index at which the hash values don't match. If they 
*  match completely the function returns block_size.
*/

int check_hash(const char *hash1, const char *hash2, long block_size) {
 int i;
		for(i = 0; i < block_size; i++){
				if(hash1[i] != hash2[i]){
				return i;
				}
		   } //for loop completed, hash values are the same 
		
		return block_size;
}
