#ifndef ALLOCATION_HPP
#define ALLOCATION_HPP

#include <string>
#include <vector>
#include <map>
#include "Stats.hpp"
#include "Lock.hpp"

class AllocStats{
private:
  /** Addresses of current allocations **/
  std::map<unsigned long, unsigned long long> _alloc_table;
  /** Current resident set size, i.e currently allocated memory space, per thread **/
  std::vector<unsigned long long> _rss;
  /** The total resident set size **/
  unsigned long long _rss_tot;
  /** rss stats accross time, per thread **/
  std::vector<OnlineStats<unsigned long long>> _local_rss_stats;
  /** rss stats accross time, global **/
  OnlineStats<unsigned long long> _global_rss_stats;
  /** For concurrent access to _rss and _global_rss_stats **/
  Lock _lock;
  /** Hold a vector of last sizes allocated because instrumentation is made in two steps: entry and exit. So information about
   allocation requires to remember the allocated size before knowing the return address. **/
  std::vector<unsigned long long> _last_sizes;
  /** Return the position of the size argument if function is an allocation function, else the position of address argument if
      the function is a free function **/
  static int  entryPointValue(const std::string& name);
  /** Save the size parameter on call to alloc **/
  void set_alloc(const unsigned& tid, const unsigned long long& size);
  /** save allocation addr and size (set with set_alloc) **/
  void count_alloc(const unsigned &tid, unsigned long addr);
  /** save free **/
  void count_free(const unsigned &tid, unsigned long addr);
  
#ifdef PIN_H
  static VOID allocBefore(AllocStats*, THREADID, ADDRINT size);
  static VOID allocAfter(AllocStats*, THREADID, ADDRINT addr);
  static VOID freeBefore(AllocStats*, THREADID, ADDRINT addr);
#endif
  
public:
  static bool enabled;
  
  AllocStats();
  AllocStats(const unsigned& nthreads);
  ~AllocStats();

  unsigned                        num_threads()             const;
  OnlineStats<unsigned long long> rss()                     const;
  OnlineStats<unsigned long long> rss(const unsigned& tid)  const;
  
  /** Return true if a function name is identified as an allocation function **/
  static bool isAlloc(const std::string& name);
  /** Return true if a function name is identified as a free function, i.e it deallocates memory. **/
  static bool isFree(const std::string& name);
#ifdef PIN_H
  /** Function to call to instrument an allocation function. Check with isAlloc is performed. 
      If test fails the function is not instrumented.**/
  VOID InstrumentAlloc(const RTN& rtn);
  /** Function to call to instrument a free function. Check with isFree is performed. 
      If test fails the function is not instrumented.**/  
  VOID InstrumentFree(const RTN& rtn);  
#endif
};

#endif
