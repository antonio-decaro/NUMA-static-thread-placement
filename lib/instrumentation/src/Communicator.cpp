#include "Communicator.hpp"

#include <stdio.h>
#include <string.h>

#include <iostream>

#include "Constant.hpp"
#include "Metrics.hpp"
using namespace std;
using namespace Affinity;

Communicator::Communicator()
    : _name(""), _bytes(MAX_THREADS), _touches(MAX_THREADS) {}

Communicator::Communicator(const string& name)
    : _name(name), _bytes(MAX_THREADS), _touches(MAX_THREADS) {}

Communicator::Communicator(const Communicator& copy)
    : _name(copy._name), _bytes(copy._bytes), _touches(copy._touches) {}

Matrix<long> Communicator::touches() const { return _touches; }
Matrix<long> Communicator::bytes() const { return _bytes; }

void CommTrace::set_output(const string& output) { _output = output; }

void Communicator::reset() {
  _bytes.set(0);
  _touches.set(0);
}

void Communicator::operator+=(const Communicator& other) {
  _bytes += other._bytes;
  _touches += other._touches;
}

string Communicator::name() const { return _name; }

size_t Communicator::enter(const unsigned& tid) {
  _recurse[tid]++;
  return _recurse[tid] - 1;
}
size_t Communicator::leave(const unsigned& tid) {
  _recurse[tid] = _recurse[tid] == 0 ? 0 : _recurse[tid] - 1;
  return _recurse[tid];
}

void Communicator::read(const unsigned& reader, const unsigned& owner,
                        const size_t& size) {
  _bytes(reader, owner) += size;
  _touches(reader, owner)++;
}

void Communicator::fprint(const char* dir) const {
  string pre = string(dir) + "/" + _name;
  _bytes.print((pre + ".bytes.mat").c_str());
  _touches.print((pre + ".touches.mat").c_str());
}

void Communicator::print() const {
  cout << _name << endl;
  _bytes.print();
}

bool CommTrace::enabled = false;

CommTrace::CommTrace() : _output("Communicator"), _global_comm("global_comm") {}

CommTrace::~CommTrace() {
  for (int i = 0; i < MAX_THREADS; i++) {
    while (!_traces[i].empty()) {
      Communicator* last = _traces[i].back();
      pop(last, i);
    }
  }
  fprint(_output);
}

void CommTrace::clone(const tid_t& parent, const tid_t& child) {
  // Insert parent backtrace as child backtrace.
  tid_t ctid = child % MAX_THREADS;
  _traces[ctid].clear();
  _traces[ctid].insert(_traces[ctid].begin(), _traces[parent].begin(),
                       _traces[parent].end());
  _current[ctid].reset();
}

void CommTrace::write(const unsigned& writer, const size_t& size,
                      const addr_t& addr) {
  _data.write(addr, writer, size);
}

void CommTrace::read(const unsigned& reader, const size_t& size,
                     const addr_t& addr) {
  const tid_t owner = _data.read(addr, reader, size);
  _global_comm.read(reader, owner, size);
  _current[reader].read(reader, owner, size);
}

Communicator* CommTrace::create_comm(const string& name, const addr_t& addr) {
  map<addr_t, Communicator*>::iterator found = _comm.find(addr);
  if (found == _comm.end()) {
    found = _comm.insert(make_pair(addr, new Communicator(name))).first;
  }
  return found->second;
}

void CommTrace::push(Communicator* new_comm, const tid_t tid) {
  /* If communicator is already in the trace, do not add it but increase the
   * count of recursion */
  if (new_comm->enter(tid) == 0) {
    for (auto comm = _traces[tid].begin(); comm != _traces[tid].end(); comm++) {
      **comm += _current[tid];
    }
    _traces[tid].push_back(new_comm);
    _current[tid].reset();
  }
}

void CommTrace::pop(Communicator* comm, const tid_t& tid) {
  /* Pop last element */
  if (comm->leave(tid) == 0 && !_traces[tid].empty()) {
    if (comm != _traces[tid].back()) {
#ifdef DEBUG
      fprintf(stderr, "trace end %s mismatch\n", comm->name().c_str());
      fprint(tid);
#endif
    } else {
      /* Propagate current communicator to the last element */
      *comm += _current[tid];
      _traces[tid].pop_back();
    }
  }
}

void CommTrace::fprint(const string& dir) {
  for (auto comm = _comm.begin(); comm != _comm.end(); comm++) {
    comm->second->fprint(dir.c_str());
  }
  _global_comm.fprint(dir.c_str());

  AffinityMetrics metrics = AffinityMetrics();
  metrics.add_matrix_metrics(_global_comm.touches().toDouble(),
                             string("touches"));
  metrics.add_PageTable_metrics(_data);
  metrics.print(dir, "metrics.csv");
}

void CommTrace::fprint(const unsigned& i) {
  char begin[8];
  memset(begin, 0, sizeof(begin));
  snprintf(begin, sizeof(begin), "%u: ", i);
  string out = string(begin);
  for (auto comm = _traces[i].begin(); comm != _traces[i].end(); comm++) {
    out += (*comm)->name() + string(" <- ");
  }
  fprintf(stderr, "%s\n", out.c_str());
}

#ifdef PIN_H

VOID CommTrace::InstrumentWrite(CommTrace* comm, ADDRINT memAddr, THREADID tid,
                                UINT32 size) {
  if (CommTrace::enabled) {
#ifdef DEBUG
    fprintf(stderr, "%d write @%lu[%u]\n", tid, memAddr, size);
#endif
    comm->write(tid % MAX_THREADS, size, memAddr);
  }
}

VOID CommTrace::InstrumentRead(CommTrace* comm, ADDRINT memAddr, THREADID tid,
                               UINT32 size) {
  if (CommTrace::enabled) {
    comm->read(tid % MAX_THREADS, size, memAddr);
  }
}

VOID CommTrace::InstrumentINS(INS ins) {
  if (INS_IsMemoryRead(ins) && !INS_IsStackRead(ins)) {
    INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)CommTrace::InstrumentRead,
                   IARG_PTR, this, IARG_MEMORYREAD_EA, IARG_THREAD_ID,
                   IARG_MEMORYREAD_SIZE, IARG_END);
  } else if (INS_HasMemoryRead2(ins) && !INS_IsStackRead(ins)) {
    INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)CommTrace::InstrumentRead,
                   IARG_PTR, this, IARG_MEMORYREAD2_EA, IARG_THREAD_ID,
                   IARG_MEMORYREAD_SIZE, IARG_END);
  } else if (INS_IsMemoryWrite(ins) && !INS_IsStackWrite(ins)) {
    INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)CommTrace::InstrumentWrite,
                   IARG_PTR, this, IARG_MEMORYWRITE_EA, IARG_THREAD_ID,
                   IARG_MEMORYWRITE_SIZE, IARG_END);
  }
}

VOID CommTrace::RoutineBegin(CommTrace* trace, Communicator* comm,
                             THREADID tid) {
  if (CommTrace::enabled) {
    trace->push(comm, tid % MAX_THREADS);
  }
}

VOID CommTrace::RoutineEnd(CommTrace* trace, Communicator* comm, THREADID tid) {
  if (CommTrace::enabled) {
    trace->pop(comm, tid % MAX_THREADS);
  }
}

VOID CommTrace::InstrumentRTN(RTN rtn) {
  RTN_Open(rtn);
  if (KnobTrace) {
    ADDRINT addr = RTN_Address(rtn);
    _routines[addr] = rtn;
    Communicator* comm = create_comm(RTN_Name(rtn), addr);
    RTN_InsertCall(rtn, IPOINT_BEFORE, (AFUNPTR)RoutineBegin, IARG_PTR, this,
                   IARG_PTR, comm, IARG_THREAD_ID, IARG_END);
    RTN_InsertCall(rtn, IPOINT_AFTER, (AFUNPTR)RoutineEnd, IARG_PTR, this,
                   IARG_PTR, comm, IARG_THREAD_ID, IARG_END);
  }
  for (INS ins = RTN_InsHead(rtn); INS_Valid(ins); ins = INS_Next(ins)) {
    InstrumentINS(ins);
  }
  RTN_Close(rtn);
}

#endif

Communicator CommTrace::communications() const {
  return Communicator(_global_comm);
}
