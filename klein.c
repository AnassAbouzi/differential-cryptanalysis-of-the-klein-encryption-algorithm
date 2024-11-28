#include <stdio.h>
#include <stdlib.h>

int hex_to_int(char c) {
	//This function converts hex caracteres to int
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

void add_round_key(int *state, int *key) {
	int i;
	for (i = 0; i < 16; i++) {
		state[i] = state[i] ^ key[i];
	}
}

int sbox[] = {7, 4, 0xa, 9, 1, 0xf, 0xb, 0, 0xc, 3, 2, 6, 8, 0xe, 0xd, 5};

void sub_nibbles(int *state) {
	int i;
	for (i = 0; i < 16; i++) {
		state[i] = sbox[state[i]];
	}
}

/*
int sbox_func(int input) {
	//This function uses algebraic normal form (ANF) to represent the KLEIN S-box, but in practice it's more efficient to use a lookup table.
	int x0, x1, x2, x3, y0, y1, y2, y3;
	x0 = input & 1;
	x1 = (input >> 1) & 1;
	x2 = (input >> 2) & 1;
	x3 = (input >> 3) & 1;
	y0 = 1 ^ x0 ^ x1 ^ x3 ^ (x0 & x2) ^ (x1 & x2) ^ (x1 & x3) ^ (x0 & x1 & x2) ^ (x0 & x1 & x3);
	y1 = 1 ^ x0 ^ x2 ^ x3 ^ (x1 & x2) ^ (x1 & x3) ^ (x2 & x3) ^ (x0 & x1 & x3);
	y2 = 1 ^ x1 ^ x2 ^ (x0 & x2) ^ (x1 & x2) ^ (x0 & x3) ^ (x0 & x1 & x2) ^ (x0 & x2 & x3) ^ (x1 & x2 & x3);
	y3 = x1 ^ x3 ^ (x0 & x2) ^ (x0 & x3) ^ (x0 & x1 & x3) ^ (x1 & x2 & x3);
	return y3 * 8 + y2 * 4 + y1 * 2 + y0 * 1;
}
*/

void rotate_nibbles(int *state) {
	//This function rotation each nibble of the state by two bytes to the left
	int i;
	int *temp = (int*) malloc(4 * sizeof(int)); // we use this temporary variable to avoid overiding the first 4 nibbles
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

void mix_column(int* column) {
	//This function applies the rijndael MixColumns operation to a single column
	int* column_1 = (int*) malloc(4 * sizeof(int)); // this variable is just a copy of the initial column
	int* column_2 = (int*) malloc(4 * sizeof(int)); // this variable contains all the elements of the initial column multiplied by 2
	int i, f;
	for (i = 0; i < 4; i++) {
		column_1[i] = column[i];
		f = column[i] >> 7; // We use f to check if the MSB of this element is set to 1
		column_2[i] = (column[i] << 1) ^ (f * 0x1B); // After the multiplication by 2 (left shift) we xor with the value 0x1B because the operations are done in rijndael's galois field (F_2)^8 (all operations are modulo the polynomial x^8 + x^4 + x^3 + x + 1 which we can represent in hex as 0x11B)
	}
	//the following are the calculations done in the mixColumn operation (that we can represent by a matrice multiplication with a column)
	column[0] = column_2[0] ^ column_1[1] ^ column_2[1] ^ column_1[2] ^ column_1[3];
	column[1] = column_1[0] ^ column_2[1] ^ column_1[2] ^ column_2[2] ^ column_1[3];
	column[2] = column_1[0] ^ column_1[1] ^ column_2[2] ^ column_1[3] ^ column_2[3];
	column[3] = column_1[0] ^ column_2[0] ^ column_1[1] ^ column_1[2] ^ column_2[3];
	free(column_1);
	free(column_2);
}

void mix_nibbles(int *state) {
	//this function splits the state into two columns of 4 bytes each then applies the mixColumns operation on them
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

void rotate_tuple(int* tuple) {
	int temp, i;
	temp = tuple[0];
	for (i = 0; i < 3; i++) {
		tuple[i] = tuple[i + 1];
	}
	tuple[3] = temp;
}

void xor_tuples(int* t1, int* t2) {
	int i;
	for (i = 0; i < 4; i++) {
		t1[i] = t1[i] ^ t2[i];
	}
}

void key_derivation(int* key, int round) {
	//This function uses a key and a round number as input to derive the next round key
	int* tuple1 = (int*) malloc(4 * sizeof(int));
	int* tuple2 = (int*) malloc(4 * sizeof(int));
	int i;
	//split the key state into two tuples of 4 bytes each
        for (i = 0; i < 4; i++) {
                tuple1[i] = (key[2 * i] << 4) ^ key[2 * i + 1];
                tuple2[i] = (key[2 * i + 8] << 4) ^ key[2 * i + 9];
        }
	//rotate each tuple to the left by one byte
	rotate_tuple(tuple1);
	rotate_tuple(tuple2);
	//set the right tuple to the xor of the two tuples
	xor_tuples(tuple2, tuple1);
	//xor the third byte of the left tuple with the round counter
	tuple1[2] = tuple1[2] ^ round;
	//apply the sbox to the second and third bytes of the right tuple
	tuple2[1] = sbox[tuple2[1]];
	tuple2[2] = sbox[tuple2[2]];
	//convert the two tuples back to the original key format
	for (i = 0; i < 4; i++) {
                key[2 * i] = tuple1[i] >> 4;
                key[2 * i + 1] = tuple1[i] & 15;
                key[2 * i + 8] = tuple2[i] >> 4;
                key[2 * i + 9] = tuple2[i] & 15;
        }

	free(tuple1);
	free(tuple2);
}

void main(int argc, char *argv[]) {
	char* input = argv[1];
	char* hex_key = argv[2];
	int* state = (int*) malloc(16 * sizeof(int));
	int* key = (int*) malloc(16 * sizeof(int));
	int i;

	for (i = 0; i < 16; i++) {
		state[i] = hex_to_int(input[i]);
		key[i] = hex_to_int(hex_key[i]);
	}

	for (i = 0; i < 12; i++) {
		add_round_key(state, key);
		sub_nibbles(state);
		rotate_nibbles(state);
		mix_nibbles(state);
		key_derivation(key, i + 1);
	}

	for (i = 0; i < 16; i++) {
		printf("%X ", state[i]);
	}

	printf("\n");

	free(key);
	free(state);
}
