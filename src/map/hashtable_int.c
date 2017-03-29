
#include <assert.h>

#include "pgmpi_tune.h"
#include "hashtable_int.h"

#define ZF_LOG_LEVEL MY_ZF_LOG_LEVEL
#include "log/zf_log.h"

static entry_t *ht_newpair(int key, int value);
static int ht_hash(hashtable_t *hashtable, int key);

/* Create a new hashtable. */
hashtable_t *ht_create(int size) {

  hashtable_t *hashtable = NULL;
  int i;

  if (size < 1) {
    return NULL;
  }

  /* Allocate the table itself. */
  if ((hashtable = malloc(sizeof(hashtable_t))) == NULL) {
    return NULL;
  }

  /* Allocate pointers to the head nodes. */
  if ((hashtable->table = calloc(size, sizeof(entry_t *))) == NULL) {
    return NULL;
  }
  for (i = 0; i < size; i++) {
    hashtable->table[i] = NULL;
  }

  hashtable->size = size;

  return hashtable;
}

void ht_free(hashtable_t *hashtable) {

  ZF_LOGW("unimplemented!");

  free(hashtable);
}


static int ht_hash(hashtable_t *hashtable, int key) {
  int x = key;

  assert(hashtable->size > 0);

  x = ((x >> 16) ^ x) * 0x45d9f3b;
  x = ((x >> 16) ^ x) * 0x45d9f3b;
  x = (x >> 16) ^ x;
  x %= hashtable->size;
  return x;
}

static entry_t *ht_newpair(int key, int value) {
  entry_t *newpair;

  if ((newpair = malloc(sizeof(entry_t))) == NULL) {
    return NULL;
  }

  newpair->key = key;
  newpair->value = value;
  newpair->next = NULL;

  return newpair;
}

void ht_set(hashtable_t *hashtable, int key, int value) {
  int bin = 0;
  entry_t *newpair = NULL;
  entry_t *next    = NULL;
  entry_t *last    = NULL;

  bin = ht_hash(hashtable, key);

  next = hashtable->table[bin];

  while (next != NULL && key != next->key) {
    last = next;
    next = next->next;
  }

  /* There's already a pair.  Let's replace that string. */
  if (next != NULL && key == next->key) {
    next->value = value;

    /* Nope, could't find it.  Time to grow a pair. */
  } else {
    newpair = ht_newpair(key, value);

    /* We're at the start of the linked list in this bin. */
    if (next == hashtable->table[bin]) {
      newpair->next = next;
      hashtable->table[bin] = newpair;

      /* We're at the end of the linked list in this bin. */
    } else if (next == NULL) {
      last->next = newpair;

      /* We're in the middle of the list. */
    } else {
      newpair->next = next;
      last->next = newpair;
    }
  }
}

int ht_get(hashtable_t *hashtable, int key, int *value) {
  int bin = 0;
  entry_t *pair;
  int ret = -1;

  bin = ht_hash(hashtable, key);

  /* Step through the bin, looking for our value. */
  pair = hashtable->table[bin];
  while (pair != NULL && key != pair->key) {
    pair = pair->next;
  }

  /* Did we actually find anything? */
  if (pair == NULL || key != pair->key) {
    *value = 0;
  } else {
    *value = pair->value;
    ret = 1;
  }
  return ret;
}

void ht_get_keys(hashtable_t *hashtable, int **keys, int *nkeys) {
  int i, key_idx;

  *nkeys = 0;

  for(i=0; i<hashtable->size; i++) {
    if( hashtable->table[i] != NULL ) {
      entry_t *cur;
      cur = hashtable->table[i];
      while (cur != NULL ) {
        (*nkeys)++;
        cur = cur->next;
      }
    }
  }

  *keys = (int*)calloc(*nkeys, sizeof(int));

  key_idx = 0;
  for(i=0; i<hashtable->size; i++) {
    if( hashtable->table[i] != NULL ) {
      entry_t *cur;
      cur = hashtable->table[i];
      while (cur != NULL ) {
        (*keys)[key_idx] = cur->key;
        key_idx++;
        cur = cur->next;
      }
    }
  }

}
