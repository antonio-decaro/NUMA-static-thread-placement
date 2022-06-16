#include <unistd.h>

#define EVENTSET_ENV "COUNTING_EVENTSET"
#define OUTPUT_ENV   "COUNTING_OUTPUT"
#define SAMPLE_ENV   "COUNTING_SAMPLE"
#define MAX_EVENTS 8

struct eventset;

/* Set event to record prior to initialization. If NULL, then get from env EVENTSET_ENV */
void counting_set_events(const char** eventnames, const unsigned n_events);

/* Initialize internal stuff */
struct eventset * counting_init(const pid_t pid);

/* Free  internal stuff */
void counting_fini(struct eventset* evset);

/* Set output. If NULL, get from env OUTPUT_ENV */
void counting_set_output(const char* output_filename);

/* Set info field in output. If NULL, then get from env SAMPLE_ENV */
void counting_set_info_field(const char* name);

/* Start recording events */
void counting_start(struct eventset* evset);

/* Stop recording events and output them to set output with set info field */
void counting_stop(struct eventset* evset);

/* #include <counting.h> */
/* struct eventset * evset = counting_init(); */
/* counting_start(evset); */
/* counting_stop(evset, NULL); */
/* counting_fini(evset); */

