#include <stdio.h>
#include <stdlib.h>

#ifndef KLEIN_H
#define KLEIN_H

#define ROUNDS 5

extern const int sbox[];

//rijndael multiplication lookup tables
extern const int mult2[];
extern const int mult3[]; 
extern const int mult9[]; 
extern const int mult11[]; 
extern const int mult13[]; 
extern const int mult14[];




/**
 * @brief Converts hex caracteres to int
 * 
 * @param c the hexa caractere
 * @return int the int result
 */
int hex_to_int(char c);

void klein_cipher(int* out, int* state, int*key);

/**
 * @brief Compute a xor between two nibbles
 * 
 * @param out 
 * @param nibble1 
 * @param nibble2 
 * @param l the nibble's length
 */
void xor_nibbles(int * out, int *nibble1, int *nibble2, int l);

/**
 * @brief AddRoundKey operation
 * 
 * @param state the state 
 * @param key the round key
 */
void add_round_key(int *state, int *key);


/**
 * @brief SubByte operation on nibbles
 * 
 * @param state the current state
 */
void sub_nibbles(int *state);

/**
 * @brief rotate each nibble of the state by two bytes to the left
 * 
 * @param state the current state
 */
void rotate_nibbles(int *state);

/**
 * @brief Applying the rijndael MixColumns operation to a single column
 * 
 * @param column 4-byte table
 */
void mix_column(int* column);

/**
 * @brief splits the state into two columns of 4 bytes each then applies the mixColumns operation on them
 * 
 * @param state the current state
 */
void mix_nibbles(int *state);

/**
 * @brief rotate each nibble of the state by only 1 byte to the left
 * 
 * @param nibble the nibble
 */
void rotate_nibbles_by_1(int *nibble);

/**
 * @brief master key derivation function 
 * 
 * @param key round key n-1
 * @param round current round key
 */
void key_derivation(int *key, int round);

/**
 * @brief Klein round function
 * 
 * @param state current round state
 * @param key current round subkey
 * @param round current round
 */
void round_function(int* state, int* key, int round);

#endif