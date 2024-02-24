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
#include <unistd.h>

struct options_s {
  long long n;
};

typedef struct options_s options;

void read_options(int argc, char *const *argv, options *opts) {
  int c;
  while ((c = getopt(argc, argv, "n:")) != -1) {
    switch (c) {
    case 'n':
      opts->n = atoll(optarg);
      break;
    case '?':
      fprintf(stderr, "Usage: %s [-n] num\n", argv[0]);
      exit(EXIT_FAILURE);
    default:
      abort();
    }
  }
}

#define ROOT_RANK 0

int main(int argc, char **argv) {
  int np, pid;
  long long nums_per_process;
  options opts;
  double result;

  read_options(argc, argv, &opts);
  MPI_Init(&argc, &argv);
  MPI_Comm_size(MPI_COMM_WORLD, &np);
  MPI_Comm_rank(MPI_COMM_WORLD, &pid);

  int last_pid = np - 1;
  nums_per_process = opts.n / np;
  if (pid == last_pid) {
    for (long long j = (nums_per_process * pid) + 1; j <= opts.n; ++j)
      result += 1.0 / (double)(j);
  } else {
    for (long long i = 0, j = (nums_per_process * pid) + 1;
         i < nums_per_process; ++i, ++j)
      result += 1.0 / (double)(j);
  }

  double reduced;
  MPI_Reduce(&result, &reduced, 1, MPI_DOUBLE, MPI_SUM, ROOT_RANK,
             MPI_COMM_WORLD);

  if (pid == ROOT_RANK) {
    printf("Rank: %d; Sum over n = %lld = %lf", pid, opts.n, reduced);
  }

  MPI_Finalize();
  return EXIT_SUCCESS;
}
