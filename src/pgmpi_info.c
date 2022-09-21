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
#include <assert.h>

#include "pgmpi_tune.h"
#include "include/collective_modules.h"

static void print_module_info(FILE *fp, module_t *mod);

static void print_module_info(FILE *fp, module_t *mod) {
  int i;
  if( mod == NULL ) {
    return;
  }

  assert(mod->alg_choices != NULL);
  for (i = 0; i < (mod->alg_choices)->nb_choices; i++) {
    fprintf(fp, "%s;%s;%s;%d\n", mod->cli_prefix, mod->mpiname, (mod->alg_choices)->alg[i].algname,mod->is_rooted);
  }

}

int main(int argc, char *argv[]) {
  int i;
  module_t *mod;

  MPI_Init(&argc, &argv);


  fprintf(stdout, "cliname;mpiname;algname;rooted\n");
  for(i=0; i<pgmpi_modules_get_number(); i++) {
    mod = pgmpi_modules_get(i);
    print_module_info(stdout, mod);
  }

  MPI_Finalize();

  return 0;
}


