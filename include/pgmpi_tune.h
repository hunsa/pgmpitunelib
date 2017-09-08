/*
 * pgmpi_tune.h
 *
 *  Created on: Feb 27, 2017
 *      Author: sascha
 */

#ifndef INCLUDE_PGMPI_TUNE_H_
#define INCLUDE_PGMPI_TUNE_H_

#include <mpi.h>
//#include "log/zf_log.h"
#include "util/keyvalue_store.h"

typedef enum {
  CID_MPI_ALLGATHER = 0,
  CID_MPI_ALLREDUCE,
  CID_MPI_ALLTOALL,
  CID_MPI_BCAST,
  CID_MPI_GATHER,
  CID_MPI_REDUCE,
  CID_MPI_REDUCESCATTERBLOCK,
  CID_MPI_SCAN,
  CID_MPI_SCATTER,
  CID_MARKER_END_DO_NOT_USE_OR_CHANGE
} pgmpi_collectives_t;

extern const int NUM_COLLECTIVES;

typedef enum {
  CONTEXT_CLI,
  CONTEXT_TUNED,
  CONTEXT_INFO
} pgmpi_context_t;

typedef struct {
  int algid;
  char *algname;
} alg_choice_t;

typedef struct {
  int nb_choices;
  alg_choice_t *alg;
} module_alg_choices_t;

typedef struct {
  char *cli_prefix;
  char *mpiname;
  int is_rooted;
  pgmpi_collectives_t cid;
  void (*parse)(const char *argv);
  void (*set_algid)(const int algid);
  module_alg_choices_t *alg_choices;
} module_t;

int pgtune_get_algorithm(pgmpi_collectives_t cid, int msg_size, int comm_size, int *alg_id);

void init_pgtune_lib(int *argc, char ***argv);

void finalize_pgtune_lib();

int get_pgmpi_context();

pgmpi_dictionary_t *pgmpi_context_get_cli_dict();

#ifdef USE_PMPI
#define PGMPI_COMM_SIZE PMPI_Comm_size
#define PGMPI( call )     P##call
#else
#define PGMPI_COMM_SIZE MPI_Comm_size
#define PGMPI( call )     call
#endif

#ifdef USE_LOGGING
#define MY_ZF_LOG_LEVEL ZF_LOG_VERBOSE
#else
#define MY_ZF_LOG_LEVEL ZF_LOG_WARN
#endif

#ifdef USE_ALGID_STORING
#define ALGID_STORING 1
#else
#define ALGID_STORING 0
#endif

extern const int PGMPI_ENABLE_ALGID_STORING;


typedef struct {
  pgmpi_context_t context_id;
  void (*context_init)(void);
  void (*context_free)(void);
  int (*context_get_algorithm)(pgmpi_collectives_t cid, int msg_size, int comm_size, int *alg_id);
} pgmpi_context_hook_t;


#endif /* INCLUDE_PGMPI_TUNE_H_ */
