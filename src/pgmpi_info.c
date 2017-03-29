/*
 * pgmpi_info.c
 *
 *  Created on: Mar 8, 2017
 *      Author: sascha
 */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "pgmpi_tune.h"
#include "collectives/collective_modules.h"

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


