#include <stdio.h>
#include <stdlib.h>
#include "klein.h"

/**
 * @brief Klein computation
 * 
 * @param argc 
 * @param argv 1 - Plaintext (hexae) 2 - key (Hexa) 
 */
 int main(int argc, char *argv[]) {

 	//check the arguments
 	if (argc!=3){
 		fprintf(stderr, "Arguments missing\n");
 		return 1;
 	}

 	char* input = argv[1];
 	char* hex_key = argv[2];

 	int* state = (int*) malloc(16 * sizeof(int));
 	int* key = (int*) malloc(16 * sizeof(int));
 	int* out = (int*) malloc(16 * sizeof(int));
 	int i;

 	//convert hexa to int
 	for (i = 0; i < 16; i++) {
 		state[i] = hex_to_int(input[i]);
 		key[i] = hex_to_int(hex_key[i]);
 	}

 	//cipher engine
 	klein_cipher(out, state, key);

 	//print result
 	for (i = 0; i < 16; i++) {
 		printf("%X ", state[i]);
 	}

 	printf("\n");

 	free(out);
 	free(key);
 	free(state);
 	return 0;
 }