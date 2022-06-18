#include <unistd.h>

#include <cmath>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <ratio>

using namespace std;

void initData(double* data, int id, int len) {
  double factor = (id % 2 ? 0.1 : 0.2);
#pragma omp for schedule(static)
  for (int j = 0; j < len; ++j) {
    data[j] = factor * (j + 1.1) / (j + 1.12345);
  }
}

int main() {
  int len = 64 * 64 * 1000 * 100000;
  double* loopData[5];
  for (int i = 0; i < 5; i++) {
    loopData[i] = new double[len];
  }

  double loopScalar[4];
  initData(loopScalar, 0, 4);
  const double cls = loopScalar[0];
  const double p_cut = loopScalar[1];
  const double pmin = loopScalar[2];
  const double eosvmax = loopScalar[3];

#pragma omp parallel firstprivate(cls, p_cut, pmin, eosvmax)
  {
    initData(loopData[0], 0, len);
    initData(loopData[1], 1, len);
    initData(loopData[2], 2, len);
    initData(loopData[3], 3, len);
    initData(loopData[4], 4, len);

    double* compression = loopData[0];
    double* bvc = loopData[1];
    double* p_new = loopData[2];
    double* e_old = loopData[3];
    double* vnewc = loopData[4];

    // double loopScalar[4];
    // initData(loopScalar, 0, 4);
    // const double cls = loopScalar[0];
    // const double p_cut = loopScalar[1];
    // const double pmin = loopScalar[2];
    // const double eosvmax = loopScalar[3];

    const int num_samples = 10;

    for (int isamp = 0; isamp < num_samples; ++isamp) {
#pragma omp for schedule(static)
      for (int i = 0; i < len; i++) {
        bvc[i] = cls * (compression[i] + 1.0);
      }
#pragma omp for nowait schedule(static)
      for (int i = 0; i < len; i++) {
        p_new[i] = bvc[i] * e_old[i];

        if (fabs(p_new[i]) < p_cut) p_new[i] = 0.0;

        if (vnewc[i] >= eosvmax) p_new[i] = 0.0;

        if (p_new[i] < pmin) p_new[i] = pmin;
      }
    }
  }
}
