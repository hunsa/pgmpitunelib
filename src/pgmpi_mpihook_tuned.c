/*
 * pgmpi_mpihook.c
 *
 *  Created on: Feb 27, 2017
 *      Author: sascha
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <mpi.h>
#include "pgmpi_tune.h"
#include "bufmanager/pgmpi_buf.h"
#include "collectives/collective_modules.h"
#include "config/pgmpi_config.h"
#include "tuning/pgmpi_function_replacer.h"
#include "tuning/pgmpi_profile_reader.h"
#include "pgmpi_algid_store.h"

#define ZF_LOG_LEVEL MY_ZF_LOG_LEVEL
#include "log/zf_log.h"

#include "map/hashtable_int.h"

/****************************************/

static alg_lookup_table_t lookup;

/****************************************/

static void context_init();
static void context_free();
static int context_get_algorithm(pgmpi_collectives_t cid, int msg_size, int comm_size, int *alg_id);

static void fill_lookup_table();
static void free_lookup_table();


pgmpi_context_hook_t context = {
     CONTEXT_TUNED,
     &context_init,
     &context_free,
     &context_get_algorithm
 };


/****************************************/

static void fill_lookup_table() {
  char *prof_path;
  reprompib_dictionary_t *hashmap;

  hashmap = pgmpi_context_get_cli_dict();

  pgmpi_allocate_replacement_table(&lookup);

  prof_path = reprompib_get_value_from_dict(hashmap, "profile_path");

  if( prof_path == NULL ) {
    ZF_LOGV("no profile path found in CLI, trying env");
    prof_path = pgmpi_get_profile_path();
    if( prof_path == NULL ) {
      ZF_LOGV("no profile file found in ENV, continuing without a profile");
    }
  }

  if( prof_path != NULL ) {
    pgmpi_profile_t *profiles;
    int n_profiles = 0;
    int i;

    ZF_LOGV("reading profiles from %s", prof_path);

    pgmpi_profile_read_many(prof_path, &profiles, &n_profiles);

    for(i=0; i<n_profiles; i++) {
      ZF_LOGV("adding profile for cid %u", profiles[i].cid);
      pgmpi_add_profile_to_table(&lookup, &profiles[i]);
    }

    free(prof_path);
  }

}

static void free_lookup_table() {
  pgmpi_free_replacement_table(&lookup);
}


static void context_init() {
  fill_lookup_table();
}


static void context_free() {
  free_lookup_table();
}


static int context_get_algorithm(pgmpi_collectives_t cid, int msg_size, int comm_size,
    int *alg_id) {
  int res;

  *alg_id = 0;

  res = pgmpi_find_replacement_algorithm(&lookup, cid, msg_size, comm_size, alg_id);

  if (res != 0) {
    ZF_LOGV("Cannot set algorithm for collective %u (%d, %d), reset alg to 0", cid, msg_size, comm_size);
    *alg_id = 0;
  }

  ZF_LOGV("found alg id %d", *alg_id);

  return 0;
}



