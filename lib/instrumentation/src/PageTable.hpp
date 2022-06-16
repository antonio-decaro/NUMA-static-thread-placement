#ifndef PAGETABLE_HPP
#define PAGETABLE_HPP

#include <stdio.h>
#include <string>
#include "Types.hpp"
#include "Lock.hpp"
#include "Constant.hpp"
#include "Stats.hpp"
#include "Matrix.hpp"

#define PSIZE              12
#define NPAGE              (1 << (MAXMEMSIZE-PSIZE))
#define PAGE_ID(addr)      (addr >> PSIZE)
#define CSIZE              6
#define CACHELINE_PER_PAGE (1<<(PSIZE-CSIZE))
#define CACHELINE_ID(addr) (( addr << (sizeof(addr)*8-PSIZE) ) >> (sizeof(addr)*8-CSIZE))

namespace Affinity{
  class Cacheline{
  private:
    addr_t               _addr;                 //Address identifier
    tid_t                _owner;                //tid of last writer
    state_t              _state;                //Data status. Incremented on write to set dirty.
    state_t              _states[MAX_THREADS];  //State for each thread    
    size_t               _read[MAX_THREADS];    //Size of read and write on this data
    size_t               _write[MAX_THREADS];   //Touches of read and write on this data
    
  public:
    Cacheline(const Cacheline& copy);    
    Cacheline(const addr_t& addr);    
    void  fprint(FILE * output) const;            
    tid_t write(const tid_t& tid, const unsigned& size);
    tid_t read(const tid_t& tid, const unsigned& size);

    void             touches(size_t* out) const;    
    unsigned long    reads()              const;
    unsigned long    writes()             const;
    unsigned         sharing_degree()     const;
    unsigned         writing_degree()     const;
    Matrix<long>     sharing_matrix()     const;
    double           write_ratio()        const;
    bool             is_shared()          const;
  };

  
  
  class Page{
  private:
    Lock         _lock;
    Cacheline**  _cacheline; 

    void allocate();            // On first touch, allocate cacheline array
    void allocate(const int&, const addr_t&);  // Allocate a single cache line
    
  public:
    Page();
    ~Page();    
    tid_t read(const addr_t& addr, const tid_t& tid, const unsigned& size);
    tid_t write(const addr_t& addr, const tid_t& tid, const unsigned& size);
    const Cacheline* cacheline(const int& id) const;
  };


  
  struct PageTableStats{
    Matrix<long>         sharing_matrix;
    OnlineStats<double>  sharing_degree;
    OnlineStats<double>  writing_degree;
    OnlineStats<double>  write_ratio;
    OnlineStats<double>  shared_write_ratio;
    Stats<unsigned long> footprint;

  public:
    PageTableStats();
  };

  class PageTable{
  private:
    Page _pages[NPAGE];

  public:
    PageTable();
    tid_t read(const addr_t& addr, const tid_t& tid, const unsigned& size);
    tid_t write(const addr_t& addr, const tid_t& tid, const unsigned& size);
    PageTableStats* stats() const;
    Matrix<long> sharing_matrix() const;
  };

};



#endif //PAGETABLE_HPP
