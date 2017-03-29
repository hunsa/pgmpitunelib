

#ifndef BUF_MANAGER_H_
#define BUF_MANAGER_H_


enum BufferErrors {
  BUF_NO_ERROR = 0,
  BUF_MALLOC_FAILED,
  BUF_NO_SPACE_LEFT
};

int grab_msg_buffer_1(size_t size, void **buf);

int grab_msg_buffer_2(size_t size, void **buf);

int grab_int_buffer_1(size_t size, int **buf);

int grab_int_buffer_2(size_t size, int **buf);

void release_msg_buffers(void);

void release_int_buffers(void);

int pgmpi_allocate_buffers(const size_t size_msg_buffer, const size_t size_int_buffer);

int pgmpi_free_buffers(void);

#endif /* BUF_MANAGER_H_ */
