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

#define ZF_LOG_LEVEL MY_ZF_LOG_LEVEL
#include "log/zf_log.h"

#include "pgmpi_tune.h"
#include "collective_modules.h"
#include "util/keyvalue_store.h"
#include "util/pgmpi_parse_cli.h"

static module_t *modules;

void pgmpi_modules_init() {

  modules = (module_t *)calloc(NUM_COLLECTIVES, sizeof(module_t));

  register_module_allgather(&modules[0]);
  register_module_allreduce(&modules[1]);
  register_module_alltoall(&modules[2]);
  register_module_bcast(&modules[3]);
  register_module_gather(&modules[4]);
  register_module_reduce(&modules[5]);
  register_module_reduce_scatter_block(&modules[6]);
  register_module_scan(&modules[7]);
  register_module_scatter(&modules[8]);

}

void pgmpi_modules_free() {

  deregister_module_allgather(&modules[0]);
  deregister_module_allreduce(&modules[1]);
  deregister_module_alltoall(&modules[2]);
  deregister_module_bcast(&modules[3]);
  deregister_module_gather(&modules[4]);
  deregister_module_reduce(&modules[5]);
  deregister_module_reduce_scatter_block(&modules[6]);
  deregister_module_scan(&modules[7]);
  deregister_module_scatter(&modules[8]);

  free(modules);

}

int pgmpi_modules_get_number() {
  return NUM_COLLECTIVES;
}

module_t *pgmpi_modules_get(pgmpi_collectives_t cid) {
  int i;
  module_t *mod = NULL;

  assert(modules != NULL);

  for(i=0; i<NUM_COLLECTIVES; i++) {
    //ZF_LOGV("module cid %d", modules[i].cid);
    if( modules[i].cid == cid  ) {
      mod = &modules[i];
      break;
    }
  }
  return mod;
}

int pgmpi_modules_get_id_by_mpiname(const char *mpiname, pgmpi_collectives_t *cid) {
  int i = 0;
  int ret = -1;
  for (i = 0; i < NUM_COLLECTIVES; i++) {
    if (strcmp(modules[i].mpiname, mpiname) == 0) {
      *cid = modules[i].cid;
      ret = 0;
      break;
    }
  }
  return ret;
}

int pgmpi_modules_get_algid_by_algname(const module_alg_choices_t *alg_choices, const char *algname) {
  int algid = -1;
  int i;
  assert(alg_choices != NULL);
  assert(algname != NULL);

  for(i=0; i<alg_choices->nb_choices; i++) {
    if( strcmp(alg_choices->alg[i].algname, algname) == 0 ) {
      algid = alg_choices->alg[i].algid;
    }
  }
  return algid;
}

int pgmpi_check_algid_valid(const module_alg_choices_t *alg_choices, const int algid) {
  int ret = -1;

  if( alg_choices != NULL ) {
    int i;
    for(i=0; i<alg_choices->nb_choices; i++) {
      if( alg_choices->alg[i].algid == algid ) {
        ret = 0;
        break;
      }
    }
    if( ret == -1 ) {
      ZF_LOGW("algid %d is invalid!", algid);
    }
  } else {
    ZF_LOGE("alg_choices is NULL!");
  }

  return ret;
}

char *pgmpi_modules_get_algname_by_algid(const module_alg_choices_t *alg_choices, const int algid) {
  char *algname = NULL;

  int i;
  assert(alg_choices != NULL);

  for(i=0; i<alg_choices->nb_choices; i++) {
    if( alg_choices->alg[i].algid == algid ) {
      algname = strdup(alg_choices->alg[i].algname);
    }
  }

  return algname;
}

