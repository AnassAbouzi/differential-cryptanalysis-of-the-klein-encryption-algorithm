#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include "klein.h"
#include "time.h"
#include <omp.h>

#define SIZE_PLAIN 16
#define true 1
#define false 0
#define NB_TEST 6

typedef struct 
{
    int *m1;
    int *m2;
    int *c1;
    int *c2;
    int *cprime;
} Couple;

void initializeACouple(Couple* c) {
    c->m1 = (int*) malloc(16 * sizeof(int));
    c->m2 = (int*) malloc(16 * sizeof(int));
    c->c1 = (int*) malloc(16 * sizeof(int));
    c->c2 = (int*) malloc(16 * sizeof(int));
    c->cprime = (int*) malloc(16 * sizeof(int));
}

void freeACouple(Couple* c) {
	free(c->m1);
	free(c->m2);
	free(c->c1);
	free(c->c2);
	free(c->cprime);
	free(c);
}

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

void show(int* input, int lenght) {
	for (int i = 0; i < lenght; i++) {
		printf("%d ", input[i]);
	}
	printf("\n");
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

/**
 * @brief Check if the differential pathway is valid
 * 
 * @param state State to be checked
 * @return int Return 1 if the differential pathway is valid, 0 otherwise
 */
int differential_pathway_check(int *state){
	
    for (int i = 0; i < (SIZE_PLAIN / 2); i++)
    {
        if (state[2 * i] != 0){
	        return false;
        }
    }
    return true;
    
}


/**
 * @brief Copy the values of a couple to another
 * 
 * @param source Couple to be copied
 * @param dest Destination couple
 */
void copyCouple(Couple *source, Couple *dest) {
       // Copier les valeurs des membres de la structure
    for (int i = 0; i < 16; i++) {
        dest->m1[i] = source->m1[i];
        dest->m2[i] = source->m2[i];
        dest->c1[i] = source->c1[i];
        dest->c2[i] = source->c2[i];
        dest->cprime[i] = source->cprime[i];
    }
}

void generateAValideCouple(Couple** valideCouples, int index, int* B_VALUE, int* KEY){
	int nb_threads =  omp_get_max_threads();

	Couple *couples = (Couple*) malloc(nb_threads * sizeof(Couple));

	//allocate memory for each couple
	for (int i = 0; i < nb_threads; i++) {
        couples[i].m1 = (int*) malloc(16 * sizeof(int));
        couples[i].m2 = (int*) malloc(16 * sizeof(int));
        couples[i].c1 = (int*) malloc(16 * sizeof(int));
        couples[i].c2 = (int*) malloc(16 * sizeof(int));
        couples[i].cprime = (int*) malloc(16 * sizeof(int));
    }

	int counter = 0;
	//the index of the thread that found the correct couple
	int found = -1;

	#pragma omp parallel shared(found, counter)
	{
		int i = omp_get_thread_num();
		while (found==-1)
		{
			#pragma omp atomic
			counter++;
			// generation of plaintext couples verifying : m1 + m2 = B_VALUE
			generateCouple(&couples[i], B_VALUE, KEY);
			// inverse mix nibbles operation of c1 + c2
			inv_mix_nibbles(couples[i].c1);
			inv_mix_nibbles(couples[i].c2);
			xor_nibbles(couples[i].cprime,
						couples[i].c1,
						couples[i].c2,SIZE_PLAIN);

				// verification of the format of c' (result after inversing the mix nibbles operation of c1 + c2)
				if (differential_pathway_check(couples[i].cprime)==true){
					#pragma omp critical
					{
						found = i;
					}
				}
			
		}
	}
	copyCouple(&couples[found], valideCouples[index]);
	

	for (int i = 0; i < nb_threads; i++) {
		free(couples[i].m1);
		free(couples[i].m2);
		free(couples[i].c1);
		free(couples[i].c2);
		free(couples[i].cprime);
	}
	free(couples);
}

int keyTest(Couple *validateCouple, int *candidate_key);

int main(int argc, char **argv)
{
    srand(time(NULL));
	printf("Attack on %d-klein\n", ROUNDS);
	int * b_value = (int*) malloc(16 * sizeof(int));
	int * key = (int*) malloc(16 * sizeof(int));
	int * key_tild = (int*) malloc(16 * sizeof(int));
	for (int i = 0; i < 16; i++) {
		b_value[i] = 0;
		key[i] = i;
	}
	b_value[5] = 0xb;
	
	// generation of the key_tild
	memcpy(key_tild, key, 16 * sizeof(int));
	key_derivation(key_tild, ROUNDS);
	inv_mix_nibbles(key_tild);
	inv_rotate_nibbles(key_tild);

	printf("Key value: ");
	show(key, 16);
	
	printf("Key tild: ");
	show(key_tild, 16);
	

	//generate validate couples
	Couple **validateCouples = (Couple**) malloc(sizeof(Couple*) * NB_TEST);
	printf("valide couples : ");
	for (int i = 0; i < NB_TEST; i++)
	{
		validateCouples[i] = (Couple*) malloc(sizeof(Couple));
		initializeACouple(validateCouples[i]);
		generateAValideCouple(validateCouples,i, b_value, key);
		show(validateCouples[i]->cprime, SIZE_PLAIN);
	}
	printf("_________\n");
	int* candidate_key = (int*) malloc(16 * sizeof(int));
	
	int* origin_key = (int*) malloc(16 * sizeof(int));
	// int prob_keys_l = 0;

	
	for (int j = 0; j < 16; j++) {
			candidate_key[j] = key_tild[j];
	}

	int i;
	int found = false;
	int flag;
	// search validate key
	for (uint64_t k = 0; k < (1ULL << 12); k++) {


		candidate_key[15] = k & 0xF;
		candidate_key[13] = (k >> 4) & 0xF;
		candidate_key[11] = (k >> 8) & 0xF;
		// candidate_key[9] = (k >> 12) & 0xF;
		// candidate_key[7] = (k >> 16) & 0xF;
		//candidate_key[5] = (k >> 20) & 0xF;
		//candidate_key[3] = (k >> 24) & 0xF;
		//candidate_key[1] = (k >> 28) & 0xF;

		//show(candidate_key, 16);

		
		
		for (i = 0; i < NB_TEST; i++){
            if (!keyTest(validateCouples[i], candidate_key))
			{
				printf("faux");
				break;
			}
			// show(candidate_key, 16);
        }
		
		if (i == NB_TEST) {
			found = true;
			//printf("eee");
			show(candidate_key, 16);
		}
		
	}
	// memcpy(origin_key, list->data, 16 * sizeof(int));
	
	// key reconstruction
	//rotate_nibbles(origin_key);
	//mix_nibbles(origin_key);


	for (int j = 0; j < NB_TEST; j++) {
		freeACouple(validateCouples[j]);
	}
	
	free(origin_key);
	free(b_value);
	free(key_tild);
	free(key);
    return 0;
}
/**
 * @brief Check if the key is valid
 * 
 * @param validateCouple 
 * @param candidate_key 
 * @return int 
 */
int keyTest(Couple *validateCouple, int *candidate_key)
{
	int tmp1[16], tmp2[16];

    inv_rotate_nibbles(validateCouple->c1);
    inv_rotate_nibbles(validateCouple->c2);
    xor_nibbles(tmp1, validateCouple->c1, candidate_key, 16);
    xor_nibbles(tmp2, validateCouple->c2, candidate_key, 16);
    sub_nibbles(tmp1);
    sub_nibbles(tmp2);
    xor_nibbles(tmp1, tmp1, tmp2, 16);
	int res = differential_pathway_check(tmp1);
    return res;
}
