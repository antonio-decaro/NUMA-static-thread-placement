#include <omp.h>
#include <stdlib.h>
#include <string.h>

#define WITH_ROI

#ifdef WITH_ROI
#include "/home/hpc1/installs/data_collection/counters/counting.h"
#endif

#define SIZE 4096 * 256

int main(void) {
  int i, n = SIZE / sizeof(double);
  double* buf = malloc(SIZE);

#ifdef WITH_ROI
  counting_set_output("my_output.csv");
  counting_set_info_field("example");
  struct eventset* evset = counting_init(0);
  counting_start(evset);
#endif

#pragma omp parallel for
  for (i = 0; i < n; i++) {
    buf[i] = 0;
  }

  double x = 0;

#pragma omp barrier
#pragma omp parallel for reduction(+ : x)
  for (i = 0; i < n; i++) {
    x += buf[i];
  }

#ifdef WITH_ROI
  counting_stop(evset);
  counting_fini(evset);
#endif
  return 0;
}
