#ifndef OKS_EXCEPTIONS_H
#define OKS_EXCEPTIONS_H

#include <exception>
#include <string>
#include <iostream>

namespace oks {

    /** Generic OKS exception. **/

  class exception : public std::exception {

    private:

      std::string p_what;
      int p_level;


    public:

      exception(const std::string& what_arg, int level_arg) noexcept;

      virtual ~exception() noexcept { }


        /** The level indicates position of nested oks exception. **/

      int level() const noexcept { return p_level; }


        /** Return reason in a string representation. **/

      virtual const char * what() const noexcept { return p_what.c_str(); }

  };

  inline std::ostream & operator<<( std::ostream & s, const oks::exception & ex) { s << ex.what(); return s; }

  void throw_validate_not_empty(const char * name);


    /** Check string value is not empty. Throw exception if length = 0. **/

  inline void validate_not_empty(const std::string& value, const char * name) {
    if(value.empty()) throw_validate_not_empty(name);
  }
}


#endif
