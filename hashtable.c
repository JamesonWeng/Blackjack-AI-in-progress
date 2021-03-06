/*
 * hashtable.c
 *
 *  Created on: Jan 17, 2016
 *      Author: Jameson
 */

#include "hashtable.h"

/********************************************
 	 HASH TABLE FUNCTIONS
 ********************************************/

// listInsert: insert into a linked list, overwrites duplicate keys
nodeType *listInsert(nodeType *head, unsigned long long int key, unsigned long int value) {
	if (!head) {
		head = malloc(sizeof(nodeType));
		head->key = key;
		head->value = value;
		head->next = NULL;
	}
	else if (head->key != key)
		head->next = listInsert (head->next, key, value);
	else
		head->value = value;

	return head;
}

nodeType *listPrepend(nodeType *lst, unsigned long long int key, unsigned long int value) {
	nodeType *head = malloc(sizeof(nodeType));
	head->key = key;
	head->value = value;
	head->next = lst;
	return head;
}

int listLength (nodeType *lst) {
	int len = 0;
	while (lst) {
		len++;
		lst = lst->next;
	}
	return len;
}

// listLookup: given a key, finds the node containing the response (either 0 or 1)
// if no such node (should not occur), will return -1
int listLookup(nodeType *current, unsigned long long int key, int digit) {
	while (current) {
		if (current->key == key) // get the digit from the right
			return getDigit(current->value, digit);

		current = current->next;
	}
	return -1;
}

// listPrint: prints out the contents of a list
void listPrint(nodeType *head) {
	nodeType *current = head;
	while (current) {
		printf ("(%llu,%lu) ", current->key, current->value);
		current = current->next;
	}
	printf ("\n\n");
}

// freeList: frees all the pointers in a linked list
void listFree(nodeType *head) {
	nodeType *current = head;
	while (current) {
		nodeType *temp = current;
		current = current->next;
		free(temp);
	}
}

// handToKey: computes the key for a given hand
// Treats each card in the hand as a digit in base 11, and returns the decimal interpretation of the number
unsigned long long int handToKey(handType *hand) {
	unsigned long long int key = 0;
	static const int base = 11;

	for (int i = 0; i < hand->handSize; i++) {
		if (hand->cards[i].rank < 9)
			key = key * base + hand->cards[i].rank + 1;
		else // ten to king have same value in blackjack, so we won't differentiate for the key
			key = key * base + 10;
	}

	return key;
}

// handToIndex: finds the index in the hash table for a given hand
int handToIndex(handType *hand) {
	return (int) (handToKey(hand) % HASH_ARRAY_SIZE);
}

void hashTableInsert(hashTableType *table, handType *hand, unsigned long int response) {
	int index = handToIndex(hand);
	unsigned long long int key = handToKey(hand);
	table->heads[index] = listInsert(table->heads[index], key, response);
}

int hashTableLookup(hashTableType *table, handType *hand, int digit) {
	int index = handToIndex(hand);
	unsigned long long int key = handToKey(hand);

	return listLookup(table->heads[index], key, digit);
}

static void hashTableInitAllKeys(hashTableType *table, handType *hand, int ranks[NUM_RANKS], int curRank) {
	// initially choose a random response for AI
	unsigned long int response = 0;
	for (int i = 0; i < NUM_APPRECIABLE_RANKS; i++)
		response = response * 10 + randInt(0,1);

	hashTableInsert(table, hand, response);

	if (ranks[curRank] >= NUM_SUITS) {
		curRank += 1;
	}

	for (int i = curRank; i < NUM_APPRECIABLE_RANKS; i++) {
		hand->cards[hand->handSize].rank = i;
		hand->handSize += 1;
		ranks[i] += 1;

		handFindSum(hand);
		if (hand->sum > 21) {
			hand->handSize -= 1;
			ranks[i] -=1;
			break;
		}

		hashTableInitAllKeys(table, hand, ranks, i);

		hand->handSize -= 1;
		ranks[i] -= 1;
	}
}

hashTableType *hashTableInit() {
	int *ranks = malloc(NUM_APPRECIABLE_RANKS * sizeof(int)); // records number of cards of each rank in hand
	for (int i = 0; i < NUM_APPRECIABLE_RANKS; i++)
		ranks[i] = 0;

	handType *hand = handInit();

	hashTableType *table = malloc(sizeof(hashTableType));
	for (int i = 0; i < HASH_ARRAY_SIZE; i++)
		table->heads[i] = NULL;

	for (int i = 0; i < NUM_APPRECIABLE_RANKS; i++) {
		hand->cards[hand->handSize].rank = i; // we don't care about suit here
		hand->handSize += 1;
		ranks[i] += 1;

		for (int j = i; j < NUM_APPRECIABLE_RANKS; j++) {
			hand->cards[hand->handSize].rank = j;
			hand->handSize += 1;
			ranks[j] += 1;

			hashTableInitAllKeys(table, hand, ranks, j);	 // guarantees that hand will have at least two cards

			hand->handSize -= 1;
			ranks[j] -= 1;
		}

		hand->handSize -=1;
		ranks[i] -= 1;
	}

	handFree(hand);
	free(ranks);
	return table;
}

void hashTableToFile (hashTableType *table, FILE *f) {
	nodeType *current;
	for (int i = 0; i < HASH_ARRAY_SIZE; i++) {
		current = table->heads[i];
		fprintf(f, "Index %i: ", i);

		while (current) {
			fprintf(f, "(%llu,%lu) ", current->key, current->value);
			current = current->next;
		}

		fprintf(f, "\n\n");
	}
}

void hashTableFree(hashTableType *table) {
	for (int i = 0; i < HASH_ARRAY_SIZE; i++) {
		listFree(table->heads[i]);
	}
	free(table);
}

