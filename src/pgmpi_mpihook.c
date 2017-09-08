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
#include "config/pgmpi_config_reader.h"
#include "util/pgmpi_parse_cli.h"
#include "util/keyvalue_store.h"
#include "pgmpi_algid_store.h"

#define ZF_LOG_LEVEL MY_ZF_LOG_LEVEL
#include "log/zf_log.h"

static pgmpi_dictionary_t hashmap;

extern pgmpi_context_hook_t context;

int MPI_Init(int *argc, char ***argv) {
  int ret;
  ZF_LOGV("Intercepting MPI_Init");
  ret = PMPI_Init(argc, argv);
  init_pgtune_lib(argc, argv);
  context.context_init();
  return ret;
}


int MPI_Finalize( void ) {
  int ret;
  ZF_LOGV("Intercepting MPI_Finalize");
  finalize_pgtune_lib();
  context.context_free();
  ret = PMPI_Finalize();
  return ret;
}

void fill_and_read_pgmpi_config() {
  char *conf_fname = NULL;

  pgmpi_config_init();

  conf_fname = pgmpitune_get_value_from_dict(&hashmap, "config_file");

  if( conf_fname == NULL ) {
    ZF_LOGV("no config file found in CLI, trying env");
    conf_fname = pgmpi_config_get_filename_from_env();
    if( conf_fname == NULL ) {
      ZF_LOGV("no config file found in ENV, continuing without changing the default config");
    }
  }

  if( conf_fname != NULL ) {
    pgmpi_config_read(conf_fname);
    free(conf_fname);
  }

}

void free_pgmpi_config() {

  pgmpi_config_free();

}





void init_pgtune_lib(int *argc, char ***argv) {

  pgmpitune_init_dictionary(&hashmap);

  pgmpi_modules_init();

  parse_cli_arguments(&hashmap, argc, argv);

  if( PGMPI_ENABLE_ALGID_STORING ) {
    pgmpi_init_algid_maps();
  }

  fill_and_read_pgmpi_config();

  {
    size_t size_msg_buffer = 0, size_int_buffer = 0;
    if( pgmpi_config_get_long_value("size_msg_buffer_bytes", &size_msg_buffer) == -1 ) {
      ZF_LOGE("cannot find size_msg_buffer_bytes in config, setting to 0");
      size_msg_buffer = 0;
    }
    if( pgmpi_config_get_long_value("size_int_buffer_bytes", &size_int_buffer) == -1 ) {
      ZF_LOGE("cannot find size_int_buffer_bytes in config, setting to 0");
      size_int_buffer = 0;
    }

    pgmpi_allocate_buffers(size_msg_buffer, size_int_buffer);
  }
}


void finalize_pgtune_lib() {

  pgmpi_free_buffers();

  if( PGMPI_ENABLE_ALGID_STORING ) {
    pgmpi_print_algids(stdout);
    pgmpi_free_algid_maps();

    pgmpi_config_print(stdout);
  }

  free_pgmpi_config();

  pgmpi_modules_free();

  pgmpitune_cleanup_dictionary(&hashmap);

}


pgmpi_dictionary_t *pgmpi_context_get_cli_dict() {
  return &hashmap;
}

int get_pgmpi_context() {
  return context.context_id;
}


int pgtune_get_algorithm(pgmpi_collectives_t cid, int msg_size, int comm_size,
    int *alg_id) {
  return context.context_get_algorithm(cid, msg_size, comm_size, alg_id);
}



