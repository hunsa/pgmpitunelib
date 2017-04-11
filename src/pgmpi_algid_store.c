/*
 * pgmpi_algid_store.c
 *
 *  Created on: Mar 8, 2017
 *      Author: sascha
 */

#include <stdio.h>
#include <stdlib.h>

#include "pgmpi_tune.h"

#define ZF_LOG_LEVEL MY_ZF_LOG_LEVEL
#include "log/zf_log.h"

#include "map/hashtable_int.h"
#include "pgmpi_algid_store.h"
#include "collectives/collective_modules.h"

static hashtable_t **maps;
const int INITIAL_MAP_SIZE = 100;

static int my_rank = -1;
//static int pgmpi_get_algid_for_msg_size(const pgmpi_collectives_t cid, const int msg_size, int *alg_id);

void pgmpi_init_algid_maps() {

  MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);

  if( my_rank != 0 ) {
    return;
  }

  // possibly one map per collective
  // map lazily initialized
  maps = (hashtable_t**)calloc(NUM_COLLECTIVES, sizeof(hashtable_t*));
}

void pgmpi_free_algid_maps() {
  int i;

  if( my_rank != 0 ) {
    return;
  }

  for(i=0; i<NUM_COLLECTIVES; i++) {
    if( maps[i] != NULL ) {
      free(maps[i]);
    }
  }

  free(maps);
}

void pgmpi_save_algid_for_msg_size(const pgmpi_collectives_t cid, const int msg_size, const int alg_id,
    const int called_default) {

  int save_alg_id = alg_id;

  if( my_rank != 0 ) {
    return;
  }

  if(called_default == 1) {
    save_alg_id = 0;
  }

  if( maps[cid] == NULL ) {
    maps[cid] = ht_create(INITIAL_MAP_SIZE);
  }

  ht_set(maps[cid], msg_size, save_alg_id);
}

void pgmpi_print_algids(FILE *fp) {
  int i;

  if( my_rank != 0 ) {
    return;
  }

  for(i=0; i<NUM_COLLECTIVES; i++) {
    if( maps[i] != NULL ) {
      int *keys, nkeys, j;
      module_t *mod;

      ht_get_keys(maps[i], &keys, &nkeys);
      mod = pgmpi_modules_get(i);
      if( mod == NULL ) {
        ZF_LOGW("module is NULL, program error!");
        continue;
      }
      for(j=0; j<nkeys; j++) {
        int val, found;
        found = ht_get(maps[i], keys[j], &val);
        if( found == 1 ) {
          char *algname = pgmpi_modules_get_algname_by_algid(mod->alg_choices, val);
          if( algname != NULL ) {
            fprintf(fp, "#@pgmpi alg %s %d %s\n", mod->mpiname, keys[j], algname);
            free(algname);
          } else {
            ZF_LOGE("unexpected error, algname NULL");
          }
        } else {
          ZF_LOGW("cannot find value for key %d", keys[j]);
        }
      }
      free(keys);
    }
  }
}

int pgmpi_convert_type_count_2_bytes(const int count, const MPI_Datatype type) {
  MPI_Aint lb, extent;
  MPI_Type_get_extent(type, &lb, &extent);
  return count * extent;
}


