#include "Allocation.hpp"
using namespace std;

bool AllocStats::enabled = false;

AllocStats::AllocStats(): _rss_tot(0){lock_init(&_lock);}
AllocStats::AllocStats(const unsigned& n): _rss_tot(0){
  _rss.resize(n, 0);
  _last_sizes.resize(n, 0);
  _local_rss_stats.resize(n, OnlineStats<unsigned long long>());
  lock_init(&_lock);
}
AllocStats::~AllocStats(){lock_fini(&_lock);}

bool AllocStats::isAlloc(const string& name){
  if(name == string("malloc")          or
     name == string("valloc")          or
     name == string("pvalloc")         or
     name == string("mmap")            or
     name == string("realloc")         or
     name == string("memalign")        or
     name == string("aligned_alloc")   or
     name == string("hbw_malloc")      or
     name == string("hbw_calloc")      or
     name == string("hbw_realloc")     or
     name == string("memkind_malloc")  or
     name == string("memkind_calloc")  or
     name == string("memkind_realloc") or
     name == string("memkind_partition_mmap"))
  {
    return true;
  } else {
    return false;
  }
}

bool AllocStats::isFree(const string& name){
  if(name == string("free")         or
     name == string("realloc")      or     
     name == string("munmap")       or
     name == string("hbw_free")     or
     name == string("hbw_realloc")  or     
     name == string("memkind_free") or
     name == string("memkind_realloc"))
  {
    return true;
  } else {
    return false;
  }
}

int AllocStats::entryPointValue(const string& name){
  if(name == string("malloc")){return 0;}
  else if(name == string("valloc")){return 0;}
  else if(name == string("pvalloc")){return 0;}    
  else if(name == string("mmap")){return 1;}
  else if(name == string("realloc")){return 1;}  
  else if(name == string("memalign")){return 1;}
  else if(name == string("aligned_alloc")){return 1;}
  else if(name == string("hbw_malloc")){return 0;}
  else if(name == string("hbw_calloc")){return 1;}
  else if(name == string("hbw_realloc")){return 1;}
  else if(name == string("memkind_malloc")){return 1;}
  else if(name == string("memkind_calloc")){return 2;}
  else if(name == string("memkind_realloc")){return 2;}
  else if(name == string("memkind_partition_mmap")){return 2;}
  else if(name == string("free")){return 0;}
  else if(name == string("munmap")){return 0;}    
  else if(name == string("hbw_free")){return 0;}  
  else if(name == string("memkind_free")){return 1;}
  else {
    return -1;
  }
}

void AllocStats::set_alloc(const unsigned& tid, const unsigned long long& size){ _last_sizes[tid] = size; }

void AllocStats::count_alloc(const unsigned& tid, unsigned long addr){  
  lock_wrlock(&_lock);
  if( tid > _rss.size() ){
    _rss.resize(tid, 0);
    _last_sizes.resize(tid, 0);
    _local_rss_stats.resize(tid, OnlineStats<unsigned long long>());
  }
  unsigned long long size = _last_sizes[tid];
  _rss_tot += size;
  _global_rss_stats.insert(_rss_tot);
  _alloc_table[addr] = size;
  lock_unlock(&_lock);
  
  _rss[tid] += size;  
  _local_rss_stats[tid].insert(_rss[tid]);  
}

void AllocStats::count_free(const unsigned &tid, unsigned long addr){
  lock_wrlock(&_lock);
  if(  tid > _rss.size()){
    lock_unlock(&_lock);
    return;
  }
  
  map<unsigned long, unsigned long long>::iterator it = _alloc_table.find(addr);
  unsigned long long size = 0;
  if( it != _alloc_table.end() ){
    size = it->second;
    it->second = 0;
  }  
  _rss_tot -= size;
  _global_rss_stats.insert(_rss_tot);      
  lock_unlock(&_lock);
  _rss[tid] -= size;
  _local_rss_stats[tid].insert(_rss[tid]);  
}

unsigned AllocStats::num_threads() const{
  return _local_rss_stats.size();  
}

OnlineStats<unsigned long long> AllocStats::rss() const{
  return _global_rss_stats;
}

OnlineStats<unsigned long long> AllocStats::rss(const unsigned& tid) const{
  return _local_rss_stats[tid];
}


#ifdef PIN_H
VOID AllocStats::allocBefore(AllocStats* stats, THREADID tid, ADDRINT size){
  if( AllocStats::enabled ){
    stats->set_alloc(tid, size);
  }
}

VOID AllocStats::allocAfter(AllocStats* stats, THREADID tid, ADDRINT memAddr){
  if( AllocStats::enabled ){
    stats->count_alloc(tid, memAddr);
  }
}

VOID AllocStats::freeBefore(AllocStats* stats, THREADID tid, ADDRINT addr){
  if( AllocStats::enabled ){
    stats->count_free(tid, addr);
  }
}

VOID AllocStats::InstrumentAlloc(const RTN& rtn){
  int epv = -1; //entry_point_value
  string name = RTN_Name(rtn);
  if( AllocStats::isAlloc(name) ){
    epv = AllocStats::entryPointValue(name);
    RTN_Open(rtn);
    RTN_InsertCall(rtn,
		   IPOINT_BEFORE, (AFUNPTR)AllocStats::allocBefore,
		   IARG_PTR, this,
		   IARG_THREAD_ID,
		   IARG_FUNCARG_ENTRYPOINT_VALUE, epv,
		   IARG_END);
    
    RTN_InsertCall(rtn,
		   IPOINT_AFTER, (AFUNPTR)AllocStats::allocAfter,
		   IARG_PTR, this,
		   IARG_THREAD_ID,
		   IARG_FUNCRET_EXITPOINT_VALUE,
		   IARG_END);
    RTN_Close(rtn);
  } else {
    return;
  }
}

VOID AllocStats::InstrumentFree(const RTN& rtn){
  int epv = -1; //entry_point_value
  string name = RTN_Name(rtn);
  if( AllocStats::isFree(name) ){
    epv = AllocStats::entryPointValue(name);
    RTN_Open(rtn);
    RTN_InsertCall(rtn,
		   IPOINT_BEFORE, (AFUNPTR)freeBefore,
		   IARG_PTR, this,
		   IARG_THREAD_ID,
		   IARG_FUNCARG_ENTRYPOINT_VALUE, epv,
		   IARG_END);
    RTN_Close(rtn);    
  }
}

#endif
