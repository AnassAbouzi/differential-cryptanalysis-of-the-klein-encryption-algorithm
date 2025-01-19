#include <stdlib.h>
#include <stdio.h>
#include "klein.h"
#include "time.h"
#include <omp.h>

#define SIZE_PLAIN 16
#define true 1
#define false 0

typedef struct 
{
    int *m1;
    int *m2;
    int *c1;
    int *c2;
    int *cprime;
} Couple;

/**
 * @brief Applying the inverse rijndael MixColumns operation to a single column
 * 
 * @param column 4-byte table
 */
void inv_mix_column(int* column) {
	int* column_tmp = (int*) malloc(4 * sizeof(int)); // this variable is just a copy of the initial column
	for (int i = 0; i < 4; i++) {
		column_tmp[i] = column[i] & 0xff;
		}
	//the following are the calculations done in the mixColumn operation (that we can represent by a matrice multiplication with a column)
	column[0] = (mult11[column_tmp[0]] ^ mult13[column_tmp[1]] ^ mult9[column_tmp[2]] ^ mult14[column_tmp[3]]) & 0xff; // we add "& 0xff" to make sure we only operate on the least significant 8 bits
	column[1] = (mult14[column_tmp[0]] ^ mult11[column_tmp[1]] ^ mult13[column_tmp[2]] ^ mult9[column_tmp[3]]) & 0xff;
	column[2] = (mult9[column_tmp[0]] ^ mult14[column_tmp[1]] ^ mult11[column_tmp[2]] ^ mult13[column_tmp[3]]) & 0xff;
	column[3] = (mult13[column_tmp[0]] ^ mult9[column_tmp[1]] ^ mult14[column_tmp[2]] ^ mult11[column_tmp[3]]) & 0xff;
	free(column_tmp);
}

/**
 * @brief splits the state into two columns of 4 bytes each then applies the inverse mixColumns operation on them
 * 
 * @param state the current state
 */
void inv_mix_nibbles(int *state) {
	//this function 
	int* col1 = (int*) malloc(4 * sizeof(int));
	int* col2 = (int*) malloc(4 * sizeof(int));
	int i;
	for (i = 0; i < 4; i++) {
		col1[i] = (state[2 * i] << 4) ^ state[2 * i + 1]; // for each element in the column we concatenate two nibbles by shifting the first by 4 bits to the left the xoring it with the second one
		col2[i] = (state[2 * i + 8] << 4) ^ state[2 * i + 9]; // same thing we did for the first column but we start at the 8th nibble (second half)
	}

	inv_mix_column(col1);
	inv_mix_column(col2);

	//after the mixColumn operation we convert the two columns back to our original state format
	for (i = 0; i < 4; i++) {
		state[2 * i] = col1[i] >> 4; //extract the high nibble (most significant 4 bits)
		state[2 * i + 1] = col1[i] & 15; // extract the low nibble (least significant 4 bits)
		state[2 * i + 8] = col2[i] >> 4;
		state[2 * i + 9] = col2[i] & 15;
	}

	free(col1);
	free(col2);
}

/**
 * @brief rotate each nibble of the state by two bytes to the right
 * 
 * @param state the current state
 */
void inv_rotate_nibbles(int *state) {
	int i;

	// we use this temporary variable to avoid overiding the first 4 nibbles
	int *temp = (int*) malloc(4 * sizeof(int)); 
	for (i = 0; i < 4; i++) {
		temp[i] = state[i + 12];
	}
	for (i = 0; i < 12; i++) {
		state[i + 4] = state[i];
	}
	for (i = 0; i < 4; i++) {
		state[i] = temp[i];
	}
	free(temp);
}

/**
 * @brief Klein inverse round function
 * 
 * @param state current round state
 * @param key current round subkey
 * @param round current round
 */
void inv_round_function(int* state, int* key, int round) {
	inv_mix_nibbles(state);
	inv_rotate_nibbles(state);
	sub_nibbles(state);
	add_round_key(state, key);
}

/**
 * @brief Function qui genere un plain aleatoir
 * 
 * @param out plain
 */
void generateCouple(Couple* c, int* diff, int* key){
    if (c->m1 == NULL || c->m2 == NULL || c->c1 == NULL || c->c2 == NULL || diff == NULL || key == NULL) {
        fprintf(stderr, "Error: One or more pointers are NULL\n");
        return;
    }
	for (int i = 0; i < SIZE_PLAIN ; i++)
    {
        c->m1[i] = rand() % 16;
    }
	xor_nibbles(c->m2, c->m1, diff, SIZE_PLAIN);
    klein_cipher(c->c1, c->m1, key);
	klein_cipher(c->c2, c->m2, key);
}

int differential_pathway_check(int *state){
    for (int i = 0; i <= SIZE_PLAIN / 2; i++)
    {
        if (state[2 * i] != 0){
            return false;
        }
    }
    return true;
    
}

void show(int* input, int lenght) {
	for (int i = 0; i < lenght; i++) {
		printf("%d ", input[i]);
	}
	printf("\n");
}

int main(int argc, char ** argv){
	srand(time(NULL));
	printf("Attack on %d-klein\n", ROUNDS);
	int * B_VALUE = (int*) malloc(16 * sizeof(int));
	int * KEY = (int*) malloc(16 * sizeof(int));
	for (int i = 0; i < 16; i++) {
		B_VALUE[i] = 0;
		KEY[i] = i;
	}
	B_VALUE[5] = 0xb;
	
	printf("Key value: ");
	show(KEY, 16);

    Couple* c;
    int counter = 0;
	int found = false;

	#pragma omp parallel shared(found, counter) private(c)
	{
		c = malloc(sizeof(Couple));
		c->m1 = (int*) malloc(16 * sizeof(int));
		c->m2 = (int*) malloc(16 * sizeof(int));
		c->c1 = (int*) malloc(16 * sizeof(int));
		c->c2 = (int*) malloc(16 * sizeof(int));
		c->cprime = (int*) malloc(16 * sizeof(int));

		while (found==false)
		{
			#pragma omp atomic
			counter++;
			// generation of plaintext couples verifying : m1 + m2 = B_VALUE
			generateCouple(c, B_VALUE, KEY);
			// inverse mix nibbles operation of c1 + c2
			inv_mix_nibbles(c->c1);
			inv_mix_nibbles(c->c2);
			xor_nibbles(c->cprime, c->c1,c->c2,SIZE_PLAIN);

			#pragma omp critical
			{
				// verification of the format of c' (result after inversing the mix nibbles operation of c1 + c2)
				if (differential_pathway_check(c->cprime)==true){
					found = true;
					printf("Number of iterations: %d\n", counter);
					show(c->cprime, SIZE_PLAIN);
				}
			}
		}
		free(c->cprime);
		free(c->m1);
		free(c->m2);
		free(c->c1);
		free(c->c2);
		free(c);
	}

	free(B_VALUE);
	free(KEY);
    return 0;
}