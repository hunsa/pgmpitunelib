/*
 * pgmpi_config_reader.c
 *
 *  Created on: Mar 9, 2017
 *      Author: sascha
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>

#include "pgmpi_tune.h"

#define ZF_LOG_LEVEL MY_ZF_LOG_LEVEL
#include "log/zf_log.h"

#include "pgmpi_config_reader.h"


static int can_ignore(char *line) {
  int ret = 0;

  if( line == NULL ) {
    return 0;
  }

  if( strlen(line) < 1 ) {
    return 0;
  }

  if( strlen(line) == 0 ) {
    ret = 1;
  }

  if( strlen(line) == 1 && line[0] == '\n' ) {
    ret = 1;
  }

  if( line[0] == '#' ) {
    ret = 1;
  }

//  ZF_LOGV("line length %zu", strlen(line));

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

  return read;
}

/*
 * we use getline, manpage says "standardized in POSIX.1-2008"
 */

int pgmpi_config_read(const char *fname) {
  int ret = 0;
  FILE *fp;
  int read;

  char *line = NULL;
  char key[80], val[80];


  if( fname == NULL ) {
    ZF_LOGE("file name is NULL");
    return -1;
  }

  if ((fp = fopen(fname, "r")) == NULL) {
    ZF_LOGE("Can't open %s", fname);
    return -1;
  }

  ZF_LOGV("reading profile %s", fname);

  do {
    int nb_of_elemns_read;
    read = get_next_line(fp, &line);
    if( read == -1 ) {
      break;
    }

    nb_of_elemns_read = sscanf(line, "%s %s\n", key, val);
    if(nb_of_elemns_read == 2) {
      ZF_LOGV("config key=%s value=%s", key, val);
      pgmpi_config_add_key_value(key, val);
    } else {
      ZF_LOGV("faulty line in config file: %s", line);
    }

  } while(1);


  fclose(fp);

  return ret;
}

