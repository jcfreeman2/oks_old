#ifndef OKS_KERNEL_UTILS_H
#define OKS_KERNEL_UTILS_H

#include <map>
#include <string>

#include <boost/date_time/posix_time/posix_time_types.hpp>
#include <boost/date_time/gregorian/gregorian_types.hpp>

#include "oks/config/map.hpp"
#include "oks/exceptions.hpp"

class OksClass;
class OksObject;
class OksXmlInputStream;
class OksFile;
struct OksAliasTable;
class OksKernel;

namespace oks {

    // read date and time strings from OKS files (oks::Date, oks::Time or Boost ISO strings)

  boost::posix_time::ptime str2time(const char * value, size_t len, const char * file_name = nullptr);
  boost::gregorian::date str2date(const char * value, size_t len);


    // the structure for efficient search of objects to be re-read or to be deleted during reload

  struct ReloadObjects {
    std::map< const OksClass *, config::map<OksObject *> * > data;
    std::vector<OksObject *> created;

    ~ReloadObjects();
    void put(OksObject * obj);
    OksObject * pop(const OksClass* c, const std::string& id);
  };


    // the structure to pass common parameters to various read() methods of OksData and OksObject class

  struct ReadFileParams {
    OksFile* f;
    OksXmlInputStream& s;
    OksAliasTable * alias_table;
    OksKernel * oks_kernel;
    char format;
    ReloadObjects * reload_objects;
    const char * object_tag;
    size_t object_tag_len;
    OksObject * owner;
    std::string tmp;

    ReadFileParams(OksFile* f_, OksXmlInputStream& s_, OksAliasTable * t_, OksKernel * k_, char m_, ReloadObjects * l_) :
      f(f_), s(s_), alias_table(t_), oks_kernel(k_), format(m_), reload_objects(l_) { init(); }

    void init();
  };


  // 17-SEP-2009: for compatibility with previous OKS versions
  // FIXME: remove later ...

  class Date {

    public:

      Date		(const char * s) {set(s);} // dd/mm/[yy]yy

      virtual ~Date() { ; }

      void		set(const char *);

      long		year() const {return p_tm.tm_year;}
      unsigned short	month() const {return p_tm.tm_mon;}
      unsigned short	day() const {return p_tm.tm_mday;}

      virtual std::string str() const;


    protected:

      struct tm	p_tm;

  };

  std::ostream& operator<<(std::ostream&, const Date&);


  class Time : public Date {

    public:

      Time		(const char * s) : Date(s) {/*set(s);*/} // dd/mm/[yy]yy hh:mm[:ss]

      virtual ~Time() { ; }

      unsigned short	hour() const {return  p_tm.tm_hour;}
      unsigned short	min() const {return  p_tm.tm_min;}
      unsigned short	sec() const {return  p_tm.tm_sec;}

      virtual std::string str() const;
  };

  std::ostream& operator<<(std::ostream&, const Time&);

}

#endif
