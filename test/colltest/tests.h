
#ifndef TESTS_H_
#define TESTS_H_


typedef struct basic_collparams {
    int root;
    MPI_Datatype datatype;
    MPI_Op op;
    MPI_Comm comm;
} basic_collective_params_t;

typedef void (*collective_test_t)(basic_collective_params_t* params, int count, void **resbuf_ref, void **resbuf_test, size_t *resbuf_size);


typedef enum check_type {
  CHECK_RES_ROOT_ONLY = 0,
  CHECK_RES_ALL_PROCS = 1
} check_type_t;

typedef struct test_info {
  collective_test_t func;
  char* name;
  check_type_t check_res_all_procs;
} test_info_t;


void test_Allgather(basic_collective_params_t* params, int count, void **resbuf_ref, void **resbuf_test, size_t *resbuf_size);
void test_Allreduce(basic_collective_params_t* params, int count, void **resbuf_ref, void **resbuf_test, size_t *resbuf_size);
void test_Alltoall(basic_collective_params_t* params, int count, void **resbuf_ref, void **resbuf_test, size_t *resbuf_size);
void test_Bcast(basic_collective_params_t* params, int count, void **resbuf_ref, void **resbuf_test, size_t *resbuf_size);
void test_Reduce(basic_collective_params_t* params, int count, void **resbuf_ref, void **resbuf_test, size_t *resbuf_size);
void test_Gather(basic_collective_params_t* params, int count, void **resbuf_ref, void **resbuf_test, size_t *resbuf_size);
void test_Reduce_scatter_block(basic_collective_params_t* params, int count, void **resbuf_ref, void **resbuf_test, size_t *resbuf_size);
void test_Scan(basic_collective_params_t* params, int count, void **resbuf_ref, void **resbuf_test, size_t *resbuf_size);
void test_Scatter(basic_collective_params_t* params, int count, void **resbuf_ref, void **resbuf_test, size_t *resbuf_size);

#endif /* TESTS_H_ */



