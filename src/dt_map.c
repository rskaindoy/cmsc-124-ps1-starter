/*
 * dt_map.c: Associative arrays for Unit 5, Section E.
 *
 * An array does not store its indices. This map stores its keys.
 * An array calculates a position with one subtraction.
 * The map calculates a hash and then compares keys in one bucket.
 *
 * Hashing turns the key into a bucket number. Compare all keys in that bucket
 * because two keys can select it. Use a linked list for each bucket. Start the
 * unsigned accumulator at 14695981039346656037ULL. For each unsigned byte,
 * exclusive-or the byte into it and multiply by 1099511628211ULL.
 *
 * A separate list stores insertion order for stable output. dt_map_key_at
 * reads this list. Updating a key preserves its position. Removing and
 * reinserting a key moves it to the end.
 */

#include "dt.h"

#include <stdlib.h>
#include <string.h>

#define BUCKET_COUNT 16

typedef struct dt_map_entry {
    char            *key;
    dt_value        value;
    struct dt_map_entry    *next;
} dt_map_entry;

struct dt_map {
    /* TODO: Add the buckets and insertion-order data. */
    dt_map_entry    *buckets[BUCKET_COUNT];
    char            **order;
    size_t          length;
    size_t          order_capacity;
};

/*
 * dt_map_new builds an empty map. It returns NULL after an allocation failure.
 */
dt_map *dt_map_new(void)
{
    /* TODO: Return an allocated empty map. Return NULL after an allocation failure.
       dt_map_new()  -> a map whose dt_map_len is 0
       cases/normal/map_basics.case */
    dt_map *m = malloc(sizeof(dt_map));

    if (m == NULL) {
        return NULL;
    }

    for (size_t i = 0; i < BUCKET_COUNT; i++) {
        m->buckets[i] = NULL;
    }

    m->order = NULL;
    m->length = 0;
    m->order_capacity = 0;

    return m;
}

/*
 * dt_map_free releases each entry, copied key, order array, and map.
 * It accepts NULL. The environment owns the values.
 */
void dt_map_free(dt_map *m)
{
    /* TODO: Release each entry, copied key, order array, and map.
       Preserve the values. The environment owns them.
       a map holding a string value  -> the nodes and keys go, the string stays
       dt_map_free(NULL)             -> returns, having done nothing
       cases/cleanup/map_churn.case */
    if (m == NULL) {
        return;
    }

    // free entries in every bucket
    for (size_t i = 0; i < BUCKET_COUNT; i++) {
        dt_map_entry *entry = m->buckets[i];

        while (entry != NULL) {
            dt_map_entry *next = entry->next;

            free(entry->key);
            free(entry);

            entry = next;
        }
    }

    free(m->order);
    free(m);
}

/*
 * dt_map_len returns the number of keys in constant time.
 */
size_t dt_map_len(const dt_map *m)
{
    /* TODO: Return the current key count.
       Replacing a value does not change this count.
       after put alpha, beta, gamma:  dt_map_len(m) -> 3
       after put beta again:          dt_map_len(m) -> 3, still
       after del alpha:               dt_map_len(m) -> 2
       cases/normal/map_basics.case */
    return m->length;
}

/*
 * dt_map_put binds v to key.
 * An existing key keeps its insertion position. A new key becomes the last key.
 * Copy each new key because the caller owns the source buffer.
 * Return DT_ERR_CAPACITY after an allocation failure.
 */

/*
 * hashes a key using the required 64-bit FNV-1a algorithm and returns its bucket index.
 */  
static size_t hash_key(const char *key)
{
    unsigned long long h = 14695981039346656037ULL;

    for (const unsigned char *p = (const unsigned char *)key; *p != '\0'; p++) {
        h ^= (unsigned long long)*p;
        h *= 1099511628211ULL;
    }

    return h % BUCKET_COUNT;
}

dt_status dt_map_put(dt_map *m, const char *key, dt_value v)
{
    /* TODO: Replace the value for an existing key.
       Add a new entry for a new key. Copy each new key.
       Hash the key. Select its bucket. Search the bucket chain.
       Add a new entry to the chain and insertion list.
       put "beta" -> 2 on an empty map    -> DT_OK, "beta" is last in order
       put "beta" -> 22 on that map       -> DT_OK, same position, new value
       an allocation failure              -> DT_ERR_CAPACITY, map unchanged
       cases/normal/map_basics.case */
    
    // replace value if key already exists
    size_t bucket = hash_key(key);
    dt_map_entry *entry = m->buckets[bucket];

    while (entry != NULL) {
        if (strcmp(entry->key, key) == 0) {
            entry->value = v;
            return DT_OK;
        }

        entry = entry->next;
    }

    // add a new entry for a new key
    size_t key_length = strlen(key);

    char *key_copy = malloc(key_length + 1);
    if (key_copy == NULL) {
        return DT_ERR_CAPACITY;
    }

    // copy the key
    memcpy(key_copy, key, key_length + 1);

    // allocate a new map entry
    dt_map_entry *new_entry = malloc(sizeof(dt_map_entry));
    if (new_entry == NULL) {
        free(key_copy);
        return DT_ERR_CAPACITY;
    }

    new_entry->key = key_copy;
    new_entry->value = v;
    new_entry->next = NULL;
    
    // grow th einsertion-order array if it is full
    if (m->length == m->order_capacity) {
        size_t new_capacity = (m->order_capacity == 0)? 4 : m->order_capacity * 2;

        char **new_order = realloc(m->order, new_capacity * sizeof(char *));
    

        if (new_order == NULL) {
            free(new_entry);
            free(key_copy);
            return DT_ERR_CAPACITY;
        }

        m->order = new_order;
        m->order_capacity = new_capacity;
    }

    // add entry to the front of the bucket's linked list
    new_entry->next = m->buckets[bucket];
    m->buckets[bucket] = new_entry;

    // add key to the inswertion-order array
    m->order[m->length] = new_entry->key;
    m->length++;

    return DT_OK;
}

/*
 * dt_map_get writes the value for key to *out.
 * It returns DT_ERR_KEY and does not change *out when the key is absent.
 * An absent key differs from a nil value.
 */
dt_status dt_map_get(const dt_map *m, const char *key, dt_value *out)
{
    /* TODO: Return DT_ERR_KEY when the key is absent.
       Preserve *out after this error. A nil value can be present.
       after put "beta" -> 22:
         dt_map_get(m, "beta", &out)   -> DT_OK, *out is the integer 22
         dt_map_get(m, "ghost", &out)  -> DT_ERR_KEY, *out untouched
       cases/normal/map_basics.case, cases/boundary/map_missing_key.case */
    
    // find the bucket for the key
    size_t bucket = hash_key(key);
    dt_map_entry *entry = m->buckets[bucket];

    // search the bucket for the key
    while (entry != NULL) {
        if (strcmp(entry->key, key) == 0) {
            *out = entry->value;
            return DT_OK;
        }

        entry = entry->next;
    }

    // key not found
    return DT_ERR_KEY;
}

/*
 * dt_map_remove removes key from its bucket and insertion position.
 * It releases the copied key. It returns DT_ERR_KEY when the key is absent.
 */
dt_status dt_map_remove(dt_map *m, const char *key)
{
    /* TODO: Remove the entry from its bucket and insertion position.
       Release the copied key. Return DT_ERR_KEY when the key is absent.
       a map holding alpha, beta, gamma:
         dt_map_remove(m, "alpha")  -> DT_OK, order is now beta, gamma
         dt_map_remove(m, "ghost")  -> DT_ERR_KEY, nothing changes
       reinserting "alpha" appends it after "gamma"
       cases/normal/map_basics.case, cases/boundary/map_remove_missing_key.case */
    
    // find the bucket containing the key
    size_t bucket = hash_key(key);
    dt_map_entry *entry = m->buckets[bucket];
    dt_map_entry *prev = NULL;

    while (entry != NULL) {
        if (strcmp(entry->key, key) == 0) {
            break;
        }

        prev = entry;
        entry = entry->next;
    }

    // key not found
    if (entry == NULL) {
        return DT_ERR_KEY;
    }

    // remove the entry from the bucket's linked list
    if (prev == NULL) {
        m->buckets[bucket] = entry->next;
    } else {
        prev->next = entry->next;
    }

    // find the entry's position in the insertion-order array
    size_t index = 0;
    while (index < m->length && m->order[index] != entry->key) {
        index++;
    }

    // shift later keys left to fill the removed position
    for (size_t i = index; i + 1 < m->length; i++) {
        m->order[i] = m->order[i+1];
    }

    m->length--;

    // free copied key and entry
    free(entry->key);
    free(entry);

    return DT_OK;
}

/*
 * dt_map_key_at writes the key at insertion position index to *out.
 * It returns DT_ERR_RANGE and does not change *out for an invalid index.
 */
dt_status dt_map_key_at(const dt_map *m, size_t index, const char **out)
{
    /* TODO: Write the key at the specified insertion position to *out.
       Return DT_ERR_RANGE for an invalid position. Preserve *out after this error.
       The printer uses this order.
       a map holding alpha, beta, gamma:
         dt_map_key_at(m, 0, &out)  -> DT_OK, *out = "alpha"
         dt_map_key_at(m, 3, &out)  -> DT_ERR_RANGE, *out untouched
       cases/normal/map_basics.case */
    
    // check if index is outside the map's insertion order
    if (index >= m->length) {
        return DT_ERR_RANGE;
    }
    
    // write key at specified position to *out
    *out = m->order[index];

    return DT_OK;
}
