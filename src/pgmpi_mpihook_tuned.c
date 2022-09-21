/*  PGMPITuneLib - Library for Autotuning MPI Collectives using Performance Guidelines
 *  
 *  Copyright 2017 Sascha Hunold, Alexandra Carpen-Amarie
 *      Research Group for Parallel Computing
 *      Faculty of Informatics
 *      Vienna University of Technology, Austria
 *  
 *  <license>
 *      This library is free software; you can redistribute it
 *      and/or modify it under the terms of the GNU Lesser General Public
 *      License as published by the Free Software Foundation; either
 *      version 2.1 of the License, or (at your option) any later version.
 *  
 *      This library is distributed in the hope that it will be useful,
 *      but WITHOUT ANY WARRANTY; without even the implied warranty of
 *      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *      Lesser General Public License for more details.
 *  
 *      You should have received a copy of the GNU Lesser General Public
 *      License along with this library; if not, write to the Free
 *      Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *      Boston, MA 02110-1301 USA
 *  </license>
 */



#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <mpi.h>
#include "pgmpi_tune.h"
#include "bufmanager/pgmpi_buf.h"
#include "include/collective_modules.h"
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
  pgmpi_dictionary_t *hashmap;

  hashmap = pgmpi_context_get_cli_dict();

  pgmpi_allocate_replacement_table(&lookup);

  prof_path = pgmpitune_get_value_from_dict(hashmap, "profile_path");

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



