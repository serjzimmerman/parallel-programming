/*
 * MIT License
 *
 * Copyright (c) 2024 Sergei Zimmerman
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>

#define ROOT_RANK 0
#define TAG 0

void modify_data_and_print_msg(int pid, int *data) {
  printf("Rank: %d, Data: %d\n", pid, *data);
  ++*data;
}

int main(int argc, char **argv) {
  int np, pid;
  int data = 0;

  MPI_Init(&argc, &argv);

  MPI_Comm_size(MPI_COMM_WORLD, &np);
  MPI_Comm_rank(MPI_COMM_WORLD, &pid);

  if (pid == ROOT_RANK) {
    modify_data_and_print_msg(pid, &data);
    MPI_Send(&data, 1, MPI_INT, pid + 1, TAG, MPI_COMM_WORLD);
  } else {
    MPI_Recv(&data, 1, MPI_INT, pid - 1, TAG, MPI_COMM_WORLD,
             MPI_STATUS_IGNORE);
    modify_data_and_print_msg(pid, &data);
    MPI_Send(&data, 1, MPI_INT, (pid + 1) % np, TAG, MPI_COMM_WORLD);
  }

  if (pid == ROOT_RANK) {
    MPI_Recv(&data, 1, MPI_INT, np - 1, TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    modify_data_and_print_msg(pid, &data);
  }

  MPI_Finalize();
  return EXIT_SUCCESS;
}
