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

#include "pgmpi_tune.h"

#define ZF_LOG_LEVEL MY_ZF_LOG_LEVEL
#include "log/zf_log.h"

#include "pgmpi_config.h"
#include "util/keyvalue_store.h"

static pgmpi_config_t config;

void pgmpi_config_init() {
  config.config_dict = (pgmpi_dictionary_t *)calloc(1, sizeof(pgmpi_dictionary_t));
  pgmpitune_init_dictionary(config.config_dict);

  pgmpitune_add_element_to_dict(config.config_dict, "size_msg_buffer_bytes", "100000000");
  pgmpitune_add_element_to_dict(config.config_dict, "size_int_buffer_bytes", "200000");
}

void pgmpi_config_free() {
  pgmpitune_cleanup_dictionary(config.config_dict);
  free(config.config_dict);
}

void pgmpi_config_add_key_value(const char *key, const char *val) {
  assert( key != NULL );
  assert( val != NULL );
  pgmpitune_add_element_to_dict(config.config_dict, key, val);
}


void pgmpi_config_get_keys(char ***keys, int *nkeys) {
  pgmpitune_get_keys_from_dict(config.config_dict, keys, nkeys);
}

int pgmpi_config_get_long_value(const char *key, unsigned long *val) {
  int ret = -1;

  assert(key != NULL);
  if (pgmpitune_dict_has_key(config.config_dict, key)) {
    char *valstr = NULL;
    valstr = pgmpitune_get_value_from_dict(config.config_dict, key);
    if( valstr != NULL ) {
      ZF_LOGV("valstr=%s", valstr);
      *val = atol(valstr);
      ret = 0;
      free(valstr);
    }
  }

  return ret;
}

int pgmpi_config_get_string_value(const char *key, char **val) {
  int ret = -1;

  assert(key != NULL);

  if (pgmpitune_dict_has_key(config.config_dict, key)) {
    *val = pgmpitune_get_value_from_dict(config.config_dict, key);
    ret = 0;
  }

  return ret;
}

void pgmpi_config_print(FILE *fp) {
  char **keys;
  int nkeys, i;
  int rank;

  MPI_Comm_rank(MPI_COMM_WORLD, &rank);

  if( rank != 0 ) {
    return;
  }

  pgmpitune_get_keys_from_dict(config.config_dict, &keys, &nkeys);

  for(i=0; i<nkeys; i++) {
    char *val;
    val = pgmpitune_get_value_from_dict(config.config_dict, keys[i]);
    if( val != NULL ) {
      fprintf(fp, "#@pgmpi config %s %s\n", keys[i], val);
    } else {
      ZF_LOGW("error: no value for key %s", keys[i]);
    }
    free(keys[i]);
  }
  free(keys);
}

char *pgmpi_config_get_filename_from_env() {
  char *fname;

  fname = getenv ("PGMPI_CONFIG_FILE");
  if( fname != NULL ) {
    fname = strdup(fname);
  }
  return fname;
}


