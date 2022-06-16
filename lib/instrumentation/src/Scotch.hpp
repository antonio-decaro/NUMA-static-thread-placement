#ifndef ASCOTCH_H
#define ASCOTCH_H

//#define _cplusplus

#include "Matrix.hpp"
#include "Permutation.hpp"
#include <cstdio>
#include <scotch.h>
#ifdef USE_HWLOC
#include <hwloc.h>
#endif

namespace Affinity{
  class ScotchGraph{  
  private:
    typedef enum {AFFINITY, MATRIX_FILE} source_e;
  
    SCOTCH_Graph _graph;
    SCOTCH_Num   _vertnbr; //Number of vertex
    SCOTCH_Num*  _verttab; //Index of first edge in _edgetab
    SCOTCH_Num   _edgenbr; //Number of edges  
    SCOTCH_Num*  _edgetab; //Edges list
    SCOTCH_Num*  _edlotab; //Edges load
    source_e     _source; //Atrtibute used for free.

    template<typename T> 
    void format(Matrix<T>& mat);
    void printArch(const SCOTCH_Num * sizetab, const SCOTCH_Num * linktab, const unsigned& levelnbr) const;
    int  mapTleef(SCOTCH_Num * parttab,
		  const SCOTCH_Num * depthtab,
		  const SCOTCH_Num * aritytab,
		  const unsigned& n);
    Permutation partitionOrder(const SCOTCH_Num* parttab,
			       const unsigned&   nelem,
			       const unsigned*   partsize,
			       const unsigned&   npart);
  public:
    ScotchGraph();
    ScotchGraph(const char* filename);
    template<typename T> ScotchGraph(const Matrix<T> &affinity);
    ~ScotchGraph();

    Permutation partition(const unsigned& max_partitions);
#ifdef USE_HWLOC
    Permutation map(const hwloc_topology_t& topology);
#endif
  };
}

#endif
