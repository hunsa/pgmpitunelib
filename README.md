
# Introduction

The PGMPITuneLib library is a tool that relies on self-consistent
performance guidelines to automatically tune the performance of MPI
libraries.

Performance guidelines require that specialized MPI collective
functions are not slower than semantically equivalent implementations
using less-specialized functions, which we call mock-up versions. 
For example, MPI_Allgather should provide a better latency than the
(semantically equivalent) call to MPI_Gather followed by an
MPI_Broadcast of the results.

PGMPITuneLib is designed to transparently replace the default
implementation of an MPI collective function with one of its mock-up
implementations, if the corresponding performance guideline is
violated.

More details can be found in: 
- Sascha Hunold and Alexandra Carpen-Amarie. 2018. Autotuning MPI Collectives using Performance Guidelines. In Proceedings of the International Conference on High Performance Computing in Asia-Pacific Region (HPC Asia 2018). Association for Computing Machinery, New York, NY, USA, 64–74. DOI: https://doi.org/10.1145/3149457.3149461


# Quick Start

## Prerequisites
  - an MPI library 
  - CMake (version >= 2.6)  

## Building the Code

The code can be built as follows:

```
cd ${PGMPITUNELIB_PATH}
cmake ./
make
```

# Using PGMPITuneLib 

PGMPITuneLib provides two different libraries:
1. *PGMPITuneCLI* enables the user to select a specific mock-up
   function implementation for each MPI collective, and can be used to
   benchmark the performance of MPI applications
2. *PGMPITuneD* uses performance profiles to automatically tune
   applications by redirecting MPI calls to the mock-up implementation
   that achieved the best performance

# PGMPITuneCLI

The user code has to be linked against the PGMPITuneCLI library and
then the selected mock-up is transparently used instead of the default
implementation.

## Example
- replace calls to MPI_Allgather with a semantically equivalent
  function that uses MPI_Gather followed by an MPI_Bcast to obtain the
  same results

```
mpicc *.c -o mympicode -lpgmpitunecli -lmpi 

mpirun -np 2 ./mympicode --module=allgather=alg:allgather_as_gather_bcast
```

## PGMPITuneD

The user code has to be linked against the PGMPITuneD library.  

To inform the library which MPI collectives should be replaced with
mock-up implementations, the user needs to provide *the path to a
directory containing performance profiles* as a command-line argument
to the application call.

A performance profile records the MPI collective name, the number of
processes for which the tuning was performed, and a list of message
size ranges for which the function should be replaced with a different
algorithm.  An example is provided in
`${PGMPITUNELIB_PATH}/test/perfmodels/models1/p_allgather.prf`.

```
# test profile
#
MPI_Allgather # collective name
4 # profile for p=4 procs
3
1 allgather_as_allreduce 
2 allgather_as_alltoall 
3 allgather_as_gather_bcast
4 # nb of (msg size range + alg id)
16 16 1
32 32 2
64 128 1
1024 2048 3
```

### Example 
- use the provided test profile to tune `MPI_Allgather`
```
  mpicc *.c -o mympicode -lpgmpituned -lmpi 

  mpirun -np 2 ./mympicode --ppath=${PGMPITUNELIB_PATH}/test/perfmodels/models1 
```


## Use a configuration file for memory requirements

Add the `--config` command-line argument to specify the path to a
configuration file.  The configuration file should contain a list of
key-value pairs, one per line, separated by a single space character.
Comment lines (starting with =#=) are also accepted.

The configuration file is useful to modify the default amount of
memory that can be used in the implementation of mock-up functions. If
a mock-up requires more memory than the limit imposed by the
configuration file, the default MPI collective will be used instead.

### Configuration file example

```
# Size limit for the additional data buffers used by mock-up functions
size_msg_buffer_bytes 100000

# Size limit for the additional counts arrays used by mock-up functions
size_int_buffer_bytes 10000
```


## List the mock-up functions implemented for each MPI collective
```
${PGMPITUNELIB_PATH}/bin/pgmpi_info 
```
