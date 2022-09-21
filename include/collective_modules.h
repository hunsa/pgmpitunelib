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


#ifndef SRC_COLLECTIVES_COLLECTIVE_MODULES_H_
#define SRC_COLLECTIVES_COLLECTIVE_MODULES_H_

void register_module_allgather(module_t *module);
void register_module_allreduce(module_t *module);
void register_module_alltoall(module_t *module);
void register_module_bcast(module_t *module);
void register_module_gather(module_t *module);
void register_module_reduce(module_t *module);
void register_module_reduce_scatter_block(module_t *module);
void register_module_scan(module_t *module);
void register_module_scatter(module_t *module);

void deregister_module_allgather(module_t *module);
void deregister_module_allreduce(module_t *module);
void deregister_module_alltoall(module_t *module);
void deregister_module_bcast(module_t *module);
void deregister_module_gather(module_t *module);
void deregister_module_reduce(module_t *module);
void deregister_module_reduce_scatter_block(module_t *module);
void deregister_module_scan(module_t *module);
void deregister_module_scatter(module_t *module);

void pgmpi_modules_init();
void pgmpi_modules_free();
int pgmpi_modules_get_number();
module_t *pgmpi_modules_get(pgmpi_collectives_t cid);

/*!
  \param mpiname MPI function name
  \param cid in which module id will be written
  \return 0 is found, <>0 if not found
*/
int pgmpi_modules_get_id_by_mpiname(const char *mpiname, pgmpi_collectives_t *cid);

int pgmpi_check_algid_valid(const module_alg_choices_t *alg_choices, const int algid);

int pgmpi_modules_get_algid_by_algname(const module_alg_choices_t *alg_choices, const char *algname);

char *pgmpi_modules_get_algname_by_algid(const module_alg_choices_t *alg_choices, const int algid);

#endif /* SRC_COLLECTIVES_COLLECTIVE_MODULES_H_ */
