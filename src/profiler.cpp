#define _OksBuildDll_

#include "oks/profiler.hpp"
#include "oks/kernel.hpp"

static const char *OksProfilerFunctionsStr[] = {
  "OksKernel::Destructor",
  "OksKernel::operator<<",
  "OksKernel::load_schema",
  "OksKernel::save_schema",
  "OksKernel::close_schema",
  "OksKernel::CreateSchemaClassList",
  "OksKernel::load_data",
  "OksKernel::save_data",
  "OksKernel::close_data",
  "OksKernel::CreateDataObjectList",
  "OksKernel::is_danglingObject",
  "OksKernel::is_danglingClass",
  "OksObject::StreamConstructor",
  "OksObject::NormalConstructor",
  "OksObject::CopyConstructor",
  "OksObject::Destructor",
  "OksObject::operator==",
  "OksObject::operator<<",
  "OksObject::SatisfiesQueryExpression",
  "OksObject::GetFreeObjectId",
  "OksObject::PutObject",
  "OksClass::Constructor",
  "OksClass::Destructor",
  "OksClass::operator<<",
  "OksClass::GetObject",
  "OksClass::execute_query",
  "OksClass::RegistrateClass",
  "OksClass::RegistrateClassChange",
  "OksClass::RegistrateAttributeChange",
  "OksClass::RegistrateRelationshipChange",
  "OksClass::RegistrateInstances",
  "OksClass::CreateSubClassesList",
  "oksStringToQuery",
  "oksStringToQueryExpression",
  "OksQuery::operator<<"
};

namespace oks
{
  double
  get_time_interval(const timeval * t1, const timeval * t2)
  {
    return (t1->tv_sec - t2->tv_sec) + (t1->tv_usec - t2->tv_usec) / 1000000.;
  }
}

OksProfiler::OksProfiler()
{
  for(short i=0; i<=(short)QueryOperatorFrom; i++) {
    t_total[i] = 0.0;
    c_total[i] = 0;
  }

  p_start_time_point = std::chrono::steady_clock::now();
}

static void printTableSeparator(std::ostream& s, unsigned char c)
{
  s << '+';
  s.fill(c);
  s.width(41); s << "" << '+';
  s.width(12); s << "" << '+';
  s.width(10); s << "" << '+';
  s.width(10); s << "" << '+' << std::endl;
}
  
static void printTableLine(std::ostream& s, unsigned char c)
{
  s << '+';
  s.fill(c);
  s.width(76);
  s << "" << '+' << std::endl;
}
  
std::ostream&
operator<<(std::ostream& s, const OksProfiler& t)
{
  const int p = s.precision(4);

  printTableLine(s, '=');
  
  s << '|';
  s.fill(' ');
  s.width(76);
  s.setf(std::ios::left, std::ios::adjustfield);
  s << " O K S  P R O F I L E R " << '|' << std::endl;
  
  printTableLine(s, '-');
  
  s << "| Time: ";
  s.setf(std::ios::right, std::ios::adjustfield);
  s << std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now()-t.p_start_time_point).count() / 1000.;
  s.fill(' ');
  s.width(64);
  s.setf(std::ios::left, std::ios::adjustfield);
  s << " seconds" << '|' << std::endl;

  printTableSeparator(s, '-');

  s.fill(' ');
  s.width(42); s << "| Function name" << '|';
  s.width(12); s << " # of calls" << '|';
  s.width(10); s << " total T," << '|';
  s.width(10); s << " ~T/call," << '|' << std::endl;
	
  s.fill(' ');
  s.width(42); s << '|' << "" << '|';
  s.width(12); s << "" << '|';
  s.width(10); s << " ms" << '|';
  s.width(10); s << " µs" << '|' << std::endl;
	
  for(short i=0; i<=(short)OksProfiler::QueryOperatorFrom; i++) {
    if(i==0 || i==12 || i==21 || i==32) printTableSeparator(s, '-');

    s << "| ";

    s.width(40);
    s.fill(' ');
    s.setf(std::ios::left, std::ios::adjustfield);
    s << OksProfilerFunctionsStr[i] << '|';

    s.width(11);
    s.setf(std::ios::right, std::ios::adjustfield);
    s << t.c_total[i] << ' ' << '|';

    if(t.c_total[i]) {
      s.width(9);
      s << t.t_total[i]/1000. << ' ' << '|';

      s.width(9);
      s << t.t_total[i]/(double)t.c_total[i];
    }
    else {
      s.width(10);
      s << " - " << '|';

      s.width(9);
      s << " -";
    }

    s << " |\n";
  }

  printTableSeparator(s, '=');

  s.precision(p);

  return s;
}
  
void
OksProfiler::Start(OksProfiler::FunctionID fid, std::chrono::time_point<std::chrono::steady_clock>& time_point)
{
  time_point = std::chrono::steady_clock::now();
  c_total[fid]++;
}

void
OksProfiler::Stop(OksProfiler::FunctionID fid, const std::chrono::time_point<std::chrono::steady_clock>& time_point)
{
  t_total[fid] += std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now()-time_point).count();
}


OksFunctionProfiler::OksFunctionProfiler(OksProfiler::FunctionID fid, const OksKernel *k) :
  kernel  (k),
  func_id (fid)
{
  if(k && k->get_profiling_mode())
    k->GetOksProfiler()->Start(fid, p_start_time_point);
}

OksFunctionProfiler::~OksFunctionProfiler()
{
  if(kernel && kernel->get_profiling_mode())
    kernel->GetOksProfiler()->Stop(func_id, p_start_time_point);
}
