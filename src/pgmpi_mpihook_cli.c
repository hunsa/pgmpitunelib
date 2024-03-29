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
#include "collectives/collective_modules.h"
#include "config/pgmpi_config.h"
#include "util/pgmpi_parse_cli.h"
#include "util/keyvalue_store.h"
#include "pgmpi_algid_store.h"
#include "pgmpi_mpihook_private.h"

#define ZF_LOG_LEVEL MY_ZF_LOG_LEVEL
#include "log/zf_log.h"


static void context_init();
static void context_free();
static int context_get_algorithm(pgmpi_collectives_t cid, int msg_size, int comm_size, int *alg_id);

pgmpi_context_hook_t context = {
     CONTEXT_CLI,
     &context_init,
     &context_free,
     &context_get_algorithm
 };

static void pass_cli_arguments_to_modules();

static void pass_cli_arguments_to_modules() {
  int nkeys, i, j;
  char **keys;
  pgmpi_dictionary_t *hashmap;

  hashmap = pgmpi_context_get_cli_dict();
  pgmpitune_get_keys_from_dict(hashmap, &keys, &nkeys);
  for (i = 0; i < nkeys; i++) {
    for (j = 0; j < pgmpi_modules_get_number(); j++) {
      int slength = strlen(keys[i]);
      int slength2;
      module_t *mod;

      mod = pgmpi_modules_get(j);
      slength2 = strlen(mod->cli_prefix);
      if (slength == slength2 && strncmp(mod->cli_prefix, keys[i], slength) == 0) {
        char *val = pgmpitune_get_value_from_dict(hashmap, keys[i]);
        if( val != NULL ) {
          mod->parse(val);
          free(val);
        }
      }
    }
  }
}


static void context_init() {
  pass_cli_arguments_to_modules();
}


static void context_free() {
}


static int context_get_algorithm(pgmpi_collectives_t cid, int msg_size, int comm_size,
    int *alg_id) {
  assert(1==1);
  return -1;
}

