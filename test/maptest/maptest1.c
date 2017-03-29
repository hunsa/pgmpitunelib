/*
 * maptest1.c
 *
 *  Created on: Mar 8, 2017
 *      Author: sascha
 */

#include <stdio.h>
#include <assert.h>

#include "map/hashtable_int.h"

int main(int argc, char *argv[]) {

  hashtable_t *map;
  int val;

  map = ht_create(2048);

  ht_set(map, 1, 2);
  ht_get(map, 1, &val);
  assert( val == 2 );
  ht_get(map, 1, &val);
  assert( val == 2 );
  ht_get(map, 1, &val);
  assert( val == 2 );

  ht_set(map, 1, 3);
  ht_get(map, 1, &val);
  assert( val == 3 );

  ht_set(map, 3, 4);
  ht_get(map, 1, &val);
  assert( val == 3 );

  ht_set(map, 4, 4);
  ht_set(map, 5, 4);
  ht_set(map, 4, 5);

  {
    int *keys, nkeys;
    int i;
    ht_get_keys(map, &keys, &nkeys);
    for(i=0; i<nkeys; i++) {
      printf("key %d\n", keys[i]);
    }

  }

  printf("done\n");


  return 0;
}
