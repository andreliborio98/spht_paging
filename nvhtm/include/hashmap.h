#ifndef HASHMAP_H
#define HASHMAP_H

#include <stdint.h>

    typedef struct entry_t
    {
        uint64_t key;
        uint64_t value;
        struct entry_t* next;
        struct entry_t* prev;
    } entry_t;

    typedef struct hashtable_t
    {
        unsigned int buckets_count;
#ifdef SIMPLEHASH
        uint64_t *buckets; // points directly to an array of timestamps
#else
        struct entry_t **buckets;
#endif
    } hashtable_t;
#endif

// ToDo: Add resizing functionality

unsigned int compute_hash(struct hashtable_t *ht, uint64_t key);
hashtable_t *ht_new(unsigned int size);
int ht_add(struct hashtable_t* ht, uint64_t key, uint64_t value);
uint64_t ht_get(struct hashtable_t* ht, uint64_t key);
void ht_print(struct hashtable_t* ht);
void ht_delete(struct hashtable_t* ht, uint64_t key);
void ht_free(struct hashtable_t* ht);

/*
getTime1();

ht_ts = ht_get(ht, addr);
while (ht_ts > replayerTS){
    sleep(1);
}

getTime2();
ht_add_ts+=getTime2 - getTime1;


*/
