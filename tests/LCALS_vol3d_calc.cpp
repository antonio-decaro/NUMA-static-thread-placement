#include <unistd.h>

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <string>

extern "C" {
#include "../lib/counters/counting.h"
}

typedef double* Real_ptr;

struct ADomain {
  ADomain(int ilen, int ndims) : ndims(ndims), NPNL(2), NPNR(1) {
    int rzmax;
    switch (ilen) {
      case 0: {
        if (ndims == 2) {
          rzmax = 156;
        } else if (ndims == 3) {
          rzmax = 28;
        }
        break;
      }
      case 1: {
        if (ndims == 2) {
          rzmax = 64;
        } else if (ndims == 3) {
          rzmax = 16;
        }
        break;
      }
      case 2: {
        if (ndims == 2) {
          rzmax = 8;
        } else if (ndims == 3) {
          rzmax = 4;
        }
        break;
      }

      default: {
      }
    }

    imin = NPNL;
    jmin = NPNL;
    imax = rzmax + NPNR;
    jmax = rzmax + NPNR;
    jp = imax - imin + 1 + NPNL + NPNR;

    if (ndims == 2) {
      kmin = 0;
      kmax = 0;
      kp = 0;
      nnalls = jp * (jmax - jmin + 1 + NPNL + NPNR);
    } else if (ndims == 3) {
      kmin = NPNL;
      kmax = rzmax + NPNR;
      kp = jp * (jmax - jmin + 1 + NPNL + NPNR);
      nnalls = kp * (kmax - kmin + 1 + NPNL + NPNR);
    }

    fpn = 0;
    lpn = nnalls - 1;
    frn = fpn + NPNL * (kp + jp) + NPNL;
    lrn = lpn - NPNR * (kp + jp) - NPNR;

    fpz = frn - jp - kp - 1;
    lpz = lrn;

    real_zones = new int[nnalls];
    for (int i = 0; i < nnalls; ++i) real_zones[i] = -1;

    n_real_zones = 0;

    if (ndims == 2) {
      for (int j = jmin; j < jmax; j++) {
        for (int i = imin; i < imax; i++) {
          int ip = i + j * jp;

          int id = n_real_zones;
          real_zones[id] = ip;
          n_real_zones++;
        }
      }

    } else if (ndims == 3) {
      for (int k = kmin; k < kmax; k++) {
        for (int j = jmin; j < jmax; j++) {
          for (int i = imin; i < imax; i++) {
            int ip = i + j * jp + kp * k;

            int id = n_real_zones;
            real_zones[id] = ip;
            n_real_zones++;
          }
        }
      }
    }
  }

  ~ADomain() {
    if (real_zones) delete[] real_zones;
  }

  int ndims;
  int NPNL;
  int NPNR;

  int imin;
  int jmin;
  int kmin;
  int imax;
  int jmax;
  int kmax;

  int jp;
  int kp;
  int nnalls;

  int fpn;
  int lpn;
  int frn;
  int lrn;

  int fpz;
  int lpz;

  int* real_zones;
  int n_real_zones;
};

#define NDPTRSET(v, v0, v1, v2, v3, v4, v5, v6, v7) \
  v0 = v;                                           \
  v1 = v0 + 1;                                      \
  v2 = v0 + domain.jp;                              \
  v3 = v1 + domain.jp;                              \
  v4 = v0 + domain.kp;                              \
  v5 = v1 + domain.kp;                              \
  v6 = v2 + domain.kp;                              \
  v7 = v3 + domain.kp;

void initData(double* data, int id, int len) {
  double factor = (id % 2 ? 0.1 : 0.2);
#pragma omp for schedule(static)
  for (int j = 0; j < len; ++j) {
    data[j] = factor * (j + 1.1) / (j + 1.12345);
  }
}

int main(int argc, char* argv[]) {
  int len = 64 * 64 * 1000;
  double* loopData[4];
  for (int i = 0; i < 4; i++) {
    loopData[i] = new double[len];
  }
  for (int i = 0; i < 4; i++) {
    initData(loopData[i], i, len);
  }

  Real_ptr x = loopData[0];
  Real_ptr y = loopData[1];
  Real_ptr z = loopData[2];
  Real_ptr vol = loopData[3];

  ADomain domain(0, /* ndims = */ 3);

  Real_ptr x0, x1, x2, x3, x4, x5, x6, x7;
  Real_ptr y0, y1, y2, y3, y4, y5, y6, y7;
  Real_ptr z0, z1, z2, z3, z4, z5, z6, z7;

  NDPTRSET(x, x0, x1, x2, x3, x4, x5, x6, x7);
  NDPTRSET(y, y0, y1, y2, y3, y4, y5, y6, y7);
  NDPTRSET(z, z0, z1, z2, z3, z4, z5, z6, z7);

  const double vnormq = 0.083333333333333333; /* vnormq = 1/12 */
  const int num_samples = 100;
  for (int isamp = 0; isamp < num_samples; ++isamp) {
#pragma omp parallel for schedule(static)
    for (int i = domain.fpz; i <= domain.lpz; i++) {
      double x71 = x7[i] - x1[i];
      double x72 = x7[i] - x2[i];
      double x74 = x7[i] - x4[i];
      double x30 = x3[i] - x0[i];
      double x50 = x5[i] - x0[i];
      double x60 = x6[i] - x0[i];

      double y71 = y7[i] - y1[i];
      double y72 = y7[i] - y2[i];
      double y74 = y7[i] - y4[i];
      double y30 = y3[i] - y0[i];
      double y50 = y5[i] - y0[i];
      double y60 = y6[i] - y0[i];

      double z71 = z7[i] - z1[i];
      double z72 = z7[i] - z2[i];
      double z74 = z7[i] - z4[i];
      double z30 = z3[i] - z0[i];
      double z50 = z5[i] - z0[i];
      double z60 = z6[i] - z0[i];

      double xps = x71 + x60;
      double yps = y71 + y60;
      double zps = z71 + z60;

      double cyz = y72 * z30 - z72 * y30;
      double czx = z72 * x30 - x72 * z30;
      double cxy = x72 * y30 - y72 * x30;
      vol[i] = xps * cyz + yps * czx + zps * cxy;

      xps = x72 + x50;
      yps = y72 + y50;
      zps = z72 + z50;

      cyz = y74 * z60 - z74 * y60;
      czx = z74 * x60 - x74 * z60;
      cxy = x74 * y60 - y74 * x60;
      vol[i] += xps * cyz + yps * czx + zps * cxy;

      xps = x74 + x30;
      yps = y74 + y30;
      zps = z74 + z30;

      cyz = y71 * z50 - z71 * y50;
      czx = z71 * x50 - x71 * z50;
      cxy = x71 * y50 - y71 * x50;
      vol[i] += xps * cyz + yps * czx + zps * cxy;

      vol[i] *= vnormq;
    }
  }
}