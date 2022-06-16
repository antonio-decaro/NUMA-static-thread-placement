#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <papi.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include "utils.h"
#include "counting.h"

static const char* output = NULL;
static const char* eventnames[MAX_EVENTS];
static unsigned    nevents = 0;
static char*       sample_info = "NA";

/**
 * run PAPI library function call and check return value against expected value. 
 * If different print error message in __VA_ARGS__.
 **/
#define PAPI_call_check(call, check_against, ...) do{	\
    int PAPI_err = 0;					\
    if((PAPI_err = (call)) != (check_against)){		\
      fprintf(stderr, __VA_ARGS__);			\
      PAPI_handle_error(PAPI_err);			\
      exit(EXIT_FAILURE);				\
    }							\
  } while(0)

static void
PAPI_handle_error(int err)
{
  if(err!=0)
    fprintf(stderr,"PAPI error %d: ",err);
  switch(err){
  case PAPI_EINVAL:
    fprintf(stderr,"Invalid argument.\n");
    break;
  case PAPI_ENOINIT:
    fprintf(stderr,"PAPI library is not initialized.\n");
    break;
  case PAPI_ENOMEM:
    fprintf(stderr,"Insufficient memory.\n");
    break;
  case PAPI_EISRUN:
    fprintf(stderr,"Eventset is already_counting events.\n");
    break;
  case PAPI_ECNFLCT:
    fprintf(stderr,"This event cannot be counted simultaneously with another event in the monitor eventset.\n");
    break;
  case PAPI_ENOEVNT:
    fprintf(stderr,"This event is not available on the underlying hardware.\n");
    break;
  case PAPI_ESYS:
    fprintf(stderr, "A system or C library call failed inside PAPI, errno:%s\n",strerror(errno)); 
    break;
  case PAPI_ENOEVST:
    fprintf(stderr,"The EventSet specified does not exist.\n");
    break;
  case PAPI_ECMP:
    fprintf(stderr,"This component does not support the underlying hardware.\n");
    break;
  case PAPI_ENOCMP:
    fprintf(stderr,"Argument is not a valid component. PAPI_ENOCMP\n");
    break;
  case PAPI_EBUG:
    fprintf(stderr,"Internal error, please send mail to the developers.\n");
    break;
  default:
    fprintf(stderr,"Unknown error ID, sometimes this error is due to \"/proc/sys/kernel/perf_event_paranoid\" not set to -1\n");
    break;
  }
}

static int add_event(const char* name){
  if(nevents+1 > MAX_EVENTS){
    fprintf(stderr, "Failed to add event %s\n", name);
    fprintf(stderr, "To many events to record.\n");
    fprintf(stderr, "The maximum number of events is %d\n", MAX_EVENTS);
    return 1;
  }
  
  eventnames[nevents] = name;
  nevents++;
  return 0;
}

struct eventset{
  int             evset;
  unsigned        n_events;
  long long *     values;
  long long *     old_values;
  struct timespec ts;
  long long       nanoseconds;
  char**          names;
};

static struct eventset * eventset_init(){
  struct eventset * evset;
  malloc_chk(evset, sizeof(*evset));
  evset->n_events=0;
  evset->evset = PAPI_NULL;
  evset->values = NULL;
  evset->old_values = NULL;
  evset->names = NULL;
  evset->nanoseconds = 0;
  PAPI_call_check(PAPI_create_eventset(&evset->evset), PAPI_OK, "Eventset creation failed: ");
  return evset;
}

static void eventset_destroy(struct eventset * evset){
  unsigned i;
  if(evset == NULL){return;}
  if(evset->n_events){
    PAPI_call_check(PAPI_stop(evset->evset, evset->values), PAPI_OK, "Eventset stop failed: ");
  }
  if(evset->values != NULL){ free(evset->values); }
  if(evset->old_values != NULL){ free(evset->old_values); }
  if(evset->names != NULL){
    for(i=0; i<evset->n_events; i++){free(evset->names[i]);}
    free(evset->names);
  }
  free(evset);
}

static int eventset_add_named_event(struct eventset * evset, const char * event)
{
  char * evt = strdup(event);
  PAPI_call_check(PAPI_add_named_event(evset->evset, evt), PAPI_OK, "Failed to add event %s to eventset: ",(char*)event);
  free(evt);
  evset->n_events++;
  realloc_chk(evset->values    , evset->n_events*sizeof(*evset->values));
  realloc_chk(evset->old_values, evset->n_events*sizeof(*evset->old_values));
  realloc_chk(evset->names     , evset->n_events*sizeof(*evset->names));
  evset->values[evset->n_events-1]     =  0;
  evset->old_values[evset->n_events-1] =  0;
  evset->names[evset->n_events-1]      =  strdup(event);
  return 0;
}

static int eventset_read(struct eventset * evset){
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  evset->nanoseconds = ts_diff(ts, evset->ts);
  swap_ptr(evset->old_values, evset->values);
  if(evset->n_events){
    int err = PAPI_read(evset->evset, evset->values);
    if(err != PAPI_OK){
      PAPI_handle_error(err);
      return -1;
    }
  }
  clock_gettime(CLOCK_MONOTONIC, &evset->ts);
  return 0;
}

static FILE* open_output(){
  FILE * out = NULL;
  if(output == NULL){
    output = getenv(OUTPUT_ENV);
    if(output == NULL){ out = stdout; }
  }
  else if(! strcmp(output, "stdout")){
    out = stdout;
  }
  else if(! strcmp(output, "1")){
    out = stdout;
  }
  else if(! strcmp(output, "stderr")){
    out = stderr;
  }
  else if(! strcmp(output, "0")){
    out = stderr;
  }
  else {
    out = fopen(output, "a+");
    if(out == NULL){ perror("fopen"); out = stdout; }    
  }
  return out;
}

static void eventset_print(struct eventset * evset){
  unsigned i;
  size_t pos;
  FILE * out = open_output();
  // print header if file is empty
  fseek(out, 0, SEEK_END);
  pos = ftell(out);
  if (pos == 0) {
    fprintf(out, "%20s ", "info");
    fprintf(out, "%20s ", "nanoseconds");
    for(i=0; i<evset->n_events; i++){ fprintf(out, "%20s ", evset->names[i]); }
    fprintf(out, "\n");    
  }
  fprintf(out, "%20s ", sample_info);
  fprintf(out, "%20ld", evset->nanoseconds);
  for(i=0; i<evset->n_events; i++){
    fprintf(out, "%20lld ", evset->values[i] - evset->old_values[i]);
  }
  fprintf(out, "\n");
  fclose(out);
}


//#############################################################################################################################
//                                                        User Functions
//#############################################################################################################################

void counting_set_output(const char* output_filename){
  output = output_filename;
}

void counting_set_info_field(const char* name){
  if(name == NULL){
    name = getenv(SAMPLE_ENV);
    if(name == NULL){
      fprintf(stderr, "Define env %s to set the info field of output.\n", SAMPLE_ENV);
    }
  }
  if(name != NULL){
    sample_info = strdup(name);
  }
  return;
}

void counting_set_events(const char** names, const unsigned n_events){
  unsigned i; nevents = 0; for(i=0; i<n_events; i++){ eventnames[i] = ""; }
  
  if(n_events == 0 || names == NULL){    
    int    err     = 0;
    char * events  = getenv(EVENTSET_ENV);
    char * saveptr = NULL;
    char * event   = strtok_r(events, " ", &saveptr);
    while(event != NULL){
      err   = add_event(event);
      event = strtok_r(NULL, " ", &saveptr);
    }    
  } else {
    for(i=0; i<n_events; i++){ add_event(names[i]); }
  }
  return;
}

struct eventset * counting_init(const pid_t pid){
  int    err                   = 0;
  unsigned i                   = 0;
  struct eventset * evset      = NULL;
  long   pos;

  PAPI_call_check(PAPI_library_init(PAPI_VER_CURRENT), PAPI_VER_CURRENT, "PAPI library version mismatch: ");
  PAPI_call_check(PAPI_is_initialized(), PAPI_LOW_LEVEL_INITED, "PAPI library init error: ");
  evset = eventset_init();
  
  for(i = 0; i<nevents; i++){
    err = eventset_add_named_event(evset, eventnames[i]);
    if(err != PAPI_OK){
      fprintf(stderr, "No eventset defined. Please set env %s to a whitespace separated list of events.\n", EVENTSET_ENV);
      eventset_destroy(evset);
    }
  }
  if(pid > 0){
    PAPI_call_check(PAPI_attach(evset->evset, pid), PAPI_OK, "Eventset attach failed: ");
  }
  if(nevents){
    PAPI_call_check(PAPI_start(evset->evset), PAPI_OK, "Eventset start failed: ");
  }
  for(i=0; i<10; i++){ eventset_read(evset); }
  return evset;
}

void counting_fini(struct eventset* evset){  
  eventset_destroy(evset);
}

void counting_start(struct eventset* evset){
  eventset_read(evset);  
}

void counting_stop(struct eventset* evset){
  eventset_read(evset);
  eventset_print(evset);
}

