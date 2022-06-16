#ifdef USE_HWLOC
extern "C" {
  #include <hwloc.h>
}
#endif

#include <assert.h>
#include <cmath>
#include <iostream>
#include <fstream>
#include <algorithm>
#include "Permutation.hpp"

using namespace std;
using namespace Affinity;

Permutation::Permutation():_type(OTHER){}
  
Permutation::Permutation(const unsigned & n, const permutation_t &t): vector<index_t>(n, 0){
  switch(t){
  case COMPACT:
    for(unsigned i=0; i<n;i++){ operator[](i) = i; }
    break;
  case RANDOM:
    for(unsigned i=0; i<n;i++){ operator[](i) = i; }
    shuffle();
    break;
  case BALANCED:
    for(unsigned i=0; i<n;i++){
      unsigned nleaf = n, offset = 0, index = i;      
      while(nleaf%2==0){
	nleaf /= 2;
	offset = offset + index%2*nleaf;
	index /= 2;
      }
      operator[](offset+index) = i;
    }
    break;      
  default:      
    break;
  }
  _type = t;
}

Permutation::Permutation(const Permutation &copy): vector<index_t>(copy), _type(copy._type){}

void Permutation::print(const char* filename) const{
    ofstream output(filename); for(auto it = begin(); it!=end(); ++it){ output << *it << " "; } output << endl;
}

void Permutation::print() const{
  fprintf(stdout, "%30s: ","Threads permutation");
  for(auto it = begin(); it!=end(); ++it){ cout << *it << " "; } cout << endl;
}

#ifdef USE_HWLOC
void Permutation::print(const hwloc_topology_t& topology) const{
  Permutation os_perm(*this); os_perm.toOSIndex(topology);
  fprintf(stdout, "%30s: ","CPU affinity");
  for(auto it = os_perm.begin(); it!=os_perm.end(); ++it){ cout << *it << " "; } cout << endl;
}
#endif

void Permutation::shuffle(){
  random_shuffle(begin(), end());
}

Permutation Permutation::order() const{
  if(_type == COMPACT){return Permutation(*this);}
  
  // initialize original index locations
  Permutation ord(this->size(), COMPACT);
  // sort indexes based on comparing values in v
  sort(ord.begin(), ord.end(), [this](size_t i1, size_t i2) {return operator[](i1) < operator[](i2);});
  return ord;
}

#ifdef USE_HWLOC
void Permutation::toOSIndex(const hwloc_topology_t& topology){
  unsigned nPU = hwloc_get_nbobjs_by_type(topology, HWLOC_OBJ_PU);
  unsigned nCore = hwloc_get_nbobjs_by_type(topology, HWLOC_OBJ_CORE);

  Permutation ordered = order();
  
  if(size()<=nCore){
    for(unsigned i=0; i<size(); i++){
      hwloc_obj_t Core = hwloc_get_obj_by_type(topology, HWLOC_OBJ_CORE, ordered[i]);
      operator[](i) = Core->first_child->os_index;
    }
  }
  else if(size()<nPU){
    unsigned r = size()%nCore;
    for(unsigned i=0; i<2*r; i++){
      hwloc_obj_t PU = hwloc_get_obj_by_type(topology, HWLOC_OBJ_PU, ordered[i]);
      operator[](i) = PU->os_index;
    }
    for(unsigned i=r; i<size(); i++){
      hwloc_obj_t Core = hwloc_get_obj_by_type(topology, HWLOC_OBJ_CORE, ordered[i+r]);
      operator[](i+r) = Core->first_child->os_index;
    }
  }
  else {
    for(unsigned i=0; i<size(); i++){
      hwloc_obj_t PU = hwloc_get_obj_by_type(topology, HWLOC_OBJ_PU, ordered[i]%nPU);
      operator[](i) = PU->os_index;
    }
  }
}

#endif

