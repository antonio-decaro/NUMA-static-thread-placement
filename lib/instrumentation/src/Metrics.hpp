#ifndef METRICS_HPP
#define METRICS_HPP

#include <stdio.h>
#include "Matrix.hpp"
#include "Stats.hpp"
#include "PageTable.hpp"

namespace Affinity{
  class Metric{
  private:
    std::string  _name;
    std::string  _value;
    std::string  _info;
    int          _vlen;
    
  public:
    Metric(const std::string& name, const std::string&   value, const std::string& info);
    Metric(const std::string& name, const double&        value, const std::string& info);
    Metric(const std::string& name, const long&          value, const std::string& info);
    Metric(const std::string& name, const unsigned long& value, const std::string& info);
    Metric(const std::string& name, const int&           value, const std::string& info);
    Metric(const std::string& name, const unsigned int&  value, const std::string& info);
    
    std::string name()  const;
    std::string value() const;
    std::string info()  const;

    void print_info(FILE* output)   const;
    void print_header(FILE* output) const;
    void print_value(FILE* output)  const;
  };
  
  class AffinityMetrics{
  private:
    Matrix<long>      _sharing;
    std::list<Metric> _metrics;
    void print_metrics(const std::string& output) const;
    
  public:
    AffinityMetrics();  
    void add_metric(const Metric& m);    
    void print(const std::string& output) const;
    void print(const std::string& dir, const std::string& filename) const;
    void print();
  
    void add_matrix_metrics(Affinity::Affinity_Matrix<double> mat, const std::string& type);
    void add_PageTable_metrics(const PageTable& pt);
  };
}

#endif
