
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mpi.h"

#include "tests.h"

#define ZF_LOG_LEVEL MY_ZF_LOG_LEVEL
#include "log/zf_log.h"


void set_buffer_random(void *buff, int count, MPI_Datatype datatype) {
  int i;
  MPI_Aint type_extent, lb;
  MPI_Type_get_extent(datatype, &lb, &type_extent);

  for (i = 0; i < count * type_extent; i++) {
    ((char*)buff)[i] = rand() % 100;
  }
}



void test_Allgather(basic_collective_params_t* params, int count, void **resbuf_ref, void **resbuf_test, size_t *resbuf_size) {
  void *sendbuf, *recvbuf1, *recvbuf2;
  MPI_Aint type_extent, lb;
  int rank, size;

  MPI_Comm_rank(params->comm, &rank);
  MPI_Comm_size(params->comm, &size);
  MPI_Type_get_extent(params->datatype, &lb, &type_extent);

  // count is the number of elements on each process
  sendbuf = (char*) calloc(count * type_extent, sizeof(char));

  recvbuf1 = (char*) calloc(size * count * type_extent, sizeof(char));
  recvbuf2 = (char*) calloc(size * count * type_extent, sizeof(char));

  set_buffer_random(sendbuf, count, params->datatype);

  PMPI_Allgather(sendbuf, count, params->datatype, recvbuf1, count, params->datatype, params->comm);
  MPI_Allgather(sendbuf, count, params->datatype, recvbuf2, count, params->datatype, params->comm);


  free(sendbuf);

  *resbuf_ref = recvbuf1;
  *resbuf_test = recvbuf2;
  *resbuf_size = size * count * type_extent;
}



void test_Allreduce(basic_collective_params_t* params, int count, void **resbuf_ref, void **resbuf_test, size_t *resbuf_size) {
  void *sendbuf, *recvbuf1, *recvbuf2;
  MPI_Aint type_extent, lb;
  int rank;

  MPI_Comm_rank(params->comm, &rank);
  MPI_Type_get_extent(params->datatype, &lb, &type_extent);

  // count is the number of elements on each process
  sendbuf = (char*) calloc(count * type_extent, sizeof(char));

  recvbuf1 = (char*) calloc(count * type_extent, sizeof(char));
  recvbuf2 = (char*) calloc(count * type_extent, sizeof(char));

  set_buffer_random(sendbuf, count, params->datatype);

  PMPI_Allreduce(sendbuf, recvbuf1, count, params->datatype, params->op, params->comm);
  MPI_Allreduce(sendbuf, recvbuf2, count, params->datatype, params->op, params->comm);

  free(sendbuf);

  *resbuf_ref = recvbuf1;
  *resbuf_test = recvbuf2;
  *resbuf_size = count * type_extent;
}

void test_Alltoall(basic_collective_params_t* params, int count, void **resbuf_ref, void **resbuf_test, size_t *resbuf_size) {
  void *sendbuf, *recvbuf1, *recvbuf2;
  MPI_Aint type_extent, lb;
  int size;

  MPI_Comm_size(params->comm, &size);
  MPI_Type_get_extent(params->datatype, &lb, &type_extent);

  // count is the number of elements on each process
  sendbuf = (char*) calloc(size * count * type_extent, sizeof(char));

  recvbuf1 = (char*) calloc(size * count * type_extent, sizeof(char));
  recvbuf2 = (char*) calloc(size * count * type_extent, sizeof(char));

  set_buffer_random(sendbuf, size * count, params->datatype);

  PMPI_Alltoall(sendbuf, count, params->datatype, recvbuf1, count, params->datatype, params->comm);
  MPI_Alltoall(sendbuf, count, params->datatype, recvbuf2, count, params->datatype, params->comm);

  free(sendbuf);

  *resbuf_ref = recvbuf1;
  *resbuf_test = recvbuf2;
  *resbuf_size = size * count * type_extent;
}


void test_Bcast(basic_collective_params_t* params, int count, void **resbuf_ref, void **resbuf_test, size_t *resbuf_size) {
  int rank, size;
  MPI_Aint type_extent, lb;
  void *buff1, * buff2;

  MPI_Comm_rank(params->comm, &rank);
  MPI_Comm_size(params->comm, &size);
  MPI_Type_get_extent(params->datatype, &lb, &type_extent);

  buff1 = (char*) calloc(count * type_extent, sizeof(char));
  buff2 = (char*) calloc(count * type_extent, sizeof(char));

  set_buffer_random(buff1, count, params->datatype);

  // identical buffers at root
  if (rank == params->root) {
    memcpy(buff2, buff1, count * type_extent);
  }

  PMPI_Bcast(buff1, count, params->datatype, params->root, params->comm);
  MPI_Bcast(buff2, count, params->datatype, params->root, params->comm);

  *resbuf_ref = buff1;
  *resbuf_test = buff2;
  *resbuf_size = count * type_extent;
}


void test_Reduce(basic_collective_params_t* params, int count, void **resbuf_ref, void **resbuf_test, size_t *resbuf_size) {
  void *sendbuf, *recvbuf1, *recvbuf2;
  MPI_Aint type_extent, lb;
  int rank;

  MPI_Comm_rank(params->comm, &rank);
  MPI_Type_get_extent(params->datatype, &lb, &type_extent);

  // count is the number of elements on each process
  sendbuf = (char*) calloc(count * type_extent, sizeof(char));

  recvbuf1 = NULL;
  recvbuf2 = NULL;
  if (rank == params->root) {  // recv buffers only needed at the root
    recvbuf1 = (char*) calloc(count * type_extent, sizeof(char));
    recvbuf2 = (char*) calloc(count * type_extent, sizeof(char));
  }
  set_buffer_random(sendbuf, count, params->datatype);

  PMPI_Reduce(sendbuf, recvbuf1, count, params->datatype, params->op, params->root, params->comm);
  MPI_Reduce(sendbuf, recvbuf2, count, params->datatype, params->op, params->root, params->comm);

  free(sendbuf);

  *resbuf_ref = recvbuf1;
  *resbuf_test = recvbuf2;
  if (rank == params->root) { // recv buffers only needed at the root
      *resbuf_size = count * type_extent;
  } else {
      *resbuf_size = 0;
  }
}


void test_Gather(basic_collective_params_t* params, int count, void **resbuf_ref, void **resbuf_test, size_t *resbuf_size) {
  void *sendbuf, *recvbuf1, *recvbuf2;
  MPI_Aint type_extent, lb;
  int rank, size;

  MPI_Comm_rank(params->comm, &rank);
  MPI_Comm_size(params->comm, &size);
  MPI_Type_get_extent(params->datatype, &lb, &type_extent);

  // count is the number of elements on each process
  sendbuf = (char*) calloc(count * type_extent, sizeof(char));

  recvbuf1 = NULL;
  recvbuf2 = NULL;
  if (rank == params->root) { // recv buffers only needed at the root
    recvbuf1 = (char*) calloc(size * count * type_extent, sizeof(char));
    recvbuf2 = (char*) calloc(size * count * type_extent, sizeof(char));
  }

  set_buffer_random(sendbuf, count, params->datatype);

  PMPI_Gather(sendbuf, count, params->datatype, recvbuf1, count, params->datatype, params->root, params->comm);
  MPI_Gather(sendbuf, count, params->datatype, recvbuf2, count, params->datatype, params->root, params->comm);


  free(sendbuf);

  *resbuf_ref = recvbuf1;
  *resbuf_test = recvbuf2;
  if (rank == params->root) { // recv buffers only needed at the root
      *resbuf_size = size * count * type_extent;
  } else {
      *resbuf_size = 0;
  }
}


void test_Reduce_scatter_block(basic_collective_params_t* params, int count, void **resbuf_ref, void **resbuf_test, size_t *resbuf_size) {
  void *sendbuf, *recvbuf1, *recvbuf2;
  MPI_Aint type_extent, lb;
  int size;

  MPI_Comm_size(params->comm, &size);
  MPI_Type_get_extent(params->datatype, &lb, &type_extent);

  // count is the number of elements on each process
  sendbuf = (char*) calloc(size * count * type_extent, sizeof(char));

  recvbuf1 = (char*) calloc(count * type_extent, sizeof(char));
  recvbuf2 = (char*) calloc(count * type_extent, sizeof(char));

  set_buffer_random(sendbuf, size * count, params->datatype);

  PMPI_Reduce_scatter_block(sendbuf, recvbuf1, count, params->datatype, params->op, params->comm);
  MPI_Reduce_scatter_block(sendbuf, recvbuf2, count, params->datatype, params->op, params->comm);

  free(sendbuf);

  *resbuf_ref = recvbuf1;
  *resbuf_test = recvbuf2;
  *resbuf_size = count * type_extent;
}



void test_Scan(basic_collective_params_t* params, int count, void **resbuf_ref, void **resbuf_test, size_t *resbuf_size) {
  void *sendbuf, *recvbuf1, *recvbuf2;
  MPI_Aint type_extent, lb;

  MPI_Type_get_extent(params->datatype, &lb, &type_extent);

  // count is the number of elements on each process
  sendbuf = (char*) calloc(count * type_extent, sizeof(char));

  recvbuf1 = (char*) calloc(count * type_extent, sizeof(char));
  recvbuf2 = (char*) calloc(count * type_extent, sizeof(char));

  set_buffer_random(sendbuf, count, params->datatype);

  PMPI_Scan(sendbuf, recvbuf1, count, params->datatype, params->op, params->comm);
  MPI_Scan(sendbuf, recvbuf2, count, params->datatype, params->op, params->comm);

  free(sendbuf);

  *resbuf_ref = recvbuf1;
  *resbuf_test = recvbuf2;
  *resbuf_size = count * type_extent;
}



void test_Scatter(basic_collective_params_t* params, int count, void **resbuf_ref, void **resbuf_test, size_t *resbuf_size) {
  void *sendbuf, *recvbuf1, *recvbuf2;
  MPI_Aint type_extent, lb;
  int rank, size;

  MPI_Comm_rank(params->comm, &rank);
  MPI_Comm_size(params->comm, &size);
  MPI_Type_get_extent(params->datatype, &lb, &type_extent);

  // count is the number of elements on each process
  sendbuf = NULL;
  if (rank == params->root) { // sendbuf only required at the root
    sendbuf = (char*) calloc(size * count * type_extent, sizeof(char));
    set_buffer_random(sendbuf, size * count, params->datatype);
  }

  recvbuf1 = (char*) calloc(count * type_extent, sizeof(char));
  recvbuf2 = (char*) calloc(count * type_extent, sizeof(char));

  PMPI_Scatter(sendbuf, count, params->datatype, recvbuf1, count, params->datatype, params->root, params->comm);
  MPI_Scatter(sendbuf, count, params->datatype, recvbuf2, count, params->datatype, params->root, params->comm);

  if (rank == params->root) {
    free(sendbuf);
  }

  *resbuf_ref = recvbuf1;
  *resbuf_test = recvbuf2;
  *resbuf_size = count * type_extent;
}



