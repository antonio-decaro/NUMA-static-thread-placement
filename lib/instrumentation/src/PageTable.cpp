#include <stdlib.h>
#include <iostream>
#include "PageTable.hpp"
#include "Matrix.hpp"

using namespace Affinity;

#ifndef PIN_H
#define MIN(a,b) ((a)<(b)?(a):(b))
#endif

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////                      Cacheline                           ////////////////////////////////// 
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

Cacheline::Cacheline(const Cacheline& copy):_addr(copy._addr),
					    _owner(copy._owner),
					    _state(copy._state){
  for(int i = 0; i<MAX_THREADS; i++){
    _states[i]  = copy._states[i];
    _read[i]    = copy._read[i];
    _write[i]   = copy._write[i];
  }
}

Cacheline::Cacheline(const addr_t& addr): _addr(addr),
					  _owner(0),
					  _state(0){
  for(int i = 0; i<MAX_THREADS; i++){
    _states[i]  = 0;
    _read[i]    = 0;
    _write[i]   = 0;
  }
}

void Cacheline::fprint(FILE * output) const{
  for(unsigned i = 0; i<MAX_THREADS; i++){
    fprintf(output, "%u %u %lu %lu\n", _addr, i, _read[i], _write[i]);
  }
}

bool Cacheline::is_shared() const{
  unsigned s = 0;
  for(unsigned i=0; i<MAX_THREADS; i++){ s += (_read[i] > 0 || _write[i] > 0); }
  return s > 1;
}

tid_t Cacheline::write(const tid_t& tid, const unsigned& size){
  tid_t prev = __sync_lock_test_and_set (&_owner, tid);
  _states[tid] = __sync_fetch_and_add(&_state, 1);  
  _write[tid]  += 1;
  return prev;
}

tid_t Cacheline::read(const tid_t& tid, const unsigned& size){
  _read[tid]  += 1;
  if(_state != __sync_lock_test_and_set (_states+tid, _state)){
    return _owner;
  } else {
    return tid;
  }
}

unsigned long Cacheline::writes() const{
  unsigned i,w = 0;
  for(i=0;i<MAX_THREADS;i++){ w += _write[i]; }
  return w;
}

unsigned long Cacheline::reads() const{
  unsigned i,r = 0;
  for(i=0;i<MAX_THREADS;i++){ r += _read[i]; }
  return r;
}

double Cacheline::write_ratio() const{
  unsigned i;
  unsigned long w = 0, r = 0;
  for(i=0;i<MAX_THREADS;i++){ w += _write[i]; r += _read[i]; }  
  return (double)w/(double)(w+r);
}

unsigned Cacheline::sharing_degree() const{
  unsigned i,degree = 0;
  for(i=0;i<MAX_THREADS;i++){
    degree += _read[i] > 0 || _write[i] > 0;
  }
  return degree;
}

unsigned Cacheline::writing_degree() const{
  unsigned i,degree = 0;
  for(i=0;i<MAX_THREADS;i++){
    degree += _write[i] > 0;
  }
  return degree;
}

Matrix<long> Cacheline::sharing_matrix() const{
  Matrix<long> ret = Matrix<long>(MAX_THREADS);
  unsigned i, j;
  for(i=0;i<MAX_THREADS;i++){
    for(j=0;j<=i;j++){
      ret(i,j) = MIN(_read[i], _read[j]) + MIN(_write[i], _write[j]);
      ret(j,i) = ret(i,j);
    }
  }
  return ret;
}

void Cacheline::touches(size_t* out) const{
  for(int i=0; i<MAX_THREADS; i++){ out[i] += _read[i] + _write[i]; }
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////                         Page                             ////////////////////////////////// 
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

Page::Page(): _cacheline(NULL){
  lock_init(&_lock);
}

Page::~Page(){
  lock_wrlock(&_lock);  
  if(_cacheline != NULL){
    for(int i=0; i<CACHELINE_PER_PAGE; i++){
      if( _cacheline[i] != NULL ){ delete _cacheline[i]; }
    }
    free(_cacheline);
  }
  lock_unlock(&_lock);  
  lock_fini(&_lock);
}

void Page::allocate(){
  lock_wrlock(&_lock);
  if(_cacheline == NULL){
    _cacheline = (Cacheline**)malloc(sizeof(Cacheline*)*CACHELINE_PER_PAGE);
    for(int i=0; i<CACHELINE_PER_PAGE; i++){ _cacheline[i] = NULL; }
  }
  lock_unlock(&_lock);  
}

void Page::allocate(const int& id, const addr_t& addr){  
  lock_wrlock(&_lock);
  if(_cacheline[id] == NULL){
    _cacheline[id] = new Cacheline(addr);
  }
  lock_unlock(&_lock);
}

const Cacheline* Page::cacheline(const int& id) const{
  if(_cacheline == NULL){ return NULL; } else { return _cacheline[id]; }
}

tid_t Page::read(const addr_t& addr, const tid_t& tid, const unsigned& size){
  addr_t id = CACHELINE_ID(addr);  
  if(_cacheline     == NULL){ allocate(); }
  if(_cacheline[id] == NULL){ allocate(id, addr); }  
  return _cacheline[id]->read(tid, size);  
}

tid_t Page::write(const addr_t& addr, const tid_t& tid, const unsigned& size){
  addr_t id = CACHELINE_ID(addr);
  if(_cacheline     == NULL){ allocate(); }
  if(_cacheline[id] == NULL){ allocate(id, addr); }  
  return _cacheline[id]->write(tid, size);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////                         Pagetable                        ////////////////////////////////// 
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

PageTableStats::PageTableStats(): sharing_matrix(MAX_THREADS) {
  sharing_degree     = OnlineStats<double>();
  writing_degree     = OnlineStats<double>();
  write_ratio        = OnlineStats<double>();
  shared_write_ratio = OnlineStats<double>();
  footprint          = Stats<unsigned long>();
}
  
PageTable::PageTable(){}

tid_t PageTable::read(const addr_t& addr, const tid_t& tid, const unsigned& size){  
  return _pages[PAGE_ID(addr)].read(addr, tid%MAX_THREADS, size);
}

tid_t PageTable::write(const addr_t& addr, const tid_t& tid, const unsigned& size){
  return _pages[PAGE_ID(addr)].write(addr, tid%MAX_THREADS, size);
}

PageTableStats* PageTable::stats() const{
  PageTableStats * st = new PageTableStats();
  unsigned long f[MAX_THREADS];
  std::list<unsigned long> footprint = std::list<unsigned long>();
  
  for(int i=0; i<NPAGE; i++){
    for(int j=0; j<CACHELINE_PER_PAGE; j++){
      const Cacheline * cline = _pages[i].cacheline(j);
      if(cline == NULL){ continue; }
      st->sharing_matrix += cline->sharing_matrix();
      st->sharing_degree.insert((double)cline->sharing_degree());
      st->writing_degree.insert((double)cline->writing_degree());
      st->write_ratio.insert((double)cline->write_ratio());
      if(cline->is_shared()){
	st->shared_write_ratio.insert(cline->write_ratio());
      }
      cline->touches(f);
    }
  }

  for(int i = 0; i< MAX_THREADS; i++){ footprint.push_back(f[i]); }
  st->footprint = Stats<unsigned long>(footprint);
  return st;
}

Matrix<long> PageTable::sharing_matrix() const{
  Matrix<long> mat(MAX_THREADS);
  for(int i=0; i<NPAGE; i++){
    for(int j=0; j<CACHELINE_PER_PAGE; j++){
      const Cacheline * cline = _pages[i].cacheline(j);
      if(cline == NULL){ continue; }
      mat += cline->sharing_matrix();
    }
  }
  return mat;
}

