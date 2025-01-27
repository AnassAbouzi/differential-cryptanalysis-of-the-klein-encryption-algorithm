#include <stdio.h>
#include <stdlib.h>

#ifndef LIST_H
#define LIST_H

typedef struct Node {
    int* data; //key
    struct Node* next; //the next node
} Node;

/**
 * @brief Function to create a new node
 * 
 * @param data 
 * @return Node* 
 */
Node* create_node(int* data) {
    Node* new_node = (Node*)malloc(sizeof(Node));
    new_node->data = (int *) malloc(16 * sizeof(int));
    if (new_node == NULL) {
        fprintf(stderr, "Erreur d'allocation mémoire\n");
        exit(EXIT_FAILURE);
    }
    memcpy(new_node->data, data, 16*sizeof(int));
    //new_node->data = data;
    new_node->next = NULL;
    return new_node;
}

/**
 * @brief function to add node into the list
 * 
 * @param head_ref 
 * @param new_data 
 */
void append(Node** head_ref, int* new_data) {
    Node* new_node = create_node(new_data);
    Node* last = *head_ref;

    if (*head_ref == NULL) {
        *head_ref = new_node;
        return;
    }

    while (last->next != NULL) {
        last = last->next;
    }
    last->next = new_node;
}

/**
 * @brief Function to print list
 * 
 * @param node 
 */
void print_list(Node* node) {
    while (node != NULL) {
        show(node->data, 16);
        node = node->next;
    }
}

/**
 * @brief Search a value in the list
 * 
 */
 int isInList(Node* node, int data){
    while (node != NULL) {
        if (node->data==data)
            return 1;
        node = node->next;
    }
    return 0;
 }

/**
 * @brief Function to free the memory
 * 
 * @param head 
 */
void free_list(Node* head) {
    Node* tmp;

    while (head != NULL) {
        tmp = head;
        head = head->next;
        free(tmp);
    }
}
#endif
/**
 * @brief Function to get the length of the list
 * 
 * @param node 
 * @return int 
 */
int len(Node* node) {
    int count = 1;
    while(node->next != NULL) {
        count++;
    }
    return count;
}

void show(int* input, int lenght) {
	for (int i = 0; i < lenght; i++) {
		printf("%d ", input[i]);
	}
	printf("\n");
}