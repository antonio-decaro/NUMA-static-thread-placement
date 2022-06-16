#ifndef MATRIX_H
#define MATRIX_H

#include "Types.hpp"
#include "Permutation.hpp"
#ifdef USE_HWLOC
#include <hwloc.h>
#endif
#include <vector>
#include <fstream>

namespace Affinity{
  template<typename T>
  class Matrix{
    
  private:
    unsigned _n;
    unsigned _nmax;    
    T *    _val;
  
  public:
    Matrix<T>();
    Matrix<T>(const unsigned &n);     //Allocator for n by n. Matrix is zeroed
    Matrix<T>(const char* filename);  //From file constructor
    Matrix<T>(const Matrix<T> &copy); //copy constructor
#ifdef USE_HWLOC  
    Matrix<T>(const hwloc_topology_t& topology); //matrix with hops between leaves
#endif  
    ~Matrix<T>();


    T&                   operator()(const unsigned &i, const unsigned &j);
    T                    operator()(const unsigned &i, const unsigned &j)const ;    
    Matrix<T>&           operator+=(const Matrix<T>& rhs);
    Matrix<T>&           operator*=(const Matrix<T>& rhs);
    void                 copy(const Matrix<T> &copy);
    Matrix<double>       toDouble() const;
    void                 set(const T& value);
    void                 resize(const size_t& n);
    void                 reserve(const size_t& n);    
    double               sum() const;
    void                 swap(const coord& a, const coord& b);
    void                 swap(const unsigned& a, const unsigned& b);
    void                 order(const std::vector<index_t>& index);
    void                 scale(const T& max_val); //values between 0 and max_val
    void                 rowdownscale();
    void                 coldownscale();
    void                 blankdiag();
    Matrix<T>            binary_hops();    
    std::pair<T,T>       limits()                                      const; //return <min,max>    
    unsigned             size()                                        const;
    void                 print(std::ofstream& output)                  const;
    void                 print(std::ostream& output)                   const;    
    void                 print(const char * filename)                  const;
    void                 print()                                       const;
  };

  template<typename T>
  class Affinity_Matrix : public Matrix<T> {
  private:
    double clusterSD(const Matrix<T>& mat, std::pair<unsigned, unsigned>& clusters) const;    
    double compute_split_frac(const int split_size) const;
    double compute_neighbour_com_frac(const double sum_val)         const;
    
  public:
    Affinity_Matrix(const Matrix<T>& input);
    Affinity_Matrix(const Matrix<T>& input, const std::vector<index_t>& permutation);

    double amount() const;    
    double hopbyte() const;
    double clusterSD() const;
    std::pair<unsigned, unsigned> nClusterSize() const; //<n_clusters, cluster_size>
    void compute_stat(double *CH, double *CB, double *CC, double *NBC) const;    
#ifdef USE_HWLOC
    double hopbyte(const hwloc_topology_t& topology) const;
    double clusterSD(const hwloc_topology_t& topology) const;
    std::pair<unsigned, unsigned> nClusterSize(const hwloc_topology_t& topology) const;
    void compute_stat(const hwloc_topology_t& topology, double *CH, double *CB, double *CC, double *NBC, double *SP) const;
#endif    
  };
};
#endif

