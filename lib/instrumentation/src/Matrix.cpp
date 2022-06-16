#ifdef USE_HWLOC
extern "C" {
  #include <hwloc.h>
}
#endif

#include "Matrix.hpp"
#include "Stats.hpp"
#include <fstream>
#include <iostream>
#include <cstdlib>
#include <string>
#include <limits>

using namespace std;
using namespace Affinity;

#define val(i,j) _val[i+j*_nmax]
#define def_min 64

template<typename T>
Matrix<T>::Matrix(const unsigned &n): _n(0), _nmax(0), _val(NULL){
  if(n==0){return;} reserve(MAX_(n,def_min)); _n = n;
}

template<typename T>
Matrix<T>::Matrix(const Matrix &copy): _n(0), _nmax(0), _val(NULL){
  if(copy._n==0){return;}
  reserve(copy._nmax);
  _n = copy._n;
  for(unsigned i=0; i<_n; i++){
    for(unsigned j=0; j<_n; j++){
      val(i,j) = copy._val[i+j*_nmax];
    }
  }
}

template<typename T>
void Matrix<T>::copy(const Matrix<T> &copy){
  if(copy._n != _n){return;}
  for(unsigned i=0; i<_n*_n; i++){
    _val[i] = copy._val[i];
  }
}

template<typename T>
Matrix<double> Matrix<T>::toDouble() const{
  Matrix<double> ret = Matrix<double>(_n);
  for(unsigned i=0; i<_n; i++){
    for(unsigned j=0; j<_n; j++){
      ret(i,j) = operator()(i,j);
    }
  }
  return ret;
}

template<typename T> Matrix<T>::Matrix(): _n(0), _nmax(0), _val(NULL){ reserve(def_min); }


#ifdef USE_HWLOC
template<typename T>
Matrix<T>::Matrix(const hwloc_topology_t& topology): _n(0), _nmax(0), _val(NULL){  
  unsigned nleaf = hwloc_get_nbobjs_by_type(topology, HWLOC_OBJ_CORE);

  //Matrix initialization
  if(nleaf==0){return;}
  resize(nleaf);

  //Fill with hops
  for(unsigned i=0; i<nleaf; i++){
    for(unsigned j=0; j<nleaf; j++){
      unsigned nhops = 0;
      hwloc_obj_t obj_i = hwloc_get_obj_by_type(topology, HWLOC_OBJ_CORE, i);
      hwloc_obj_t obj_j = hwloc_get_obj_by_type(topology, HWLOC_OBJ_CORE, j);
      
      T&hops_0 = val(obj_i->logical_index, obj_j->logical_index);
      T&hops_1 = val(obj_j->logical_index, obj_i->logical_index);
      
      while(obj_i != obj_j){
	nhops +=2;
	obj_i = obj_i->parent;
	obj_j = obj_j->parent;	
      }
      hops_0 = hops_1 = nhops;
    }
  }
}
#endif

template<typename T> Matrix<T>::~Matrix<T>(){ free(_val); }

template<typename T>
void Matrix<T>::reserve(const size_t& n){
  if(n < _nmax){ return; }
  T * new_val = (T*) malloc(n * n * sizeof *new_val);
  if(new_val == NULL){
    perror("malloc");
    return;
  }

  for(unsigned i = 0; i< n*n; i++){ new_val[i] = 0; }
  if(_val != NULL){
    for(unsigned i = 0; i<_n; i++){ for(unsigned j = 0; j<_n; j++){ new_val[i + j*n] = val(i, j); } } free(_val); 
  }
  _val = new_val; _nmax = n;
}

template<typename T>
void Matrix<T>::resize(const size_t& n){
  if(n < _n){ return; }
  if(n > _nmax){ reserve(2*n); }
  _n = n;
}

template<typename T>
void Matrix<T>::set(const T& value){
  for(unsigned i=0; i<_n*_n; i++){
    _val[i] = value;
  }
}

template<typename T>
T& Matrix<T>::operator()(const unsigned &i, const unsigned &j){
  size_t size = MAX_(i+1,j+1); if(size>_n){resize(size);} //Check bounds
  return val(i, j);  
}

template<typename T>
T Matrix<T>::operator()(const unsigned &i, const unsigned &j) const{
  size_t size = MAX_(i+1,j+1); //Check bounds
  if(size>_nmax){
    return val(MIN_(i,_nmax-1), MIN_(j,_nmax-1));
  }
  return val(i, j);  
}

template<typename T>
Matrix<T>& Matrix<T>::operator+=(const Matrix<T>& rhs){
  size_t n = MIN_(rhs.size(), size());
  for(unsigned i=0; i<n; i++){
    for(unsigned j=0; j<n; j++){      
      val(i, j) += rhs._val[i + j*rhs._nmax];
    }
  }
  return *this;
}

template<typename T>
Matrix<T>& Matrix<T>::operator*=(const Matrix<T>& rhs){
  size_t n = MIN_(rhs.size(), size());
  for(unsigned i=0; i<n; i++){
    for(unsigned j=0; j<n; j++){
      val(i, j) *= rhs._val[i + j*rhs._nmax];
    }
  }
  return *this;
}

template<typename T>
unsigned Matrix<T>::size() const{ return _n; }

template<typename T>
void Matrix<T>:: scale(const T &max_val){
  pair<T,T> minmax = limits();
  T scale = minmax.second-minmax.first;
  for(unsigned i=0; i<_n; i++){
    for(unsigned j=0; j<_n; j++){
      val(i, j) = (val(i, j) - minmax.first)*max_val / scale;
    }
  }  
}


// template<typename T, typename V> 
// Matrix<T>::operator Matrix<V>(){
//   Matrix<V> ret = Matrix<V>(_n);
//   for(unsigned i = 0; i<_n*_n; i++){
//     ret._val[i] = _val[i];
//   }
//   return ret;
// }

template<typename T>
void Matrix<T>:: blankdiag(){
  for(unsigned j=0; j<_n; j++){
    val(j, j) = 0;
  }
}

template<typename T>
void Matrix<T>:: coldownscale(){
  //Find min
  for(unsigned i=0; i<_n; i++){
    T min = numeric_limits<T>::max();    
    for(unsigned j=0; j<_n; j++){
      if(val(i, j) < min) min = val(i, j);
    }
    //substract min
    for(unsigned j=0; j<_n; j++){
      val(i, j) -= min;
    }        
  }
}

template<typename T>
void Matrix<T>::rowdownscale(){
  //Find min
  for(unsigned j=0; j<_n; j++){
    T min = numeric_limits<T>::max();
    for(unsigned i=0; i<_n; i++){ if(val(i, j) < min) min = val(i, j); }    
    for(unsigned i=0; i<_n; i++){ val(i, j) -= min; } //substract min
  }
}

template<typename T>
pair<T,T> Matrix<T>::limits() const{
  T max= _val[0];
  T min= _val[0];
  for(unsigned i=0; i<_n; i++){
    for(unsigned j=0; j<_n; j++){
      if(val(i, j) > max) max = val(i, j);
      if(val(i, j) < min) min = val(i, j);
    }
  }
  return pair<T,T>(min,max);
}

template<typename T>
void Matrix<T>::print(ostream& output) const{
  output << "0" << endl << _n << " " << _n*_n << endl << "0 010" << endl;
  for(unsigned i = 0; i<_n; i++){
    output << _n;
    for(unsigned j = 0; j<_n; j++) output << " " << val(i, j) << " " << j;
    output << endl;
  }
}

template<typename T>
void Matrix<T>::print(ofstream& output) const{
  output << "0" << endl << _n << " " << _n*_n << endl << "0 010" << endl;
  for(unsigned i = 0; i<_n; i++){
    output << _n;
    for(unsigned j = 0; j<_n; j++) output << " " << val(i, j) << " " << j;
    output << endl;
  }
}

template<typename T>
void Matrix<T>::print(const char * filename) const{
  ofstream output(filename);
  if(! output.good() || output.bad() || output.fail()){
    perror("ofstream");
  }
  print(output);
  std::cout<<"dumped affinity to "<<filename<<endl;  
}

template<typename T>
void Matrix<T>::print() const{
  print((ofstream&)std::cout);
}

template<typename T>
void Matrix<T>::swap(const coord& a, const coord& b){  
  T tmp = val(a.row, a.col);
  val(a.row, a.col) = val(b.row, b.col);
  val(b.row, b.col) = tmp;
}

template<typename T>
void Matrix<T>::swap(const unsigned& i, const unsigned& j){
  for(unsigned n=0; n<_n; n++){    
    swap(coord(i,n), coord(j,n)); //swap columns
    swap(coord(n,i), coord(n,j)); //swap lines
  }
}

template<typename T>
void Matrix<T>::order(const vector<index_t>& index){
  Matrix<T> tmp(*this);  
  for(unsigned i=0; i<_n; i++){
    for(unsigned j=0; j<_n; j++){
      val(i,j) = tmp(index[i],index[j]);
    }
  }
}

template<typename T>
double Matrix<T>::sum() const{
  vector<double> s(_n,0);
  
  for(unsigned i=0; i<_n; i++){
    for(unsigned j=0; j<_n; j++){
      s[i] += static_cast<double>(val(i, j));
    }
  }

  double ret = 0;
  for(unsigned i=0; i<_n; i++){
    ret += s[i];
  }  
  return ret;
}

template<typename T>
Matrix<T>::Matrix(const char * filename): _n(0), _nmax(0), _val(NULL){
  string line;
  ifstream input(filename);
  
  //skip first line
  getline(input, line);
  if(input.eof() || input.fail() || input.bad()){ return; }

  //Line with number of vertex and edges
  getline(input, line);
  if(input.eof() || input.fail() || input.bad()){ return; }
  string::size_type pos = line.find(' ');
  unsigned n = strtoul(line.substr(0,pos).c_str(),NULL,10);
  
  //Initialize matrix
  resize(n);

  //Skip line with baseval, assume it is 0
  getline(input, line);
  if(input.eof() || input.fail() || input.bad()){ return; }
  
  unsigned j, nval;
  T val;
  std::string::size_type begin = 0, end = 0;
  
  for(unsigned i=0; i<n;i++){
    getline(input, line);
    if(input.eof() || input.fail() || input.bad()){ return; }
    begin = line.find(' ');
    nval = strtoul(line.substr(0,begin).c_str(), NULL, 10);
    for(unsigned k=0;k<nval;k++){
      end = line.find(' ', begin+1);
      val = strtoul(line.substr(begin, end).c_str(), NULL, 10);
      begin = line.find(' ', end+1);
      j = strtoul(line.substr(end,  begin).c_str(), NULL, 10);
      val(i,j) = val;      
    }
  }
}

template<typename T>
Matrix<T> Matrix<T>::binary_hops(){
  Matrix<T> hops = Matrix<T>(*this);
  unsigned n = _n;
  //One hop 
  for(unsigned i=0; i<_n; i++){
    for(unsigned j=0; j<i; j++){
      hops(i, j)=2;
    }
    for(unsigned j=i+1; j<_n; j++){
      hops(i, j)=2;
    }          
  }
  while(n%2 == 0 && n>1){
    n = n/2;
    for(unsigned i=0; i<_n; i++){
      for(unsigned p=0; p<_n; p+=n){
	if(p == i-(i%n)){continue;}
	for(unsigned j=p; j<p+n; j++){
	  hops(i, j)++;
	  hops(j, i)++;
	}
      }
    }
  }
  return hops;
}

template class Matrix<double>;
template class Matrix<float>;
template class Matrix<int>;
template class Matrix<unsigned>;
template class Matrix<long>;
template class Matrix<unsigned long>;
template class Matrix<long long>;
template class Matrix<unsigned long long>;

template<typename T>
Affinity_Matrix<T>::Affinity_Matrix(const Matrix<T>& input, const std::vector<index_t>& permutation): Matrix<T>(input){
  Matrix<T>::order(permutation);
}

template<typename T>
Affinity_Matrix<T>::Affinity_Matrix(const Matrix<T>& input): Matrix<T>(input){}

template<typename T>
double Affinity_Matrix<T>::hopbyte() const{
  Matrix<T> bytes = Matrix<T>(*this);
  Matrix<T> hops = bytes.binary_hops();
  hops *= bytes;
  return hops.sum();
}

#ifdef USE_HWLOC
template<typename T>
double Affinity_Matrix<T>::hopbyte(const hwloc_topology_t& topology) const{
  Matrix<T> hops  = Matrix<T>(topology);
  hops *= *this;
  return hops.sum();
}
#endif

template<typename T>
double Affinity_Matrix<T>::amount() const{
  Matrix<T> copy(*this);  
  return copy.sum()/(copy.size()*copy.size());
}


template<typename T>
pair<unsigned, unsigned> Affinity_Matrix<T>::nClusterSize() const{  
  unsigned nclusters = 1;
  unsigned cluster_size = Matrix<T>::size();
  while(cluster_size%2 == 0 && cluster_size > 4){
    cluster_size/=2;
    nclusters *= 2;
  }
  return make_pair(nclusters, cluster_size);
}

#ifdef USE_HWLOC
template<typename T>
pair<unsigned, unsigned> Affinity_Matrix<T>::nClusterSize(const hwloc_topology_t& topology) const{
  hwloc_obj_t Node = hwloc_get_obj_by_type(topology, HWLOC_OBJ_NODE, 0);  
  unsigned nclusters = hwloc_get_nbobjs_by_type(topology, HWLOC_OBJ_NODE);;
  unsigned cluster_size = hwloc_get_nbobjs_inside_cpuset_by_type(topology, Node->cpuset, HWLOC_OBJ_CORE);
  return make_pair(nclusters, cluster_size);
}
#endif

template<typename T>
double Affinity_Matrix<T>::clusterSD(const Matrix<T>& mat, pair<unsigned, unsigned>& clusters) const{
  //compute the total affinity inside clusters
  list<T> caffinity;
  for(unsigned c = 0; c<clusters.first; c++){
    unsigned cbegin = c*(clusters.second);
    unsigned cend   = (c+1)*(clusters.second);
    T aff = 0;
    for(unsigned i = 0; i<mat.size(); i++){
      for(unsigned j = cbegin; j<cend; j++){
	aff += mat(i,j);
	aff += mat(j,i);
      }
    }
    caffinity.push_back(aff);
  }
  
  Stats<T> stats(caffinity);
  return stats.sd();
}

template<typename T>
double Affinity_Matrix<T>::clusterSD() const{
  pair<unsigned, unsigned> clusters = Affinity_Matrix<T>::nClusterSize();
  Matrix<T> mat = Matrix<T>(*this);
  return Affinity_Matrix<T>::clusterSD(mat, clusters);
}

#ifdef USE_HWLOC
template<typename T>
double Affinity_Matrix<T>::clusterSD(const hwloc_topology_t& topology) const{
  pair<unsigned, unsigned> clusters = Affinity_Matrix<T>::nClusterSize(topology);
  return Affinity_Matrix<T>::clusterSD(*this, clusters);
}
#endif

template<typename T>
double Affinity_Matrix<T>::compute_neighbour_com_frac(const double sum_val) const{
  int    i,j1,j2,n    = this->size();
  double neighbour_sum_com = 0.0;
  for(i=0;i<n;i++){
    j1=i-1;
    j2=i+1;
    if(j1>0) neighbour_sum_com+=this->operator()(i,j1);
    if(j2<n-1) neighbour_sum_com+=this->operator()(i,j2);
  }

  /* if neighbour_sum_com == sum_val then return 0. hence the higher the more oportunlity for mapping*/
  return 1-neighbour_sum_com/sum_val; 
}


template<typename T>
double Affinity_Matrix<T>::compute_split_frac(const int split_size) const{
  double sum_inside = 0, sum_val=0;
  int i,j,s,k,l,n   = this->size();

  if(split_size >= n){ return 0; }

  for(unsigned i=0;i<this->size();i++){
    for(unsigned j=0;j<this->size();j++){
      sum_val += this->operator()(i,j);
    }
  }
    

  /* compute the amount of communication in block of size split_size x split_size around the diagonal
     useful to count the communications inside a node of split_size cores*/

  for (s = 0 ; s < n ; s += split_size){
    for (l = 0 ; l < split_size ; l++){
      for(k = 0 ; k < split_size ; k++){
	i=s+l;
	j=s+k;
	if( (i<n) && (j<n))
	  sum_inside += this->operator()(i,j);
      }
    }
  }
  /* if sum_inside == sum_val then SP = 0 hence the higher the more oportunlity for mapping*/
  return 1-sum_inside/sum_val; 
}

/* Matthias Deiner Matrix statistic */
/*  CH (communication heterogeneity):
    Characterizing communication and page usage of parallel applications for thread and data mapping
    Matthias Diener a,∗, Eduardo H.M. Cruz a, Laércio L. Pilla c, Fabrice Dupros b, Philippe O.A. Navaux a */
/* CB communication balance :
   Locality and Balance for Communication-Aware Thread Mapping in Multicore Systems
   M Diener, EHM Cruz, MAZ Alves, MS Alhakeem… - Euro-Par 2015: Parallel …, 2015 - Springer
*/
/* CC average communiction centrality:
   Propotion of the considered line taken from teh center that contain half of teh communication
   S_i = sum of the communication.
   j1 and j2 are such  that sum_{j=j1}^j2 mat[i][j]<S_i/2 and n/2-j1=j2-n/2
   we compute R_i = (j2-j1)/n
   CC = avg(Ri)
*/

template<typename T>
void Affinity_Matrix<T>::compute_stat(double *CH, double *CB, double *CC, double *NBC) const{
  double var, sum_var = 0, sum_sq = 0;
  double sum_val = 0, avg_val = 0;
  int i,j, j1, j2,n = this->size();
  double sum_line = 0, avg_val_line = 0, max = 0;
  double sum = 0;
  double r=0;
  
  for(i=0;i<n;i++){
    sum_line=0;
    sum_sq = 0;
    for(j=0;j<n;j++){
      if(i!=j){
	sum_line += this->operator()(i,j);
	sum_sq   += this->operator()(i,j)*this->operator()(i,j);
      }
	if(this->operator()(i,j)>max)
	  max = this->operator()(i,j);
    }
    sum_val += sum_line;
    avg_val_line = sum_line/(n-1);
    var = sum_sq/(n-1)-(avg_val_line*avg_val_line); /* variance of the line by theorem König-Huygens*/
    /* printf("%d: %f (%f, %f)\n",i,var, sum_sq, avg_val_line);  */
    sum_var += var; /* sul teh varaiance of the line to compute its avraga for CH */

    j1=j2=i;
    sum=0;
    while(sum<sum_line/2){
      if(j1>0) j1--;
      if(j2<n-1) j2++;
      sum += this->operator()(i,j1)+this->operator()(i,j2);
    }
    r+= (j2-j1)/((double)n);
  }

  avg_val = sum_val/(n*n-n);
  // printf("%f %f\n",avg_val, max); 

  *CH = sum_var/n; /* avg variance of the lines:  the higher the more opportunity for mapping*/
  *CB = 1 - avg_val/max; /*  CB = 0 <=>  max == avg : the higher the more opportunity for mapping*/
  *CC = r/n;   /* CC=0 <=> all communications on the diagonal: the higher the more opportunity for mapping*/

  *NBC = compute_neighbour_com_frac(sum_val);
}

#ifdef USE_HWLOC
template<typename T>
void Affinity_Matrix<T>::compute_stat(const hwloc_topology_t& topology, double *CH, double *CB, double *CC, double *NBC, double *SP) const{
  hwloc_obj_t NUMANODE = hwloc_get_obj_by_type(topology, HWLOC_OBJ_NUMANODE,0);
  unsigned NB_CORE_PER_NODE = hwloc_get_nbobjs_inside_cpuset_by_type(topology, NUMANODE->parent->cpuset, HWLOC_OBJ_CORE);
  *SP = compute_split_frac(NB_CORE_PER_NODE);  
}
#endif
  
template class Affinity_Matrix<double>;
template class Affinity_Matrix<float>;
template class Affinity_Matrix<int>;
template class Affinity_Matrix<unsigned>;
template class Affinity_Matrix<long>;
template class Affinity_Matrix<unsigned long>;
template class Affinity_Matrix<long long>;
template class Affinity_Matrix<unsigned long long>;

