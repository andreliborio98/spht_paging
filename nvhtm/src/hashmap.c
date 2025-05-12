#include "../include/hashmap.h"
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

// Source: https://codereview.stackexchange.com/questions/253409/generic-implementation-of-a-hashtable-with-double-linked-list

// Sources used as reference/learning material:
// http://pokristensson.com/code/strmap/strmap.c
// https://github.com/Encrylize/hashmap/blob/master/hashmap.c
// https://github.com/goldsborough/hashtable/blob/master/hashtable.c

hashtable_t *ht_new(unsigned int bucket_count)
{
    struct hashtable_t *ht;

    ht = malloc(sizeof(struct hashtable_t));
    if (ht == NULL) //works as an assert
        return NULL;

    #ifdef USE_SIMPLEHASH
//        printf ("SIMPLEHASH\n");
        ht->buckets = calloc(bucket_count, sizeof(uint64_t));
    #else
//        printf ("REGULAR HASH\n");
        ht->buckets = malloc(sizeof(entry_t) * bucket_count);
    #endif
    if (ht->buckets == NULL){
        free(ht);
        return NULL;
    }
    ht->buckets_count = bucket_count;
    return ht;
}

void ht_delete(struct hashtable_t *ht, uint64_t key)
{
    // printf ("HT DELETE start\n");
    unsigned int index = compute_hash(ht, key);

    #ifdef USE_SIMPLEHASH
        ht->buckets[index]=0;    
    #else
        entry_t *e_curr = ht_get(ht, key);

        /* Chain has no entries */
        if (e_curr == NULL)
            return;

        entry_t *e_prev = e_curr->prev;
        entry_t *e_next = e_curr->next;

        /* Entry is first element in the chain */
        if(e_prev == NULL)
        {
            if(e_next != NULL)
            {
                e_next->prev = NULL;
            }

            ht->buckets[index] = e_next;
            free(e_curr);
        }
        /* Entry is not the first element in the chain */
        else
        {
            e_prev->next = e_next;

            if(e_next != NULL)
                e_next->prev = e_prev;

            free(e_curr);
        }
    #endif
    // printf ("HT DELETE end\n");
}

// int ht_add(struct hashtable_t *ht, uint64_t key, long value) //old
// {
//     unsigned int index = compute_hash(ht, key);
//     struct entry_t *new_entry = malloc(sizeof(struct entry_t));
//     if (new_entry == NULL)
//         return -1;

//     if (ht->buckets[index] != NULL)
//     {
//         /* Go to the end of the linked list and append new entry */
//         entry_t *current_entry = ht->buckets[index];
//         while (current_entry->next != NULL)
//         {
//             current_entry = current_entry->next;
//         }
//         new_entry->key = key;
//         new_entry->value = value;
//         new_entry->next = NULL;
//         new_entry->prev = current_entry;
//         current_entry->next = new_entry;
//     } else {
//         new_entry->key = key;
//         new_entry->value = value;
//         new_entry->next = NULL;
//         new_entry->prev = NULL;

//         ht->buckets[index] = new_entry;
//     }
//     assert(ht != NULL);
//     return 0;
// }

int ht_add(struct hashtable_t *ht, uint64_t key, uint64_t value) //key=address, value=timestamp
{
    unsigned int index = compute_hash(ht, key);
    #ifdef USE_SIMPLEHASH
        ht->buckets[index] = value;
        return 0;
    #else
        struct entry_t *new_entry;
        if (ht->buckets[index] != NULL) //bucket_count starts w/ 1
        {
            /* Go to the end of the linked list and append new entry */
            entry_t *current_entry = ht->buckets[index];
                if (new_entry == NULL)
                    return -1;
                while (current_entry->next != NULL)
                {    
                    if (current_entry->key == key){
                        break;
                    }
                    current_entry = current_entry->next;
                }
                if (current_entry->key == key){
                    new_entry=current_entry;
                }else{           
                    new_entry = malloc(sizeof(struct entry_t));
                    new_entry->next = NULL;
                    new_entry->prev = current_entry;
                    current_entry->next = new_entry;
                }
                new_entry->key = key;
                new_entry->value = value;
        } else {
            new_entry = malloc(sizeof(struct entry_t));
            if (new_entry == NULL)
            return -1;
            new_entry->key = key;
            new_entry->value = value;
            new_entry->next = NULL;
            new_entry->prev = NULL;

            ht->buckets[index] = new_entry;
        }    
        assert(ht != NULL);
        return 0;
    #endif
}

// Taken from https://burtleburtle.net/bob/hash/integer.html
// Should give a fairly decent distribution of entries
unsigned int compute_hash(struct hashtable_t *ht, uint64_t key)
{
    key = (key ^ 61) ^ (key >> 16);
    key = key + (key << 3);
    key = key ^ (key >> 4);
    key = key * 0x27d4eb2d;
    key = key ^ (key >> 15);
    unsigned int r = key % ht->buckets_count;
    return r;
}

void ht_print(struct hashtable_t *ht)
{
    for (int i = 0; i < ht->buckets_count; ++i)
    {
        printf("[%d] ", i);
        #ifndef USE_SIMPLEHASH
            struct entry_t *e_curr = ht->buckets[i];
            int j=0;
            for (;e_curr != NULL; e_curr = e_curr->next)
            {
                j++;
                // printf("%ld - %ld<=>", e_curr->key, e_curr->value);                
            }
            printf ("%d: %d\n", i, j);
        #endif
        // printf(" NULL\n");
    }
}

uint64_t ht_get(struct hashtable_t *ht, uint64_t key) //key=timestamp here
{
  unsigned int index = compute_hash(ht, key);

  #ifdef USE_SIMPLEHASH
    return ht->buckets[index];
  #else
    entry_t *e_curr = ht->buckets[index];
    for (;e_curr != NULL; e_curr = e_curr->next) {
      if (e_curr->key == key) {
        return e_curr->value;
      }
    }
    return 0;
  #endif                
}

// entry_t *ht_get_swap(struct hashtable_t *ht, unsigned int key, uint64_t* e_curr_addr)
// {
//     unsigned int index   = compute_hash(ht, key);
//     entry_t      *e_curr = ht->buckets[index];
    
//     for (;e_curr != NULL; e_curr = e_curr->next) {
//         if (e_curr->key == key) {
//             *e_curr_addr = &e_curr->key;
//             return e_curr;
//         } else {
//             *e_curr_addr = 0;
//         }
//     }

//     return NULL;
// }

void ht_free(struct hashtable_t *ht)
{
    free(ht->buckets);
    free(ht);
}
