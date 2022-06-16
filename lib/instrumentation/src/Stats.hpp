#ifndef STATS_H
#define STATS_H

#include <stdio.h>
#include <list>
#include <string>

#define MAX_(a,b) ((a)>(b) ? (a):(b))
#define MIN_(a,b) ((a)<(b) ? (a):(b))

template<typename T>
class Stats{
private:
  double _min;
  double _max;
  double _sd;
  double _var;  
  double _mean;
  double _sum;
  
public:
  Stats();  
  Stats(const std::list<T>& samples);

  T sum() const;
  T min() const;
  T max() const;
  double mean() const;
  double var() const;
  double sd() const;  
};


template<typename T>
class OnlineStats{
private:
  double   _sum;
  double   _min;
  double   _max;
  double   _mean;
  double   _delta;
  double   _delta2;
  double   _M2;
  unsigned _n;
  
public:
  OnlineStats();
  OnlineStats(const OnlineStats<T>& copy);
  void insert(const T & value);
  unsigned count() const;
  T sum() const;
  T min() const;
  T max() const;
  double mean() const;
  double var() const;
  double sd() const;  
};


#endif
