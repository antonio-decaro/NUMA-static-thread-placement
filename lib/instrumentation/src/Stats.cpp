#include "Stats.hpp"
#include <cmath>

using namespace std;

template<typename T>
Stats<T>::Stats(): _min(0), _max(0), _sd(0), _var(0), _mean(0){};

template<typename T>
Stats<T>::Stats(const list<T>& samples){
  _min  = (double)samples.front();
  _max  = (double)samples.front();
  _mean = 0;
  _sd   = 0;
  _var  = 0;
  _sum  = 0;
  
  //online_kurtosis: https://en.wikipedia.org/wiki/Algorithms_for_calculating_variance
  double M2 = 0;
  double delta = 0;
  double delta2 = 0;
  double x = 0;
  size_t i = 0;
  
  if(samples.size() < 2){
    _var = 0;
    _sd = 0;
    _mean = _min;
    _sum = samples.front();
  } else {  
    for(auto e = samples.begin(); e!=samples.end(); e++){
      i++;
      _sum += (double)(*e);
      x = (double)(*e);
      _min = MIN_(x, _min);
      _max = MAX_(x, _max);        
      delta = x - _mean;
      _mean += delta/(i);
      delta2 = x - _mean;
      M2 += delta*delta2;
    }
    _var = M2 / (samples.size() - 2);
    _sd   = sqrt(_var);    
  }
}

template<typename T>
T Stats<T>::min() const{ return _min; }

template<typename T>
T Stats<T>::max() const{ return _max; }

template<typename T>
double Stats<T>::mean() const{ return _mean; }

template<typename T>
double Stats<T>::var() const{ return _var; }

template<typename T>
double Stats<T>::sd() const{ return _sd; }

template<typename T>
T Stats<T>::sum() const{ return _sum; }

template class Stats<double>;
template class Stats<float>;
template class Stats<int>;
template class Stats<unsigned>;
template class Stats<long>;
template class Stats<unsigned long>;
template class Stats<long long>;
template class Stats<unsigned long long>;

template<typename T>
OnlineStats<T>::OnlineStats(): _min(0), _max(0), _mean(0), _delta(0), _delta2(0), _M2(0), _n(0){}

template<typename T>
OnlineStats<T>::OnlineStats(const OnlineStats<T>& copy): _min(copy._min),
							 _max(copy._max),
							 _mean(copy._mean),
							 _delta(copy._delta),
							 _delta2(copy._delta2),
							 _M2(copy._M2),
							 _n(copy._n){}

template<typename T>
void OnlineStats<T>::insert(const T& value){
  double x = (double)value;
  _sum += value;
  _n ++;
  _min = MIN_(x, _min);
  _max = MAX_(x, _max);        
  _delta = x - _mean;
  _mean += _delta/_n;
  _delta2 = x - _mean;
  _M2 += _delta*_delta2;  
}

template<typename T>
T OnlineStats<T>::sum() const{ return _sum; }

template<typename T>
T OnlineStats<T>::min() const{ return _min; }

template<typename T>
T OnlineStats<T>::max() const{ return _max; }

template<typename T>
double OnlineStats<T>::mean() const{ return _mean; }

template<typename T>
double OnlineStats<T>::var() const{ return _M2 / (_n-1); }

template<typename T>
double OnlineStats<T>::sd() const{ return sqrt((double)this->var()); }

template<typename T>
unsigned OnlineStats<T>::count() const{ return _n; }

template class OnlineStats<double>;
template class OnlineStats<float>;
template class OnlineStats<int>;
template class OnlineStats<unsigned>;
template class OnlineStats<long>;
template class OnlineStats<unsigned long>;
template class OnlineStats<long long>;
template class OnlineStats<unsigned long long>;

