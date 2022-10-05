#ifndef __OKS_PROFILER
#define __OKS_PROFILER

#include <chrono>

#include <oks/defs.h>

class OksKernel;


class OksProfiler
{
public:
  enum FunctionID
  {
    KernelDestructor = 0,
    KernelOperatorOut,
    KernelLoadSchema,
    KernelSaveSchema,
    KernelCloseSchema,
    KernelCreateSchemaClassList,
    KernelLoadData,
    KernelSaveData,
    KernelCloseData,
    KernelCreateDataObjectList,
    KernelIsDanglingObject,
    KernelIsDanglingClass,
    ObjectStreamConstructor,
    ObjectNormalConstructor,
    ObjectCopyConstructor,
    ObjectDestructor,
    ObjectOperatorEqual,
    ObjectOperatorOut,
    ObjectSatisfiesQueryExpression,
    ObjectGetFreeObjectId,
    ObjectPutObject,
    ClassConstructor,
    ClassDestructor,
    ClassOperatorOut,
    ClassGetObject,
    Classexecute_query,
    ClassRegistrateClass,
    ClassRegistrateClassChange,
    ClassRegistrateAttributeChange,
    ClassRegistrateRelationshipChange,
    ClassRegistrateInstances,
    ClassCreateSubClassesList,
    fStringToQuery,
    fStringToQueryExpression,
    QueryOperatorFrom
  };

  friend std::ostream&
  operator<<(std::ostream&, const OksProfiler&);

  const std::chrono::time_point<std::chrono::steady_clock>
  start_time_point() const
  {
    return p_start_time_point;
  }

private:
  OksProfiler();

  void
  Start(OksProfiler::FunctionID, std::chrono::time_point<std::chrono::steady_clock>&);

  void
  Stop(OksProfiler::FunctionID, const std::chrono::time_point<std::chrono::steady_clock>&);

  std::chrono::time_point<std::chrono::steady_clock> p_start_time_point;

  double t_total[(unsigned) OksProfiler::QueryOperatorFrom + 1];
  unsigned long c_total[(unsigned) OksProfiler::QueryOperatorFrom + 1];

  friend class OksKernel;
  friend class OksFunctionProfiler;
};


class OksFunctionProfiler
{
public:
  OksFunctionProfiler(OksProfiler::FunctionID, const OksKernel*);
  ~OksFunctionProfiler();

private:
  const OksKernel *kernel;
  OksProfiler::FunctionID func_id;
  std::chrono::time_point<std::chrono::steady_clock> p_start_time_point;
};

#endif
