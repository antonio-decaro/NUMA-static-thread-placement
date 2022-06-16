#include <stdio.h>
#include <stdlib.h>

#include "counting.h"

int main() {
  counting_set_output(getenv("RESULT_DIR"));

  struct eventset* evset1;
  struct eventset* evset2;

  const char* event1[] = {
      "PAPI_L2_DCM",
      "PAPI_BR_INS",
      "PAPI_SR_INS",
      "PAGE-FAULTS",
  };
  // const char* event1[] = {"PAPI_TOT_CYC", "PAPI_LD_INS", "PAPI_SR_INS",
  //                         "PAPI_BR_INS"};
  // const char* event2[] = {"PAPI_L1_DCM", "PAPI_L2_DCM", "PAPI_L3_TCM"};

  /* Line output line with counters close to 0 */
  // counting_set_info_field("void");
  // counting_start(evset);
  // counting_stop(evset);
  // counting_fini(evset);

  // if (getenv(EVENTSET_ENV) == NULL) {
  //   setenv(EVENTSET_ENV, EVSET, 1);
  //   counting_set_events(NULL, 0);
  //   evset = counting_init(0);
  //   counting_set_info_field("test");
  //   counting_start(evset);
  //   counting_stop(evset);
  //   counting_fini(evset);
  //   unsetenv(EVENTSET_ENV);

  //   setenv(EVENTSET_ENV, EVSET1, 1);
  //   counting_set_events(NULL, 0);
  //   counting_set_info_field("testcache");
  //   evset1 = counting_init(0);

  //   counting_start(evset1);
  //   counting_stop(evset1);
  //   counting_fini(evset1);
  //   unsetenv(EVENTSET_ENV);

  // }

  counting_set_events(event1, 4);
  counting_set_info_field("test");
  evset1 = counting_init(0);
  counting_start(evset1);
  counting_stop(evset1);
  counting_fini(evset1);

  // counting_set_events(event2, 3);
  // counting_set_info_field("testCache");
  // evset2 = counting_init(0);
  // counting_start(evset2);

  // printf("hello");

  // counting_stop(evset2);
  // counting_fini(evset2);

  return 0;
}
