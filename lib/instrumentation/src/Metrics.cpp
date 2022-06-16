#include <iostream>
#include "Metrics.hpp"

using namespace Affinity;
using namespace std;

#define MAX_LEN 32

Metric::Metric(const string& name, const string&   value, const string& info){
  _vlen  = name.size() > value.size() ? name.size() : value.size();
  _name  = name;
  _value = value;
  _info  = info;
}

Metric::Metric(const string& name, const double& value, const string& info){
  char val[MAX_LEN]; memset(val, 0, MAX_LEN); snprintf(val, MAX_LEN, "%lf", value);
  _vlen = name.size() > strlen(val) ? name.size() : strlen(val);
  _name  = name;
  _value = string(val);
  _info  = info;
}

Metric::Metric(const string& name, const long& value, const string& info){
  char val[MAX_LEN]; memset(val, 0, MAX_LEN); snprintf(val, MAX_LEN, "%ld", value);
  _vlen = name.size() > strlen(val) ? name.size() : strlen(val);
  _name  = name;
  _value = string(val);
  _info  = info;
}


Metric::Metric(const string& name, const unsigned long& value, const string& info){
  char val[MAX_LEN]; memset(val, 0, MAX_LEN); snprintf(val, MAX_LEN, "%lu", value);
  _vlen = name.size() > strlen(val) ? name.size() : strlen(val);
  _name  = name;
  _value = string(val);
  _info  = info;
}

Metric::Metric(const string& name, const int& value, const string& info){
  char val[MAX_LEN]; memset(val, 0, MAX_LEN); snprintf(val, MAX_LEN, "%d", value);
  _vlen = name.size() > strlen(val) ? name.size() : strlen(val);
  _name  = name;
  _value = string(val);
  _info  = info;
}

Metric::Metric(const string& name, const unsigned int&  value, const string& info){
  char val[MAX_LEN]; memset(val, 0, MAX_LEN); snprintf(val, MAX_LEN, "%u", value);
  _vlen = name.size() > strlen(val) ? name.size() : strlen(val);
  _name  = name;
  _value = string(val);
  _info  = info;
}
    
string Metric::name()  const{ return _name;}
string Metric::value() const{ return _value;}
string Metric::info()  const{ return _info;}

void Metric::print_info(FILE* output)   const{ fprintf(output, "# %20s: %s\n", _name.c_str(), _info.c_str()); }
void Metric::print_header(FILE* output) const{ fprintf(output, "%*s ", _vlen, _name.c_str()); }
void Metric::print_value(FILE* output)  const{ fprintf(output, "%*s ", _vlen, _value.c_str()); }

AffinityMetrics::AffinityMetrics(){
  _sharing = Matrix<long>((unsigned)MAX_THREADS);
}

void AffinityMetrics::add_metric(const Metric& m){ _metrics.push_back(m); }

void AffinityMetrics::print_metrics(const string& output) const{
  FILE * out = fopen(output.c_str(), "w+");
  if(out == NULL){
    perror("fopen");
    return;
  }
  for(auto m = _metrics.begin(); m != _metrics.end(); m++){ m->print_info(out); }
  fprintf(out, "\n");
  for(auto m = _metrics.begin(); m != _metrics.end(); m++){ m->print_header(out); }
  fprintf(out, "\n");
  for(auto m = _metrics.begin(); m != _metrics.end(); m++){ m->print_value(out); }
  fprintf(out, "\n");
  printf("metrics output to %s\n", output.c_str());
  fclose(out);
}

void AffinityMetrics::print(const string& output) const{
  cerr << "metrics output to " << output << endl;
  print_metrics(output);
  cerr << "Sharing matrix output to sharing.mat" << endl;
  _sharing.print("sharing.mat");
}

void AffinityMetrics::print(const string& dir, const string& filename) const{
  string metrics_output = dir + string("/") + filename;
  string sharing_output = dir + string("/sharing.mat");  
  cerr << "Sharing matrix output to " << sharing_output << endl;
  _sharing.print(sharing_output.c_str());
  print_metrics(metrics_output);
}

void AffinityMetrics::print(){
  for(auto m = _metrics.begin(); m != _metrics.end(); m++){
    printf("%20s: %16s\n", m->name().c_str(), m->value().c_str());
  }
}


void AffinityMetrics::add_matrix_metrics(Affinity_Matrix<double> mat, const string& name){
  double priv = 0, tot=0;
  for(unsigned i=0; i<mat.size(); i++){
    priv += mat(i,i);
    for(unsigned j=0; j<i; j++){ tot += mat(i,j) + mat(j,i); }
  }

  mat.blankdiag();
  mat.scale(1.0);
  double clusterSD = mat.clusterSD();
  double amount    = mat.amount();
  double hopbyte   = mat.hopbyte();
  double CH, CB, CC, NBC;  
  mat.compute_stat(&CH, &CB, &CC, &NBC);
  
  add_metric(Metric(name + string("_private"),     priv/tot,   string("ratio of private (ie diag/(lower+upper))")));
  add_metric(Metric(name + string("_CH"),          CH,        string("matrix heterogeneity")));
  add_metric(Metric(name + string("_CB"),          CB,        string("matrix balance")));
  add_metric(Metric(name + string("_CC"),          CC,        string("average matrix centrality")));
  add_metric(Metric(name + string("_NC"),          NBC,       string("matrix neighboors communication fraction")));  
  add_metric(Metric(name + string("_amount"),      amount,    string("sum of matrix elements")));
  add_metric(Metric(name + string("_clusterSD"),   clusterSD, string("deviation of (NUMA) clusters amount")));
  add_metric(Metric(name + string("_hop_element"), hopbyte,   string("number of hop per element")));
}

void AffinityMetrics::add_PageTable_metrics(const PageTable& pt){
  PageTableStats * st = pt.stats();
  _sharing.copy(st->sharing_matrix);

  add_matrix_metrics(st->sharing_matrix.toDouble(), "sharing");  
  add_metric(Metric(string("avg_sharing_degree"),
		    st->sharing_degree.mean(),
		    string("average sharing degree among cachelines")));
  add_metric(Metric(string("sd_sharing_degree"),
		    st->sharing_degree.sd(),		    
		    string("sharing degree deviation among cachelines")));
  add_metric(Metric(string("avg_write_degree"),
		    st->writing_degree.mean(),
		    string("average writing degree among cachelines")));
  add_metric(Metric(string("sd_write_degree"),
		    st->writing_degree.sd(),
		    string("writing degree deviation among cachelines")));  
  add_metric(Metric(string("avg_write_ratio"),
		    st->write_ratio.mean(),
		    string("average write/read among cachelines")));
  add_metric(Metric(string("sd_write_ratio"),
		    st->write_ratio.sd(),
		    string("write/read deviation among cachelines")));  
  add_metric(Metric(string("avg_shared_write_ratio"),
		    st->shared_write_ratio.mean(),
		    string("average write/read among shared cachelines")));
  add_metric(Metric(string("sd_shared_write_ratio"),
		    st->shared_write_ratio.sd(),
		    string("write/read deviation among shared cachelines")));  
  add_metric(Metric(string("footprint"),
		    st->footprint.sum(),
		    string("total application footprint")));
  add_metric(Metric(string("sd_thread_footprint"),
		    st->footprint.sd(),
		    string("total application footprint")));
}

