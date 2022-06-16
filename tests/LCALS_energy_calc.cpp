#include <unistd.h>

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <string>

extern "C" {
#include "../lib/counters/counting.h"
}

void initData(double* data, int id, int len) {
  double factor = (id % 2 ? 0.1 : 0.2);
#pragma omp for schedule(static)
  for (int j = 0; j < len; ++j) {
    data[j] = factor * (j + 1.1) / (j + 1.12345);
  }
}

int main(int argc, char* argv[]) {
  int len = 64 * 64 * 1000;
  double* loopData[15];
  for (int i = 0; i < 15; i++) {
    loopData[i] = new double[len];
  }

  for (int i = 0; i < 15; i++) {
    initData(loopData[i], i, len);
  }

  const char* WITH_PAPI = getenv("WITH_PAPI");
  struct eventset* evset;
  if (WITH_PAPI) {
    counting_set_output(getenv("RESULT_DIR"));

    const char* event[] = {
        "PAPI_L2_DCM",
        "PAPI_BR_INS",
        "PAPI_SR_INS",
        "PAGE-FAULTS",
    };
    counting_set_events(event, 4);
    counting_set_info_field("LCALS_energy_calc");
    evset = counting_init(0);
    counting_start(evset);
  }

  double* e_new = loopData[0];
  double* e_old = loopData[1];
  double* delvc = loopData[2];
  double* p_new = loopData[3];
  double* p_old = loopData[4];
  double* q_new = loopData[5];
  double* q_old = loopData[6];
  double* work = loopData[7];
  double* compHalfStep = loopData[8];
  double* pHalfStep = loopData[9];
  double* bvc = loopData[10];
  double* pbvc = loopData[11];
  double* ql_old = loopData[12];
  double* qq_old = loopData[13];
  double* vnewc = loopData[14];

  double loopScalar[4];
  initData(loopScalar, 1, 4);

  const double rho0 = loopScalar[0];
  const double e_cut = loopScalar[1];
  const double emin = loopScalar[2];
  const double q_cut = loopScalar[3];

  const int num_samples = 10;

  for (int isamp = 0; isamp < num_samples; ++isamp) {
#pragma omp parallel
    {
#pragma omp for nowait schedule(static)
      for (int i = 0; i < len; i++) {
        e_new[i] =
            e_old[i] - 0.5 * delvc[i] * (p_old[i] + q_old[i]) + 0.5 * work[i];
      }

#pragma omp for nowait schedule(static)
      for (int i = 0; i < len; i++) {
        if (delvc[i] > 0.0) {
          q_new[i] = 0.0;
        } else {
          double vhalf = 1.0 / (1.0 + compHalfStep[i]);
          double ssc =
              (pbvc[i] * e_new[i] + vhalf * vhalf * bvc[i] * pHalfStep[i]) /
              rho0;

          if (ssc <= 0.1111111e-36) {
            ssc = 0.3333333e-18;
          } else {
            ssc = sqrt(ssc);
          }

          q_new[i] = (ssc * ql_old[i] + qq_old[i]);
        }
      }

#pragma omp for nowait schedule(static)
      for (int i = 0; i < len; i++) {
        e_new[i] = e_new[i] + 0.5 * delvc[i] *
                                  (3.0 * (p_old[i] + q_old[i]) -
                                   4.0 * (pHalfStep[i] + q_new[i]));
      }

#pragma omp for nowait schedule(static)
      for (int i = 0; i < len; i++) {
        e_new[i] += 0.5 * work[i];

        if (fabs(e_new[i]) < e_cut) {
          e_new[i] = 0.0;
        }

        if (e_new[i] < emin) {
          e_new[i] = emin;
        }
      }

#pragma omp for nowait schedule(static)
      for (int i = 0; i < len; i++) {
        double q_tilde;

        if (delvc[i] > 0.0) {
          q_tilde = 0.;
        } else {
          double ssc =
              (pbvc[i] * e_new[i] + vnewc[i] * vnewc[i] * bvc[i] * p_new[i]) /
              rho0;

          if (ssc <= 0.1111111e-36) {
            ssc = 0.3333333e-18;
          } else {
            ssc = sqrt(ssc);
          }

          q_tilde = (ssc * ql_old[i] + qq_old[i]);
        }

        e_new[i] = e_new[i] -
                   (7.0 * (p_old[i] + q_old[i]) -
                    8.0 * (pHalfStep[i] + q_new[i]) + (p_new[i] + q_tilde)) *
                       delvc[i] / 6.0;

        if (fabs(e_new[i]) < e_cut) {
          e_new[i] = 0.0;
        }
        if (e_new[i] < emin) {
          e_new[i] = emin;
        }
      }

#pragma omp for nowait schedule(static)
      for (int i = 0; i < len; i++) {
        if (delvc[i] <= 0.0) {
          double ssc =
              (pbvc[i] * e_new[i] + vnewc[i] * vnewc[i] * bvc[i] * p_new[i]) /
              rho0;

          if (ssc <= 0.1111111e-36) {
            ssc = 0.3333333e-18;
          } else {
            ssc = sqrt(ssc);
          }

          q_new[i] = (ssc * ql_old[i] + qq_old[i]);

          if (fabs(q_new[i]) < q_cut) q_new[i] = 0.0;
        }
      }
    }  // omp parallel
  }

  if (WITH_PAPI) {
    counting_stop(evset);
    counting_fini(evset);
  }
}