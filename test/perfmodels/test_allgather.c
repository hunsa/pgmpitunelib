#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>


void test_allgather(int n) {
  int i, correct;
  int *sendbuf, *recvbuf, *recvbuf2;
  int sendcount, recvcount;
  int rank, size;

  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &size);

  sendcount = n;
  recvcount = sendcount;

  sendbuf = (int*) calloc(sendcount, sizeof(int));
  recvbuf = (int*) calloc(sendcount * size, sizeof(int));

  for (i = 0; i < sendcount; i++) {
    sendbuf[i] = rand() % 100;
  }

  printf("%d: Calling MPI_Allgather\n", rank);

  MPI_Allgather(sendbuf, sendcount, MPI_INT, recvbuf, recvcount, MPI_INT, MPI_COMM_WORLD);

  recvbuf2 = (int*) calloc(sendcount * size, sizeof(int));
  PMPI_Allgather(sendbuf, sendcount, MPI_INT, recvbuf2, recvcount, MPI_INT, MPI_COMM_WORLD);

  correct = 1;
  for (i = 0; i < recvcount; i++) {
    if (recvbuf[i] != recvbuf2[i]) {
      printf("buf idx %d: %d != %d\n", i, recvbuf[i], recvbuf2[i]);
      correct = 0;
      break;
    }
  }

  if (correct == 1) {
    printf("%d, correct\n", rank);
  } else {
    printf("%d, incorrect\n", rank);
  }

  free(sendbuf);
  free(recvbuf);
  free(recvbuf2);

}

int main(int argc, char *argv[]) {
  int n;

  MPI_Init(&argc, &argv);

  srand(1);
  n = 4; // 16B
  test_allgather(n);
  n = 8; // 32B
  test_allgather(n);
  n = 256; // 1024B
  test_allgather(n);

  MPI_Finalize();

  return 0;
}
