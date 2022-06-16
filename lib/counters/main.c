#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "counting.h"

#define EVENTS_ENV "COUNTING_EVENTSET"

static char *      output_str              = NULL;
static char**      prog_args               = NULL;
static const char* eventnames[MAX_EVENTS];
static unsigned    nevents                 = 0;

static void usage(char * argv0){
  printf("USAGE:\n");
  printf("%s ", argv0);
  printf("-o <output_filename> ");
  printf("-e <papi event name> ");
  printf("-- prog prog_args ... \n");

  printf("\nOPTIONS:\n");
  printf("\t-o, --output: Output file where to write the results\n");
  printf("\t-e, --event: Papi event name to append to the list of events to record\n");
  printf("\t-- prog prog_args ...: The program to start with its argument\n");

  printf("\nENVIRONMENT:\n");
  printf("\t%s: File where to output results. Option '-o' has priority over env\n", OUTPUT_ENV);
  printf("\t%s: Blank space separated list of papi counters. Is appended to the list with options '-e'\n", EVENTSET_ENV);
}

void add_event(const char* name){
  if(nevents+1 > MAX_EVENTS){
    fprintf(stderr, "To many events to record. The maximum number of events is %d\n", MAX_EVENTS);
    return;
  }
  eventnames[nevents] = name;
  nevents++;
  return;
}

static void parse_args(int argc, char ** argv){
  int i;
  for(i=1;i<argc;i++){
    if(!strcmp(argv[i], "-o") || ! strcmp(argv[i], "--output")){
      if(i+1 == argc){
	usage(argv[0]);
      } else {
	output_str = argv[++i];
      }      
    }
    else if(!strcmp(argv[i], "-e") || ! strcmp(argv[i], "--event")){
      if(i+1 == argc){
	usage(argv[0]);
      } else {
	add_event(argv[++i]);
      }
    }
    else if(strlen(argv[i]) == 2 && !strcmp(argv[i], "--")){
      prog_args = &(argv[i+1]);
      break;
    }   
    else{ usage(argv[0]); }
  }
  if(prog_args == NULL){
    fprintf(stderr, "Must be used with a program\n");
    usage(argv[0]);
    exit(1);    
  }
}

int main(int argc, char** argv){
  parse_args(argc, argv);
  counting_set_output(output_str);
  counting_set_events(eventnames, nevents);
  counting_set_info_field(prog_args[0]);

  pid_t pid = fork();
  if(pid == 0) {
    if(execvp(prog_args[0], prog_args) == -1){
      perror("execvp");
      return -1;
    }
    return 0;
  }
  else if(pid > 0){
    int status = 0;
    struct eventset * evset = counting_init(pid);
    counting_start(evset);

    waitpid(pid, &status, 0);

    counting_stop(evset);
    counting_fini(evset);
  }
  else {
    /* fork error */  
    perror("fork");
  } 
  return 0;
}
