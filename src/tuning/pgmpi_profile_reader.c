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
#include <dirent.h>

#include "pgmpi_tune.h"

#define ZF_LOG_LEVEL MY_ZF_LOG_LEVEL
#include "log/zf_log.h"

#include "pgmpi_profile_reader.h"


static int can_ignore(char *line) {
  int ret = 0;

  if( line == NULL ) {
    return 0;
  }

  if( strlen(line) < 1 ) {
    return 0;
  }

  if( line[0] == '#' ) {
    ret = 1;
  }

  return ret;
}


static ssize_t get_next_line(FILE *fp, char **line) {
  size_t len;
  ssize_t read;

  do {
    read = getline(&(*line), &len, fp);
    if (read == -1) {
      break;
    } else {
      //ZF_LOGV("line=%s", *line);
      if (can_ignore(*line) == 0) {
        break;
      }
    }
  } while (1);

  if( read == -1 ) {
    ZF_LOGE("unexpected data format");
    exit(1);
  }

  return read;
}

/*
 * we use getline, manpage says "standardized in POSIX.1-2008"
 */

int pgmpi_profile_read(const char *fname, pgmpi_profile_t *profile) {
  int ret = 0;
  FILE *fp;
  int i;
  int nb_procs, n_ranges;
  int alg_nb;

  //char *line = NULL;
  int linelength = 100;
  char buf[40], buf2[60];
  char *line;
  char **alg_names;
  int   *alg_ref_id;

//  ssize_t read;

  if( fname == NULL ) {
    ZF_LOGE("file name is NULL");
    return -1;
  }

  if ((fp = fopen(fname, "r")) == NULL) {
    ZF_LOGE("Can't open %s", fname);
    return -1;
  }
  ZF_LOGV("reading profile %s", fname);

  line = (char*)calloc(linelength, sizeof(char));

  get_next_line(fp, &line);
  sscanf(line, "%s", buf);
  ZF_LOGV("mpiname=%s", buf);

  get_next_line(fp, &line);
  sscanf(line, "%d", &nb_procs);
  ZF_LOGV("nb_procs=%d", nb_procs);

  if( nb_procs <= 0) {
    ZF_LOGE("number of procs must be > 0");
    return -1;
  }

  get_next_line(fp, &line);
  sscanf(line, "%d", &alg_nb);
  ZF_LOGV("alg_nb=%d", alg_nb);

  alg_names  = (char**)calloc(alg_nb, sizeof(char*));
  alg_ref_id = (int*)calloc(alg_nb, sizeof(int));
  for(i=0; i<alg_nb; i++) {
    get_next_line(fp, &line);
    sscanf(line, "%d %s", &alg_ref_id[i], buf2);
    alg_names[i] = strdup(buf2);
    ZF_LOGV("alg name[%d] (%d)=%s (l %zu) (p %p) ", i, alg_ref_id[i], alg_names[i], strlen(alg_names[i]), alg_names[i]);
  }

  get_next_line(fp, &line);
  sscanf(line, "%d", &n_ranges);
  ZF_LOGV("n_ranges=%d", n_ranges);

  if( n_ranges <= 0) {
    ZF_LOGE("number of ranges must be > 0");
    return -1;
  }

  pgmpi_profile_allocate(profile, buf, nb_procs, n_ranges);

  for(i=0; i<n_ranges; i++) {
    int ref_id, msg_size_begin, msg_size_end;
    int ref_id_idx = -1;
    int j;

    get_next_line(fp, &line);
    sscanf(line, "%d %d %d", &msg_size_begin, &msg_size_end, &ref_id);
    ZF_LOGV("range: %d %d %d", msg_size_begin, msg_size_end, ref_id);

    for(j=0; j<alg_nb; j++) {
      if( alg_ref_id[j] == ref_id ) {
        ref_id_idx = j;
        break;
      }
    }

    if( ref_id_idx != -1 ) {
      //ZF_LOGV("set alg (%d, %p), for %d", ref_id_idx, alg_names[ref_id_idx], i);
      pgmpi_profile_set_alg_for_range(profile, i, msg_size_begin, msg_size_end, alg_names[ref_id_idx]);
    } else {
      ZF_LOGW("cannot find ref id for %d", ref_id);
    }
  }

  for(i=0; i<alg_nb; i++) {
    free(alg_names[i]);
  }
  free(alg_names);
  free(alg_ref_id);
  free(line);

  fclose(fp);


  return ret;
}


int pgmpi_profile_read_many(const char *path, pgmpi_profile_t **profiles, int *num_profiles) {
  int ret = 0;
  DIR *dirp;
  struct dirent *entry;
  int nb_files;
  char *suffix;
  int cur_prof_idx;

  suffix = pgmpi_get_profile_file_suffix();
  if( suffix == NULL ) {
    ZF_LOGE("profile suffix invalid. program error!");
    return -1;
  }

  if ((dirp = opendir(path)) == NULL) {
    ZF_LOGE("could not open profile directory");
    return -1;
  }

  nb_files = 0;
  while ((entry = readdir(dirp)) != NULL) {
    //if (entry->d_type == DT_REG) {
      if (strstr(entry->d_name, suffix) != NULL) {
        nb_files++;
      }
    //}
  }

  // rewind and now read files
  rewinddir(dirp);


  *num_profiles = nb_files;
  *profiles = (pgmpi_profile_t *)calloc(*num_profiles, sizeof(pgmpi_profile_t));

  ZF_LOGV("number of profiles found and malloc'd: %d", *num_profiles);

  cur_prof_idx = 0;
  while ((entry = readdir(dirp)) != NULL) {
    //if (entry->d_type == DT_REG) {
      if (strstr(entry->d_name, suffix) != NULL) {
        char fullpath[1024];

        //profiles[cur_prof_idx] = (pgmpi_profile_t *)calloc(1, sizeof(pgmpi_profile_t));

        ZF_LOGV("IDX %d -> %p", cur_prof_idx, &((*profiles)[cur_prof_idx]));

        strcpy(fullpath, path);
        strcat(fullpath, "/");
        strcat(fullpath, entry->d_name);

        pgmpi_profile_read(fullpath, &((*profiles)[cur_prof_idx]));
        cur_prof_idx++;
      }
    //}
  }

  closedir(dirp);

  return ret;
}
