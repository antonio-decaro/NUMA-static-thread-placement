#include <omp.h>
#include <stdio.h>
#include <stdlib.h>

#include "/home/hpc1/installs/data_collection/counters/counting.h"

#define N 256
#define SIZE N * sizeof(double)
#define ROUNDS (1 << 7)

int main(void) {
  counting_set_output("my_output.csv");
  counting_set_info_field("test");
  struct eventset* evset = counting_init(0);
  counting_start(evset);

  double* buf = malloc(SIZE);
  size_t i;
  int j, nthreads = 1;

  for (i = 0; i < N; i++) {
    buf[i] = rand();
  }

#pragma omp parallel
  {
#pragma omp single
    {
      nthreads = omp_get_num_threads();
      fprintf(stdout, "%d threads\n", nthreads);
    }
  }

  counting_start(evset);
#pragma omp parallel for
  for (j = 0; j < ROUNDS * nthreads; j++) {
    buf[rand() % N] = buf[rand() % N];
  }

  counting_stop(evset);
  counting_fini(evset);
  return 1;
}
