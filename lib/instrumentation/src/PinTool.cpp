#include <assert.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <cstdio>
#include <list>
#include <map>
#include <string>

#include "pin.H"

/* Utils for mapping threads affinity */
#include "Communicator.cpp"
#include "Constant.hpp"
#include "Matrix.cpp"
#include "Metrics.cpp"
#include "PageTable.cpp"
#include "ROImanagement.cpp"
#include "Stats.cpp"

using namespace std;
using namespace Affinity;

#define MAX_PATH 10000
char path[MAX_PATH];               // tool directory
map<OS_THREAD_ID, THREADID> tids;  // Convert from OS to PIN tids.
PIN_RWMUTEX lock;                  // Lock for safe multithread access.
string bin_name = "";              // binary name
CommTrace comm;  // Stores datamap for program and communication matrices for
                 // each function.

VOID countThreads(THREADID tid, __attribute__((unused)) CONTEXT* ctxt,
                  __attribute__((unused)) INT32 flags,
                  __attribute__((unused)) VOID* v) {
  PIN_RWMutexWriteLock(&lock);
  tids[PIN_GetTid()] = tid;                    // Register thread os id
  THREADID parent = tids[PIN_GetParentTid()];  // Get parent tid
  PIN_RWMutexUnlock(&lock);
  comm.clone(parent, tid);
}

VOID InstrumentRTN(RTN rtn) {
  string name = RTN_Name(rtn);
  // Create an affinity matrix
  if (name[0] != '.' && name != "_init" && name != "_fini" &&
      name != "_start" && name != "frame_dummy" &&
      strncmp(name.c_str(), "__", 2) && name.rfind("libc") == string::npos) {
    LOG("Instrument routine " + name + "\n");
    comm.InstrumentRTN(rtn);
  }
}

/* Instrument every memory instruction to catch communications */
void InstrumentIMG(IMG img, __attribute__((unused)) VOID* v) {
  if (!IMG_Valid(img)) {
    fprintf(stderr, "Invalid image\n");
    return;
  }
  for (SEC sec = IMG_SecHead(img); SEC_Valid(sec); sec = SEC_Next(sec)) {
    /* Iterate over all routines */
    for (RTN rtn = SEC_RtnHead(sec); RTN_Valid(rtn); rtn = RTN_Next(rtn)) {
      InstrumentRTN(rtn);
      InstrumentROI(rtn);
    }
  }
}

/* Find the application routines for wich we count communication, not detailing
 * every dynamically loaded library */
void InstrumentAPP(VOID* v) {
  IMG app = APP_ImgHead();
  bin_name = IMG_Name(app);
  if (mkdir(bin_name.c_str(), S_IRWXU) == -1) {
    if (errno != EEXIST) {
      perror("mkdir");
      bin_name = string("");
    }
  }
  string::size_type len = bin_name.length();
  string::size_type pos = bin_name.rfind("/", len);
  if (pos != string::npos && pos < len) {
    bin_name = bin_name.substr(pos + 1, len);
  }
  comm.set_output(((path) + bin_name));
  InstrumentIMG(app, NULL);
}

/* Output results */
void Cleanup(__attribute__((unused)) INT32 Code,
             __attribute__((unused)) VOID* arg) {
  PIN_RWMutexFini(&lock);
  // Create output directory
}

int main(int argc, char* argv[]) {
  const char* env = getenv("AFFINITY_WD");
  memset(path, 0, sizeof(path));
  if (env == NULL) {
    getcwd(path, MAX_PATH);
    path[strlen(path)] = '/';
  } else {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstringop-truncation"
        strncpy(path, env, MAX_PATH);
#pragma GCC diagnostic pop
    if (path[strlen(path) - 1] != '/') {
      path[strlen(path)] = '/';
    }
  }
  if (PIN_Init(argc, argv)) {
    fprintf(stderr, "Error detected while parsing command line\n");
  }
  PIN_InitSymbols();
  PIN_RWMutexInit(&lock);
  PIN_AddThreadStartFunction((THREAD_START_CALLBACK)countThreads, NULL);
  IMG_AddInstrumentFunction(InstrumentIMG, NULL);
  PIN_AddFiniFunction(Cleanup, NULL);
  PIN_AddApplicationStartFunction(InstrumentAPP, NULL);
  PIN_StartProgram();
}
