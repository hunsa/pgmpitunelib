/*
 * hashtable.h
 *
 *  Created on: Mar 7, 2017
 *      Author: sascha
 */

#ifndef SRC_MAP_HASHTABLE_INT_H_
#define SRC_MAP_HASHTABLE_INT_H_

#define _XOPEN_SOURCE 500 /* Enable certain library functions (strdup) on linux.  See feature_test_macros(7) */

#include <stdlib.h>
#include <stdio.h>
//#include <limits.h>
//#include <string.h>

struct entry_s {
  int key;
  int value;
  struct entry_s *next;
};

typedef struct entry_s entry_t;

struct hashtable_s {
  int size;
  struct entry_s **table;
};

typedef struct hashtable_s hashtable_t;


hashtable_t *ht_create(int size);
void ht_free(hashtable_t *hashtable);
int ht_get(hashtable_t *hashtable, int key, int *value);
void ht_set(hashtable_t *hashtable, int key, int value);
void ht_get_keys(hashtable_t *hashtable, int **keys, int *nkeys);

#endif /* SRC_MAP_HASHTABLE_INT_H_ */
