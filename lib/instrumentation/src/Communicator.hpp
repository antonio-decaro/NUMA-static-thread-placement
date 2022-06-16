#ifndef COMMUNICATOR_HPP
#define COMMUNICATOR_HPP

#include <stdint.h>

#include <list>
#include <string>

#include "Constant.hpp"
#include "Matrix.hpp"
#include "PageTable.hpp"
#include "Types.hpp"

namespace Affinity {
class Communicator {
 private:
  std::string _name;
  Matrix<long> _bytes;
  Matrix<long> _touches;
  size_t _recurse[MAX_THREADS];  // number of time entered

 public:
  Communicator();
  Communicator(const std::string& name);
  Communicator(const Communicator& copy);

  void operator+=(const Communicator& other);
  void read(const unsigned& reader, const unsigned& owner, const size_t& size);
  void reset();

  Matrix<long> touches() const;
  Matrix<long> bytes() const;
  std::string name() const;
  void fprint(const char* dir) const;
  void print() const;
  size_t enter(const unsigned& tid);
  size_t leave(const unsigned& tid);
};

#ifdef PIN_H
KNOB<INT32> KnobTrace(KNOB_MODE_WRITEONCE, "pintool", "t", "0",
                      "per function affinity activation");
#endif

class CommTrace {
 private:
  std::string _output;
  // Record accesses to catch communications.
  PageTable _data;
  // Hold a Communication matrix per routine (identified by address).
  std::map<addr_t, Communicator*> _comm;
  // Hold a Communication matrix for the whole application.
  Communicator _global_comm;
  // Hold backtraces for each thread.
  std::list<Communicator*>
      _traces[MAX_THREADS];  // In case of recursion do not count twice.
  // Current function communications for each thread
  Communicator _current[MAX_THREADS];

#ifdef PIN_H
  std::map<addr_t, RTN> _routines;  // Map routines to their address to find
                                    // them back at instrumentation step.

  static VOID InstrumentWrite(CommTrace*, ADDRINT memAddr, THREADID tid,
                              UINT32 size);
  static VOID InstrumentRead(CommTrace*, ADDRINT memAddr, THREADID tid,
                             UINT32 size);
  VOID InstrumentINS(INS ins);
  static VOID RoutineBegin(CommTrace* trace, Communicator* comm, THREADID tid);
  static VOID RoutineEnd(CommTrace* trace, Communicator* comm, THREADID tid);
#endif

  Communicator* create_comm(const std::string& name, const addr_t& addr);
  void push(Communicator* new_comm, const tid_t tid);
  void pop(Communicator* comm, const tid_t& tid);
  void write(const tid_t& writer, const size_t& size, const addr_t& addr);
  void read(const tid_t& reader, const size_t& size, const addr_t& addr);

 public:
  static bool enabled;
  CommTrace();
  ~CommTrace();

  Communicator communications() const;
  void set_output(const std::string& output);
  void clone(const tid_t& parent, const tid_t& child);
  void fprint(const std::string& dir);
  void fprint(const unsigned& i);

#ifdef PIN_H
  VOID InstrumentRTN(RTN rtn);
#endif
};
};  // namespace Affinity

#endif
