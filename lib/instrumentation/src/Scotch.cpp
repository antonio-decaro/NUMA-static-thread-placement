#include "Scotch.hpp"
#include <cstdio>
#include <iostream>
#include <vector>
#ifdef USE_HWLOC
extern "C" {
  #include <hwloc.h>
}
#endif

using namespace std;
using namespace Affinity;

ScotchGraph::ScotchGraph(){
  _vertnbr = 0;
  _edgenbr = 0;  
  _verttab = NULL;
  _edgetab = NULL;
  _edlotab = NULL;
  SCOTCH_graphInit(&_graph);
}

template<typename T> 
ScotchGraph::ScotchGraph(const Matrix<T>& affinity){
  Matrix<T> mat(affinity);
  format(mat);
  _vertnbr = mat.size();      
  _verttab = new SCOTCH_Num[_vertnbr+1];
  _edgetab = new SCOTCH_Num[mat.size()*mat.size()];
  _edlotab = new SCOTCH_Num[mat.size()*mat.size()];

  //Fill _verttab and set _edgenbr
  _edgenbr = 0;
  _verttab[0] = 0;
  for(unsigned i=0; i<mat.size(); i++){
    for(unsigned j=0; j<mat.size(); j++){
      if(mat(i,j) != 0){
	_edgetab[_edgenbr] = j;
	_edlotab[_edgenbr] = mat(i,j);	
	_edgenbr++;
      }
    }
    _verttab[i+1] = _edgenbr;
  }
  
  SCOTCH_graphInit(&_graph);
  int err = SCOTCH_graphBuild(&_graph, 0, _vertnbr, _verttab, NULL, NULL, NULL, _edgenbr, _edgetab, _edlotab);
  if(err == 1){
    fprintf(stderr, "Graph build failed.\n");
  }
  _source = AFFINITY;
}

template ScotchGraph::ScotchGraph(const Matrix<int>& affinity);
template ScotchGraph::ScotchGraph(const Matrix<unsigned>& affinity);
template ScotchGraph::ScotchGraph(const Matrix<long>& affinity);
template ScotchGraph::ScotchGraph(const Matrix<unsigned long>& affinity);
template ScotchGraph::ScotchGraph(const Matrix<long long>& affinity);
template ScotchGraph::ScotchGraph(const Matrix<unsigned long long>& affinity);
template ScotchGraph::ScotchGraph(const Matrix<float>& affinity);
template ScotchGraph::ScotchGraph(const Matrix<double>& affinity);


ScotchGraph::ScotchGraph(const char * filename): _verttab(NULL), _edgetab(NULL), _edlotab(NULL), _vertnbr(0), _edgenbr(0)  
{
  FILE * stream = fopen(filename, "r");
  _source = MATRIX_FILE;      
  if(stream == NULL){
    perror("fopen");
    fprintf(stderr, "Graph instanciation failed at file opening.\n");
    return;
  }

  //Build graph with baseval 0 (C-style) and remove vertex load if any.
  SCOTCH_graphInit(&_graph);
  int err = SCOTCH_graphLoad(&_graph, stream, 0, 1);
  if(err == 1){
    fprintf(stderr, "Graph Load from file stream failed.\n");
  } else {
    SCOTCH_graphData(&_graph, NULL, &_vertnbr, &_verttab, NULL, NULL, NULL, &_edgenbr, &_edgetab, &_edlotab);  
  }
  fclose(stream);
}

template<typename T> 
void ScotchGraph::format(Matrix<T>& mat){
  mat.coldownscale();  
  mat.rowdownscale();    
  mat.blankdiag();      
  mat.scale(100);
  //mat.print();
}

template void ScotchGraph::format(Matrix<int>& mat);
template void ScotchGraph::format(Matrix<unsigned>& mat);
template void ScotchGraph::format(Matrix<long>& mat);
template void ScotchGraph::format(Matrix<unsigned long>& mat);
template void ScotchGraph::format(Matrix<long long>& mat);
template void ScotchGraph::format(Matrix<unsigned long long>& mat);
template void ScotchGraph::format(Matrix<float>& mat);
template void ScotchGraph::format(Matrix<double>& mat);

ScotchGraph::~ScotchGraph(){
  if(_source == AFFINITY){ delete[] _verttab; }
  if(_source == AFFINITY){ delete[] _edgetab; }
  if(_source == AFFINITY){ delete[] _edlotab; }
  SCOTCH_graphExit(&_graph);
}

void ScotchGraph::printArch(const SCOTCH_Num * sizetab, const SCOTCH_Num * linktab, const unsigned& levelnbr) const{
  printf("%-16s: ", "distances");
  for(unsigned i=0; i<levelnbr; i++){ printf("%6d ", sizetab[i]); }
  printf("\n");
  printf("%-16s: ", "arity");
  for(unsigned i=0; i<levelnbr; i++){ printf("%6d ", linktab[i]); }
  printf("\n");  
}

int ScotchGraph::mapTleef(SCOTCH_Num * parttab,
			  const SCOTCH_Num * depthtab,
			  const SCOTCH_Num * aritytab,
			  const unsigned& n){
  int ret = 0, err;
  SCOTCH_Arch arch;
  SCOTCH_archInit(&arch);  
  err = SCOTCH_archTleaf(&arch, n, aritytab, depthtab);
  if(err == 1){
    fprintf(stderr, "tleaf arch build failed.\n");
    ret = -1;
    goto exit_with_arch;
  }
    
  SCOTCH_Strat strat;
  SCOTCH_stratInit(&strat);
  err = SCOTCH_stratGraphMapBuild(&strat, SCOTCH_STRATBALANCE|SCOTCH_STRATQUALITY, _vertnbr, 0.01);    
  if(err == 1){
    fprintf(stderr, "Strategy build failed.\n");
    ret = -1;
    goto exit_with_strat;
  }
    
  err = SCOTCH_graphMap(&_graph, &arch, &strat, parttab);
  if(err == 1){
    fprintf(stderr, "Graph partitioning failed.\n");
    ret = -1;
  }

exit_with_strat:  
  SCOTCH_stratExit(&strat);
exit_with_arch:    
  SCOTCH_archExit(&arch);  
  return ret;
}

Permutation ScotchGraph::partitionOrder(const SCOTCH_Num* parttab,
					const unsigned&   nelem,
					const unsigned*   partsize,
					const unsigned&   npart)
{
  Permutation perm(nelem, Permutation::COMPACT);
  SCOTCH_Num * partindex = new SCOTCH_Num[npart];  //Partitions index [0, partsize]
  SCOTCH_Num * startindex = new SCOTCH_Num[npart]; //Partitions index start (partsize[i-1])
  partindex[0] = 0; startindex[0] = 0;
  for(unsigned i=1;i<npart;i++){
    partindex[i] = 0;
    startindex[i] = partsize[i-1] + startindex[i-1];
  }
  
  for(unsigned i=0;i<nelem;i++){
    SCOTCH_Num  p   = parttab[i];                //The vertex partition 
    SCOTCH_Num* index = &(partindex[p]);           //The current vertex index in partition - startindex[p].    
    perm[i]         = startindex[p] + *index;    //Update final permutation.
    *index          = (*index+1) % partsize[p];  //update index in partition.
  }

  delete partindex;
  delete startindex;  
  return perm;
}

Permutation ScotchGraph::partition(const unsigned& max_partitions){
  SCOTCH_Num   partnbr = 1;                          //number of partition
  unsigned     parts = _vertnbr;                     //number of vertex per partition
  SCOTCH_Num * parttab = new SCOTCH_Num[_vertnbr];   //vertex partition index table.
  
  //Build architecture
  unsigned levelnbr = 0;                             //depth of tree arch  
  while( parts>0 && parts%2==0 && (partnbr*2)<max_partitions){
    partnbr *= 2;
    levelnbr++;
    parts /= 2;
  }
  
  SCOTCH_Num * sizetab  = new SCOTCH_Num[levelnbr](); //levels arity
  SCOTCH_Num * linktab  = new SCOTCH_Num[levelnbr](); //distance to leaf in tree ar
  unsigned *   partsize = new unsigned[partnbr];
 
  for(unsigned i=0; i<levelnbr; i++){
    linktab[i] = levelnbr-i;
    sizetab[i] = 2;
  }
  for(unsigned i=0; i<partnbr; i++){
      partsize[i] = parts;
  }
  
  //linktab[levelnbr-1] = 1;
  //sizetab[levelnbr-1] = parts;
  //printArch(linktab, sizetab, levelnbr);

  if(mapTleef(parttab, linktab, sizetab, levelnbr)){ goto exit_partitionning; }
  
exit_partitionning:
  Permutation perm = partitionOrder(parttab, _vertnbr, partsize, partnbr);      
  delete[] sizetab;
  delete[] linktab;
  delete[] parttab;
  delete[] partsize;
  return perm;
}

#ifdef USE_HWLOC
Permutation ScotchGraph::map(const hwloc_topology_t& topology){
  int err;  

  //Find last shared level.
  hwloc_obj_t partlevel = hwloc_get_obj_by_type(topology, HWLOC_OBJ_CORE, 0);
  do{
    partlevel = partlevel->parent;
  } while(partlevel->parent && partlevel->arity == 1);
  unsigned     partnbr  = hwloc_get_nbobjs_by_depth(topology, partlevel->depth); //number of levels;
  SCOTCH_Num * aritytab = new SCOTCH_Num[partlevel->depth]();                 //Node arity at level i
  SCOTCH_Num * disttab  = new SCOTCH_Num[partlevel->depth]();                 //Node distance to leaf at level i
  SCOTCH_Num * parttab  = new SCOTCH_Num[_vertnbr]();                     //Shared level index where vertex belongs
  unsigned *   partsize = new unsigned[partnbr];

  //Init architecture for partitionning
  for(unsigned i=0; i<partlevel->depth; i++){
    //Build only with first node at depth i. If the nodes have a different arity, then we are doomed.
    hwloc_obj_t curlevel = hwloc_get_obj_by_depth(topology, i, 0);
    unsigned    dist     = partlevel->depth-i; //dist = dist*dist; //Quadratic distance
    disttab[i] = dist;
    aritytab[i] = curlevel->arity;    
  }
  //printArch(disttab,aritytab,partlevel->depth);

  if(mapTleef(parttab, disttab, aritytab, partlevel->depth) == -1) { goto exit_mapping; }
  for(unsigned i=0; i<partnbr; i++){ partsize[i] = partlevel->arity; }  

exit_mapping:
  Permutation perm = partitionOrder(parttab, _vertnbr, partsize, partnbr);  
  delete[] aritytab;
  delete[] disttab;
  delete[] parttab;
  delete[] partsize;
  return perm;
}
#endif

