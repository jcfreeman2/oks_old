/**
 *  \file DalObjectPrint.h This file contains various print functions used to implement print() method in generated DAL classes.
 *  \author Igor Soloviev
 *  \brief print functions for generated template objects
 */

#ifndef CONFIG_DAL_OBJECT_PRINT_H_
#define CONFIG_DAL_OBJECT_PRINT_H_

#include <iostream>

namespace config {
  enum PrintNumBase {dec, hex, oct};

  template<class T>
  void
    p_val(std::ostream& s, const T & data)
    {
      s << data;
    }

  template<>
    void inline
    p_val<uint8_t>(std::ostream& s, const uint8_t & data)
    {
      s << static_cast<uint16_t>(data);
    }

  template<>
    void inline
    p_val<int8_t>(std::ostream& s, const int8_t & data)
    {
      s << static_cast<int16_t>(data);
    }


  template<PrintNumBase NUM_BASE = dec>
    void
    p_base_start(std::ostream &s) noexcept
    {
      if constexpr (NUM_BASE == hex)
        s.setf(std::ios::hex, std::ios::basefield);
      else if constexpr (NUM_BASE == oct)
        s.setf(std::ios::oct, std::ios::basefield);

      if constexpr (NUM_BASE != dec)
        s.setf(std::ios::showbase);
    }

  template<PrintNumBase NUM_BASE>
    void
    p_base_end(std::ostream &s) noexcept
    {
      if constexpr (NUM_BASE != dec)
        s.setf(std::ios::dec, std::ios::basefield);
    }

  /// print single-value attribute
  template<PrintNumBase NUM_BASE=dec, class T>
    void
    p_sv_attr(std::ostream &s, const std::string &str, const std::string &name, const T &val) noexcept
    {
      s << str << name << ": ";
      p_base_start<NUM_BASE>(s);
      p_val(s, val);
      p_base_end<NUM_BASE>(s);
      s << '\n';
    }

  /// print multi-value attribute
  template<PrintNumBase NUM_BASE=dec, class T>
    void
    p_mv_attr(std::ostream &s, const std::string &str, const std::string &name, const T &val) noexcept
    {
      if (!val.empty())
        {
          s << str << val.size() << " value(s) in " << name << ": ";
          p_base_start<NUM_BASE>(s);
          for (const auto &x : val)
            {
              if (x != *val.begin())
                s << ", ";
              p_val(s, x);
            }
         p_base_end<NUM_BASE>(s);
         s << '\n';
        }
      else
        {
          s << str << name << " value is empty\n";
        }
    }

  /// print weak single-value relationship
  void
  p_sv_rel(std::ostream &s, const std::string &str, const std::string &name, const DalObject *obj);

  /// print composite single-value relationship
  template<class T>
    void
    p_sv_rel(std::ostream &s, const std::string &str, unsigned int indent, const std::string &name, const T *obj) noexcept
    {
      s << str << name << ":\n";

      if (obj)
        obj->print(indent + 4, true, s);
      else
        s << str << "  (null)\n";
    }

  /// print weak multi-value relationship
  template<class T>
    void
    p_mv_rel(std::ostream &s, const std::string &str, const std::string &name, const T &objs) noexcept
    {
      if (auto len = objs.size())
        s << str << len << " object(s) in " << name << ": ";
      else
        s << str << name << " value is empty";

      for (const auto &x : objs)
        {
          if (x != *objs.begin())
            s << ", ";
          s << x;
        }

      s << '\n';
    }

  /// print composite multi-value relationship
  template<class T>
    void
    p_mv_rel(std::ostream &s, const std::string &str, unsigned int indent, const std::string &name, const T &objs) noexcept
    {
      if (auto len = objs.size())
        s << str << len << " object(s) in " << name << ":\n";
      else
        s << str << name << " value is empty\n";

      for (const auto &x : objs)
        x->print(indent + 4, true, s);
    }

}


#endif // CONFIG_CONFIGOBJECT_H_
