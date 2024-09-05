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
#include "config/pgmpi_config_reader.h"
#include "util/pgmpi_parse_cli.h"
#include "util/keyvalue_store.h"
#include "pgmpi_algid_store.h"

#define ZF_LOG_LEVEL MY_ZF_LOG_LEVEL
#include "log/zf_log.h"

static pgmpi_dictionary_t hashmap;

extern pgmpi_context_hook_t context;

void check_and_override_lib_env_params(int *argc, char ***argv);

int MPI_Init(int *argc, char ***argv) {
  int ret;
  ZF_LOGV("Intercepting MPI_Init");
  ret = PMPI_Init(argc, argv);
  check_and_override_lib_env_params(argc, argv);
  init_pgtune_lib(argc, argv);
  return ret;
}


int MPI_Finalize( void ) {
  int ret;
  ZF_LOGV("Intercepting MPI_Finalize");
  finalize_pgtune_lib();
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

static int compute_argc(char *str) {
  int i;
  int cnt = 0;
  int white = 0;
  int seenword = 0;

  for (i = 0; i < strlen(str); i++) {
    if (str[i] == ' ') {
      white = 1;
    } else {
      if( i == strlen(str) -1 &&  white == 0 ) {
        cnt++;
      } else if (white == 1) {
        if( seenword == 1 ) {
          cnt++;
        }
      }
      white = 0;
      seenword = 1;
    }
  }
  return cnt;
}

void check_and_override_lib_env_params(int *argc, char ***argv) {
  char *env = getenv("PGMPI_PARAMS");
  char **argvnew;

  if( env != NULL ) {
    char *token;
    //printf("env:%s\n", env);
    *argc = compute_argc(env) + 1;  // + 1 is for argv[0], which we'll copy
    //printf("argc: %d\n", *argc);

//    printf("(*argv)[0]=%s\n", (*argv)[0]);

    //  TODO: we should probably free the old argv
    argvnew = (char**)malloc(*argc * sizeof(char**));
    // copy old argv[0]
    argvnew[0] = (char*)"dummy";

//    printf("argvnew[0]=%s\n", argvnew[0]);

    token = strtok(env, " ");
    if( token != NULL ) {
//      printf("token: %s\n", token);
      argvnew[1] = token;
//      printf("argvnew[1]=%s\n", argvnew[1]);
      for(int i=2; i<*argc; i++) {
        token = strtok(NULL, " ");
        if( token != NULL ) {
//          printf("token: %s\n", token);
          argvnew[i] = token;
        }
      }
    }

    *argv = argvnew;
  }

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

  context.context_init();
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

  context.context_free();
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

void pgtune_override_argv_parameter(int argc, char **argv) {
  pgmpitune_cleanup_dictionary(&hashmap);
  pgmpitune_init_dictionary(&hashmap);
  parse_cli_arguments(&hashmap, &argc, &argv);
  // need to reinit context to pass args to modules
  context.context_init();
}

