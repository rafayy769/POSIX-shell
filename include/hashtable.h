/**  
 * @file hashtable.h Hashtable implementation
 * @author Abdul Rafay (24100173@lums.edu.pk)
 * @brief Implementation of a hashtable to be used by aliases. Thought about implementing a generic hashtable, but I would go with the specific one for performance reasons.
 * @version 0.1
 * @date 2023-06-27
 * @copyright Copyright (c) 2023
 * 
 */

#ifndef HASHTABLE_H
#define HASHTABLE_H

#include "utils.h"

#define NUMBER_OF_BUCKETS 101

/**
 * @brief The struct represents a key-value pair in the hashtable.
 * 
 * Since the hashtable is implemented using chaining, the struct also contains a pointer to the next entry in the chain. Both key and value are strings.
 * 
 */
typedef struct htEntry
{
    char *key;              //< The key of the entry
    char *value;            //< The value of the entry
    struct htEntry *next;   //< Pointer to the next entry in the chain
} htEntry;

/**
 * @brief The struct represents the hashtable itself. It contains an array of linked lists, which are used to implement chaining.
 * 
 */
typedef struct hashtable
{
    int size;                       //< The size of the hashtable
    struct linkedList **buckets;    //< The array of linked lists
} hashtable;

/**
 * @brief The struct represents a linked list. It contains pointers to the head and tail of the list.
 * 
 */
typedef struct linkedList
{
    struct htEntry *head;  //< Pointer to the head of the list
    struct htEntry *tail;  //< Pointer to the tail of the list
} linkedList;


/**
 * @brief Creates a Hashtable object.
 * 
 * The function allocates memory for the hashtable and initializes the buckets. All the buckets are initialized to NULL.
 * 
 * @param size The size of the hashtable
 * @return hashtable* Pointer to the hashtable
 */
hashtable *createHashtable(int size);

/**
 * @brief Deletes a hashtable object.
 * 
 * The function frees the memory allocated for the hashtable and all the entries in the hashtable.
 * 
 * @param ht Pointer to the hashtable
 */
void deleteHashtable(hashtable* ht);

/**
 * @brief Standard set function for the hashtable. It takes a key and a value and sets the value for the given key.
 * 
 * @param ht The hashtable to be used
 * @param key Key to be used
 * @param value Value to be set
 */
void set(hashtable* ht, const char* key, const char* value);

/**
 * @brief Standard get function for the hashtable. It takes a key and returns the value for the given key. If the key is not found, it returns NULL.
 * 
 * The function first calculates the hash of the key and then traverses the linked list at the index of the hash to find the key. If the key is found, it returns the value, otherwise it returns NULL. Returns a pointer to the stored value instead of copying it.
 * 
 * @param ht Hashtable to be used.
 * @param key Key to be used.
 * @return char* Value for the given key.
 */
char* get(hashtable* ht, const char* key);

/**
 * @brief Prints the hashtable in the following format: `"%s=%s\n", key, value`
 * 
 * @param ht Hashtable to be printed
 */
void printHashtable(hashtable* ht);

#endif // HASHTABLE_H