#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <math.h>

#include "pgmpi_tune.h"
#include "pgmpi_buf.h"

#define ZF_LOG_LEVEL MY_ZF_LOG_LEVEL
#include "log/zf_log.h"

static char *msg_buf;
static int *int_buf;

static size_t offset_msg_buf2;
static size_t offset_int_buf2;

static size_t total_msg_buf_size = 0;
static size_t total_int_buf_size = 0;

static int msg_buf1_in_use;
static int msg_buf2_in_use;
static int int_buf1_in_use;
static int int_buf2_in_use;

#ifdef OPTION_ENABLE_MEMALIGNED_BUFFERS
static size_t buf_alignment = 0;
static const size_t BUFFER_ALIGNMENT_DEFAULT = 64;

void set_alloc_alignment_buf(size_t align) {
  int is_power_of_two;
  ZF_LOGV("initialize alignment with size %zu", align);

  assert(align > 0);
  assert(align % sizeof(void*) == 0);   // multiple of sizeof(void*)
  is_power_of_two = !(align & (align-1));
  assert(is_power_of_two);

  buf_alignment = align;
}
#endif


void* pgmpi_malloc(size_t count, size_t elem_size) {
  void *buf = NULL;

#ifdef OPTION_ENABLE_MEMALIGNED_BUFFERS
  set_alloc_alignment_buf(BUFFER_ALIGNMENT_DEFAULT);
#ifdef OPTION_BUFFER_ALIGNMENT
  set_alloc_alignment_buf(OPTION_BUFFER_ALIGNMENT);
#endif
  posix_memalign(&buf, buf_alignment, count*elem_size);

#else
  buf = calloc(count, elem_size);
#endif
  return buf;
}

int pgmpi_allocate_buffers(const size_t size_msg_buffer, const size_t size_int_buffer) {
  int ret = BUF_NO_ERROR;

  total_msg_buf_size = size_msg_buffer;
  total_int_buf_size = size_int_buffer;

  ZF_LOGV("initialize buffer with sizes %zu / %zu", size_msg_buffer, size_int_buffer);

  msg_buf = (char*) pgmpi_malloc(total_msg_buf_size, sizeof(char));
  int_buf = (int*) pgmpi_malloc(total_int_buf_size / sizeof(int), sizeof(int));

  offset_msg_buf2 = 0;
  offset_int_buf2 = 0;

  if (msg_buf == NULL || int_buf == NULL) {
    ret = BUF_MALLOC_FAILED;
  }

  msg_buf1_in_use = 0;
  msg_buf2_in_use = 0;
  int_buf1_in_use = 0;
  int_buf2_in_use = 0;

  return ret;
}

int pgmpi_free_buffers(void) {
  free(msg_buf);
  free(int_buf);
  return 0;
}

void set_max_size_msg_buf(size_t size) {
  assert(size > 0);
  total_msg_buf_size = size;
}

void set_max_size_int_buf(size_t size) {
  assert(size > 0);
  total_int_buf_size = size;
}

int grab_msg_buffer_1(size_t size, void **buf) {
  int ret = BUF_NO_ERROR;

  if (!msg_buf1_in_use && size <= total_msg_buf_size) {
    *buf = msg_buf;
    ZF_LOGV("buf points to %p", *buf);
    offset_msg_buf2 = size;
    msg_buf1_in_use = 1;
  } else {
    ret = BUF_NO_SPACE_LEFT;
  }

  return ret;
}

int grab_msg_buffer_2(size_t size, void **buf) {
  int ret = BUF_NO_ERROR;

  if (!msg_buf2_in_use && offset_msg_buf2 + size <= total_msg_buf_size) {
    *buf = &(msg_buf[offset_msg_buf2]);
    msg_buf2_in_use = 1;
  } else {
    ret = BUF_NO_SPACE_LEFT;
  }

  return ret;
}

int grab_int_buffer_1(size_t size, int **buf) {
  int ret = BUF_NO_ERROR;

  if (!int_buf1_in_use && size <= total_int_buf_size) {
    *buf = int_buf;
    ZF_LOGV("int buf points to %p", *buf);
    offset_int_buf2 = ceil(size / sizeof(int));
    int_buf1_in_use = 1;
  } else {
    ret = BUF_NO_SPACE_LEFT;
  }
  return ret;
}

int grab_int_buffer_2(size_t size, int **buf) {
  int ret = BUF_NO_ERROR;

  if (!int_buf2_in_use && offset_int_buf2 * sizeof(int) + size <= total_int_buf_size) {
    *buf = &(int_buf[offset_int_buf2]);
    int_buf2_in_use = 1;
  } else {
    ret = BUF_NO_SPACE_LEFT;
  }

  return ret;
}

void release_msg_buffers(void) {
  msg_buf1_in_use = 0;
  msg_buf2_in_use = 0;
}

void release_int_buffers(void) {
  int_buf1_in_use = 0;
  int_buf2_in_use = 0;
}
