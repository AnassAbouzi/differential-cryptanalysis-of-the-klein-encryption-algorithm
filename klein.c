#include <stdio.h>
#include <stdlib.h>

#define ROUNDS 12

const int sbox[] = {7, 4, 0xa, 9, 1, 0xf, 0xb, 0, 0xc, 3, 2, 6, 8, 0xe, 0xd, 5};

/**
 * @brief Converts hex caracteres to int
 * 
 * @param c the hexa caractere
 * @return int the int result
 */
int hex_to_int(char c) {
	if (c >= '0' && c <= '9') {
		return c - '0';
	} else if (c >= 'A' && c <= 'F') {
		return c - 'A' + 10;
	} else if (c >= 'a' && c <= 'f') {
		return c - 'a' + 10;
	} else {
		return -1;
	}
}

/**
 * @brief Compute a xor between two nibbles
 * 
 * @param nibble1 
 * @param nibble2 
 * @param l the nibble's length
 */
void xor_nibbles(int *nibble1, int *nibble2, int l) {
	int i;
	for (i = 0; i < l; i++) {
		nibble1[i] = nibble1[i] ^ nibble2[i];
	}
}

/**
 * @brief AddRoundKey operation
 * 
 * @param state the state 
 * @param key the round key
 */
void add_round_key(int *state, int *key) {
	xor_nibbles(state, key, 16);
}


/**
 * @brief SubByte operation on nibbles
 * 
 * @param state the current state
 */
void sub_nibbles(int *state) {
	int i;
	for (i = 0; i < 16; i++) {
		state[i] = sbox[state[i]];
	}
}

/**
 * @brief rotate each nibble of the state by two bytes to the left
 * 
 * @param state the current state
 */
void rotate_nibbles(int *state) {
	int i;

	// we use this temporary variable to avoid overiding the first 4 nibbles
	int *temp = (int*) malloc(4 * sizeof(int)); 
	for (i = 0; i < 4; i++) {
		temp[i] = state[i];
	}
	for (i = 0; i < 12; i++) {
		state[i] = state[i + 4];
	}
	for (i = 0; i < 4; i++) {
		state[i + 12] = temp[i];
	}
	free(temp);
}

/**
 * @brief Applying the rijndael MixColumns operation to a single column
 * 
 * @param column 4-byte table
 */
void mix_column(int* column) {
	int* column_1 = (int*) malloc(4 * sizeof(int)); // this variable is just a copy of the initial column
	int* column_2 = (int*) malloc(4 * sizeof(int)); // this variable contains all the elements of the initial column multiplied by 2
	int i, f;
	for (i = 0; i < 4; i++) {
		column_1[i] = column[i];
		f = column[i] >> 7; // We use f to check if the MSB of this element is set to 1
		column_2[i] = (column[i] << 1) ^ (f * 0x1B); // After the multiplication by 2 (left shift) we xor with the value 0x1B because the operations are done in rijndael's galois field (F_2)^8 (all operations are modulo the polynomial x^8 + x^4 + x^3 + x + 1 which we can represent in hex as 0x11B)
	}
	//the following are the calculations done in the mixColumn operation (that we can represent by a matrice multiplication with a column)
	column[0] = (column_2[0] ^ column_1[1] ^ column_2[1] ^ column_1[2] ^ column_1[3]) & 0xff; // we add "& 0xff" to make sure we only operate on the least significant 8 bits
	column[1] = (column_1[0] ^ column_2[1] ^ column_1[2] ^ column_2[2] ^ column_1[3]) & 0xff;
	column[2] = (column_1[0] ^ column_1[1] ^ column_2[2] ^ column_1[3] ^ column_2[3]) & 0xff;
	column[3] = (column_1[0] ^ column_2[0] ^ column_1[1] ^ column_1[2] ^ column_2[3]) & 0xff;
	free(column_1);
	free(column_2);
}

/**
 * @brief splits the state into two columns of 4 bytes each then applies the mixColumns operation on them
 * 
 * @param state the current state
 */
void mix_nibbles(int *state) {
	//this function 
	int* col1 = (int*) malloc(4 * sizeof(int));
	int* col2 = (int*) malloc(4 * sizeof(int));
	int i;
	for (i = 0; i < 4; i++) {
		col1[i] = (state[2 * i] << 4) ^ state[2 * i + 1]; // for each element in the column we concatenate two nibbles by shifting the first by 4 bits to the left the xoring it with the second one
		col2[i] = (state[2 * i + 8] << 4) ^ state[2 * i + 9]; // same thing we did for the first column but we start at the 8th nibble (second half)
	}

	mix_column(col1);
	mix_column(col2);

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
 * @brief rotate each nibble of the state by only 1 byte to the left
 * 
 * @param nibble the nibble
 */
void rotate_nibbles_by_1(int *nibble) {
	int tmp1, tmp2, i;
	tmp1 = nibble[0];
	tmp2 = nibble[1];
	for (i = 0; i < 6; i++) {
		nibble[i] = nibble[i + 2];
	}
	nibble[6] = tmp1;
	nibble[7] = tmp2;
}

/**
 * @brief master key derivation function 
 * 
 * @param key round key n-1
 * @param round current round key
 */
void key_derivation(int *key, int round) {
	// seperating the key into two tuples
	int *tuple1 = (int*) malloc(8 * sizeof(int));
	int *tuple2 = (int*) malloc(8 * sizeof(int));
	int i;
	for (i = 0; i < 8; i++) {
		tuple1[i] = key[i];
		tuple2[i] = key[i + 8];
	}

	//rotating each tuple to the left by 1 byte
	rotate_nibbles_by_1(tuple1);
	rotate_nibbles_by_1(tuple2);

	//set the right tuple to the xor of the two tuples
        xor_nibbles(tuple2, tuple1, 8);
        //set the value of left tuple to that of the right tuple (we achieve th>
        xor_nibbles(tuple1, tuple2, 8);
        //xor the third byte of the left tuple with the round counter
        tuple1[5] = tuple1[5] ^ round;
        //apply the sbox to the second and third bytes of the right tuple
        tuple2[2] = sbox[tuple2[2]];
        tuple2[3] = sbox[tuple2[3]];
	tuple2[4] = sbox[tuple2[4]];
	tuple2[5] = sbox[tuple2[5]];

	//convert the two tuples back to the original key format
	for (i = 0; i < 8; i++) {
		key[i] = tuple1[i];
		key[i + 8] = tuple2[i];
	}

	free(tuple1);
	free(tuple2);
}

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
	int i;

	//convert hexa to int
	for (i = 0; i < 16; i++) {
		state[i] = hex_to_int(input[i]);
		key[i] = hex_to_int(hex_key[i]);
	}

	//cipher engine
	for (i = 1; i <= ROUNDS; i++) {
		add_round_key(state, key);
		sub_nibbles(state);
		rotate_nibbles(state);
		mix_nibbles(state);
		key_derivation(key, i);
	}

	add_round_key(state, key);

	//print result
	for (i = 0; i < 16; i++) {
		printf("%X ", state[i]);
	}

	printf("\n");

	free(key);
	free(state);
	return 0;
}
