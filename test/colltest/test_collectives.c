/*  ReproMPI Benchmark
 *
 *  Copyright 2015 Alexandra Carpen-Amarie, Sascha Hunold
 Research Group for Parallel Computing
 Faculty of Informatics
 Vienna University of Technology, Austria

 <license>
 This program is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 2 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program.  If not, see <http://www.gnu.org/licenses/>.
 </license>
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <assert.h>
#include "mpi.h"

#include "pgmpi_tune.h"
#include "bufmanager/pgmpi_buf.h"
#include "src/collectives/collective_modules.h"
#include "tests.h"

#define ZF_LOG_LEVEL MY_ZF_LOG_LEVEL
#include "log/zf_log.h"

static const int root_rank = 0;

typedef enum test_res_codes {
  TEST_PASSED = 0,
  TEST_FAILED = 1
} test_res_codes_t;

static char* test_res_str[] = {
    [TEST_PASSED] = "Passed",
    [TEST_FAILED] = "FAILED",
};


static const test_info_t test_info_list[] = {
    { &test_Allgather, "MPI_Allgather", CHECK_RES_ALL_PROCS},
    { &test_Allreduce, "MPI_Allreduce", CHECK_RES_ALL_PROCS},
    { &test_Alltoall, "MPI_Alltoall", CHECK_RES_ALL_PROCS},
    { &test_Bcast, "MPI_Bcast", CHECK_RES_ALL_PROCS},
    { &test_Gather, "MPI_Gather", CHECK_RES_ROOT_ONLY},
    { &test_Reduce, "MPI_Reduce", CHECK_RES_ROOT_ONLY},
    { &test_Reduce_scatter_block, "MPI_Reduce_scatter_block",CHECK_RES_ALL_PROCS},
    { &test_Scan, "MPI_Scan", CHECK_RES_ALL_PROCS},
    { &test_Scatter, "MPI_Scatter", CHECK_RES_ALL_PROCS}
};
static const int N_TESTS = 9;

collective_test_t get_test_function(char* module_mpiname) {
  int i;

  if (module_mpiname == NULL) {
    return NULL;
  }

  for (i=0; i<N_TESTS; i++) {
    if (strcmp(module_mpiname, test_info_list[i].name) == 0) {
      return test_info_list[i].func;
    }
  }
  return NULL;
}

check_type_t get_test_check_type(char* module_mpiname) {
  int i;

  if (module_mpiname == NULL) {
    ZF_LOGE("ERROR: Module name is null\n");
    exit(1);
  }

  for (i=0; i<N_TESTS; i++) {
    if (strcmp(module_mpiname, test_info_list[i].name) == 0) {
      return (test_info_list[i].check_res_all_procs);
    }
  }
  return CHECK_RES_ALL_PROCS;
}


test_res_codes_t identical(void* buffer1, void* buffer2, size_t size) {
  size_t i;
  test_res_codes_t res = TEST_PASSED;

  for (i = 0; i < size; i++) {
    if (((char*) buffer1)[i] != ((char*) buffer2)[i]) {
      res = TEST_FAILED;
      break;
    }
  }
  return res;
}

test_res_codes_t check_results(module_t *module, basic_collective_params_t* params, void *resbuf_ref, void *resbuf_test,
    size_t resbuf_size) {

  int rank, size;
  test_res_codes_t local_test_result = TEST_FAILED;
  test_res_codes_t test_result;
  int *test_res_all_procs;
  int i;

  MPI_Comm_rank(params->comm, &rank);
  MPI_Comm_size(params->comm, &size);

  test_res_all_procs = (int*) calloc(size, sizeof(int));

  if (get_test_check_type(module->mpiname) == CHECK_RES_ROOT_ONLY) {
    if (rank == params->root) { // only check receive buffers on the test's root process
      local_test_result = identical(resbuf_ref, resbuf_test, resbuf_size);
    } else {
      local_test_result = TEST_PASSED;
    }
  } else {  // each process check its local receive buffers
    local_test_result = identical(resbuf_ref, resbuf_test, resbuf_size);
  }

  // make sure all processes return the same test result
  PMPI_Allgather(&local_test_result, 1, MPI_INT, test_res_all_procs, 1, MPI_INT, params->comm);

  test_result = TEST_PASSED;
  for (i = 0; i < size; i++) {
    if (test_res_all_procs[i] == TEST_FAILED) {
      test_result = TEST_FAILED;
      break;
    }
  }

  free(test_res_all_procs);
  return test_result;
}

void print_test_result(module_t *module, int alg_id, test_res_codes_t test_res, MPI_Comm comm) {
  int rank;
  MPI_Comm_rank(comm, &rank);

  if (rank == root_rank) {  // only the root has the test results
    printf("Module %-25s %-50s... %-10s\n", module->mpiname, (module->alg_choices)->alg[alg_id].algname,
        test_res_str[test_res]);
  }
}


void do_test(module_t *module, basic_collective_params_t *params, int count) {
  void *resbuf_ref, *resbuf_test;
  size_t resbuf_size;
  test_res_codes_t test_result;
  int alg_id;
  collective_test_t module_test;
  int rank;

  MPI_Comm_rank(MPI_COMM_WORLD, &rank);


  if (module == NULL) {
    if (rank == root_rank) {
      ZF_LOGE("Module is null\n");
    }
    exit(1);
  }
  module_test = get_test_function(module->mpiname);

  if (module_test == NULL) {
    if (rank == root_rank) {
      printf("Module %-25s %-50s... no tests implemented\n", module->mpiname, "");
    }
    return;
  }

  for (alg_id = 0; alg_id < (module->alg_choices)->nb_choices; alg_id++) {
    // set algorithm
    module->set_algid(alg_id);

    // run test
    module_test(params, count, &resbuf_ref, &resbuf_test, &resbuf_size);
    test_result = check_results(module, params, resbuf_ref, resbuf_test, resbuf_size);

    print_test_result(module, alg_id, test_result, params->comm);
    // cleanup result buffers
    free(resbuf_ref);
    free(resbuf_test);
  }
  // set algorithm id back to default to avoid infinite loops
  //module->set_algid(0);
}


int main(int argc, char* argv[]) {

  basic_collective_params_t basic_coll_info;
  int count = 0;
  char *module_name = NULL;
  int rank, i;
  module_t *module;

  srand(1000);

  MPI_Init(&argc, &argv);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);

  if (argc < 3) {
    if (rank == root_rank) {
      printf("\nUSAGE: %s mpi_collective count\n", argv[0]);
    }
    exit(1);
  }

  module_name = strdup(argv[1]);
  count = atol(argv[2]);

  basic_coll_info.comm = MPI_COMM_WORLD;
  basic_coll_info.datatype = MPI_DOUBLE;
  basic_coll_info.op = MPI_MIN;
  basic_coll_info.root = 0;

  if (strcmp(module_name, "all") == 0) {
    for (i = 0; i < pgmpi_modules_get_number(); i++) {
      module = pgmpi_modules_get(i);
      do_test(module, &basic_coll_info, count);
    }
  } else {
    int err;
    pgmpi_collectives_t cid;

    err = pgmpi_modules_get_id_by_mpiname(module_name, &cid);
    if (err) {
      if (rank == root_rank) {
        ZF_LOGE("Invalid module name: %s\n", module_name);
      }
      exit(1);
    }

    module = pgmpi_modules_get(cid);
    do_test(module, &basic_coll_info, count);
  }

  free(module_name);

  /* shut down MPI */
  MPI_Finalize();

  return 0;
}
