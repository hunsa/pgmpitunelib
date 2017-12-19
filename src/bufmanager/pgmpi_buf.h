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
