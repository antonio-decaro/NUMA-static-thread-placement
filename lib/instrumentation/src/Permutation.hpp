#ifndef PERMUTATION_H
#define PERMUTATION_H

#ifdef USE_HWLOC
#include <hwloc.h>
#endif
#include <vector>
#include "Types.hpp"

namespace Affinity{
  class Permutation: public std::vector<index_t>{
    
  public:
    typedef enum {COMPACT, BALANCED, RANDOM, OTHER} permutation_t;  
  private:
    permutation_t  _type;
  
  public:
    Permutation();
    Permutation(const unsigned & n, const permutation_t &t);  
    Permutation(const Permutation &copy);

    void shuffle();
    Permutation order() const;
    
#ifdef USE_HWLOC
    void toOSIndex(const hwloc_topology_t& topology);
    void print(const hwloc_topology_t& topology) const;    
#endif
    void print() const;    
    void print(const char* filename) const;
  };
}

#endif

