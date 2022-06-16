#ifndef TYPES_HPP
#define TYPES_HPP

#include <stdint.h>

namespace Affinity{
  
  typedef unsigned             index_t;
  typedef uint32_t             addr_t;
  typedef unsigned             tid_t;
  typedef unsigned long        state_t ;
  
  typedef struct coord_s {
    unsigned row;
    unsigned col;
    coord_s(unsigned r, unsigned c): row(r), col(c){}
    coord_s(): row(0), col(0){}      
  } coord;
}

#endif
