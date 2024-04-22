#include "hashtable.h"

// Hash function for strings (djb2)
static unsigned long hash(const char *str)
{
    unsigned long hash = 5381;
    int c;

    while ((c = *str++))
    {
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
    }

    return hash;
}

// creates a new entry
static struct htEntry *createEntry(const char *key, const char *value)
{
    struct htEntry *entry = (struct htEntry *)malloc(sizeof(struct htEntry));
    if (entry)
    {
        entry->key = COPY(key);
        entry->value = COPY(value);
        entry->next = NULL;
    }
    return entry;
}

// traverses the linked list and returns the entry with the given key
static struct htEntry *getEntry(struct linkedList *list, const char *key)
{
    struct htEntry *curr = list->head;
    while (curr != NULL)
    {
        if (strcmp(curr->key, key) == 0)
        {
            return curr;
        }
        curr = curr->next;
    }
    return NULL;
}

void cleanUpEntry(struct htEntry *entry)
{
    if (!entry)
        return;

    if (entry->key)
        free(entry->key);
    if (entry->value)
        free(entry->value);
    free(entry);
}

// insert a new entry into the linked list
static void insertEntry(struct linkedList *list, struct htEntry *entry)
{
    if (list->head == NULL)
    {
        list->head = entry;
        list->tail = entry;
    }
    else
    {
        list->tail->next = entry;
        list->tail = entry;
    }
}

// traverses the linked list and sets the entry with the given key to the given value, if the key exists, else it creates a new entry
static void setEntry(struct linkedList *list, const char *key, const char *value)
{
    struct htEntry *entry = getEntry(list, key);
    if (entry)
    {
        if (entry->value)
            free(entry->value);
        entry->value = COPY(value);
    }
    else
    {
        entry = createEntry(key, value);
        if (entry)
            insertEntry(list, entry);
    }
}

// clean up a linked list
static void cleanUpList(struct linkedList *list)
{
    if (!list)
        return;
    
    struct htEntry *curr = list->head;
    while (curr != NULL)
    {
        struct htEntry *next = curr->next;
        cleanUpEntry(curr);
        curr = next;
    }
    free(list);
}

// the set function for the hashtable
void set(struct hashtable *table, const char *key, const char *value)
{
    int index = hash(key) % table->size;
    setEntry(table->buckets[index], key, value);
}

// the get function for the hashtable
char *get(struct hashtable *table, const char *key)
{
    int index = hash(key) % table->size;
    struct htEntry *entry = getEntry(table->buckets[index], key);
    if (entry)
        return entry->value;
    
    return NULL;
}

// creates a new hashtable
struct hashtable *createHashtable(int size)
{
    struct hashtable *table = (struct hashtable *)malloc(sizeof(struct hashtable));
    if (table)
    {
        table->size = size;
        table->buckets = (struct linkedList **)malloc(sizeof(struct linkedList *) * size);
        if (table->buckets)
        {
            for (int i = 0; i < size; i++)
            {
                table->buckets[i] = (struct linkedList *)malloc(sizeof(struct linkedList));
                if (table->buckets[i])
                {
                    table->buckets[i]->head = NULL;
                    table->buckets[i]->tail = NULL;
                }
            }
        }
    }
    return table;
}

// deletes the hashtable
void deleteHashtable(struct hashtable *table)
{
    if (!table)
        return;
    
    for (int i = 0; i < table->size; i++)
    {
        cleanUpList(table->buckets[i]);
    }
    free(table->buckets);
    free(table);
}

// prints the hashtable
void printHashtable(struct hashtable *table)
{
    if (!table)
        return;
    
    for (int i = 0; i < table->size; i++)
    {
        struct htEntry *curr = table->buckets[i]->head;
        while (curr != NULL)
        {
            if (curr->value)
                LOG_PRINT("%s=\'%s\'\n", curr->key, curr->value);
            curr = curr->next;
        }
    }
}