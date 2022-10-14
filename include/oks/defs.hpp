#ifndef OKS_DEFS_H
#define OKS_DEFS_H

#include <string>
#include <iosfwd>

#include "ers/ers.hpp"

#include "oks/exceptions.hpp"


  /**
   *  @defgroup oks Package oks
   *
   *  @brief This package contains set of classes to implement OKS persistent in-memory object manager
   *  (see https://twiki.cern.ch/twiki/bin/viewauth/Atlas/DaqHltOks).
   *
   *  This software was developed for the ATLAS experiment at CERN.
   */

namespace oks
{
  /**
   *  @brief Convert C error number to string
   */

  const std::string
  strerror(int error);
}


  /// @addtogroup oks

  /**
   *  @ingroup oks
   *
   *  @brief Class contains common OKS classes and methods.
   */


class Oks {

  public:

    static std::ostream& error_msg(const char *);
    static std::ostream& warning_msg(const char *);
    static std::ostream& error_msg(const std::string& s) {return error_msg(s.c_str());}
    static std::ostream& warning_msg(const std::string& s) {return warning_msg(s.c_str());}

    static void substitute_variables(std::string&);
    static bool real_path(std::string&, bool ignore_errors);


      /**
       *  @brief String tokenizer.
       */

    class Tokenizer {

      public:

        Tokenizer(const std::string& s, const char * separator);
        const std::string next();
        bool next(std::string&);


      private:

        std::string p_string;
        const char * p_delimeters;
        std::string::size_type p_idx;

        static std::string s_empty;
    };

};



    // defines MACRO and CONSTANTS for verbose report

#ifndef ERS_NO_DEBUG
# include <boost/date_time/posix_time/posix_time_types.hpp>
# include <boost/date_time/posix_time/time_formatters.hpp>
# define OSK_VERBOSE_REPORT(MSG)	if(p_verbose) { boost::posix_time::ptime now = boost::posix_time::second_clock::universal_time(); std::cout << "[OKS " << boost::posix_time::to_simple_string(now) << "]: " << MSG << std::endl; std::cout.flush();}
#else
# define OSK_VERBOSE_REPORT(MSG)	;
#endif


    // defines MACRO for OKS profiler

#ifndef ERS_NO_DEBUG
# define OSK_PROFILING(FID, K)		OksFunctionProfiler yyy(FID, K);
#else
# define OSK_PROFILING(FID, K)		;
#endif


#endif
