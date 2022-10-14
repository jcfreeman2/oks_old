/**	
 *  \file oks/attribute.h
 *
 *  This file is part of the OKS package.
 *  Author: Igor SOLOVIEV "https://phonebook.cern.ch/phonebook/#personDetails/?id=432778"
 *
 *  This file contains the declarations for the OKS attribute.
 */

#ifndef OKS_ATTRIBUTE_H
#define OKS_ATTRIBUTE_H

#include "oks/defs.hpp"
#include "oks/object.hpp"

#include <string>
#include <vector>

#include <boost/regex.hpp>


class OksXmlOutputStream;
class OksXmlInputStream;


/// @addtogroup oks

/**
 *  @ingroup oks
 *
 *  \brief OKS range class.
 *
 *  The OksRange class implements OKS range that can be defined for an OKS attribute.
 *  The range is a set of list of conditions using UML syntax.
 *  For string types the range is a regular expression.
 */

class OksRange
{
public:

  OksRange(const std::string& range, OksAttribute * a)
  {
    reset(range, a);
  }

  void
  reset(const std::string& range, OksAttribute * a);

  bool
  validate(const OksData&) const;

  inline bool
  is_empty()
  {
    return (m_less.empty() && m_equal.empty() && m_interval.empty() && m_great.empty() && m_like.empty());
  }

private:

  std::string m_range;

  std::list<OksData> m_less;
  std::list<OksData> m_equal;
  std::list<std::pair<OksData,OksData>> m_interval;
  std::list<OksData> m_great;
  std::list<boost::regex> m_like;

private:

  inline void
  clear()
  {
    m_range.clear();

    m_less.clear();
    m_equal.clear();
    m_interval.clear();
    m_great.clear();
    m_like.clear();
  }
};


  /// @addtogroup oks

  /**
   *    @ingroup oks
   *
   *	\brief OKS attribute class.
   *	
   *  	This class implements OKS attribute that is a part of an OKS class.
   *  	An attribute has name, type, range, initial value and description.
   *  	An attribute can be single-value or multi-value.
   *  	An attribute can be non-null.
   */

class OksAttribute
{
  friend class OksKernel;
  friend class OksClass;
  friend class OksIndex;
  friend class OksObject;
  friend struct OksData;

public:

  /** Format for integer representation (OKS data file, print output) */

  enum Format {
    Oct = 8,
    Dec = 10,
    Hex = 16
  };


  /**
   *  \brief OKS attribute simple constructor.
   *
   *  Create new OKS attribute providing name.
   *
   *  The parameter is:
   *  \param name     name of the attribute
   *  \param p        containing parent class
   *
   *  \throw oks::exception is thrown in case of problems.
   */

  OksAttribute(const std::string& name, OksClass * p = nullptr);


  /**
   *  \brief OKS attribute complete constructor.
   *
   *  Create new OKS attribute providing all properties.
   *
   *  The parameters are:
   *  \param name         name of the attribute (max 128 bytes, see #s_max_name_length variable)
   *  \param type         type of attribute; see set_type() for details
   *  \param is_mv        if true, the attribute is multi-values, see set_is_multi_values() for details
   *  \param range        range of attribute, see set_range() for details
   *  \param init_v       initial value of attribute
   *  \param description  description of the method (max 2000 bytes, see #s_max_description_length variable)
   *  \param no_null      if true, the value of attribute cannot be null, see set_is_no_null() for details
   *  \param format       format of attribute for integers, see set_format() for details
   *  \param p            containing parent class
   *
   *  \throw oks::exception is thrown in case of problems.
   */

  OksAttribute(const std::string& name, const std::string& type, bool is_mv, const std::string& range, const std::string& init_v, const std::string& description, bool no_null, Format format = Dec, OksClass * p = nullptr);


  ~OksAttribute()
  {
    if (p_enumerators)
      delete p_enumerators;

    clean_range();
  }


  bool operator==(const class OksAttribute&) const;                      /// equality operator
  friend  std::ostream& operator<<(std::ostream&, const OksAttribute&);  /// out stream operator

  const std::string&
  get_name() const noexcept
  {
    return p_name;
  }


  /**
   *  \brief Set attribute name.
   *
   *  Attributes linked with the same class shall have different names.
   *
   *  \param name    new name of attribute (max 128 bytes, see #s_max_name_length variable)
   *
   *  \throw oks::exception is thrown in case of problems.
   */

  void
  set_name(const std::string& name);


  /**
   *  \brief Get attribute string type.
   *
   *  The type meaning is described by the set_type() method.
   */

  const std::string&
  get_type() const noexcept;


  /**
   *  \brief Set attribute type.
   *
   *  The valid attribute types are:
   *  - \b "bool" - boolean
   *  - \b "s8" - signed 8-bits integer number
   *  - \b "u8" - unsigned 8-bits integer number
   *  - \b "s16" - signed 16-bits integer number
   *  - \b "u16" - unsigned 16-bits integer number
   *  - \b "s32" - signed 32-bits integer number
   *  - \b "u32" - unsigned 32-bits integer number
   *  - \b "s64" - signed 64-bits integer number
   *  - \b "u64" - unsigned 64-bits integer number
   *  - \b "float" - floating-point number
   *  - \b "double" - double precision floating-point number
   *  - \b "date" - date type
   *  - \b "time" - time type
   *  - \b "string" - string
   *  - \b "enum" - enumeration
   *  - \b "class" - reference on OKS class
   *
   *  \param type         new type
   *  \param skip_init    if true, do not initialise enum and range
   *
   *  \throw oks::exception is thrown in case of problems.
   */

  void
  set_type(const std::string& type, bool skip_init = false);


  /**
   *  \brief Get attribute range.
   *
   *  The range meaning is described by the set_range() method.
   */

  const std::string&
  get_range() const noexcept
  {
    return p_range;
  }


  /**
   *  \brief Set attribute range.
   *
   *  To define a range the UML syntax is used.
   *  It can be defined as list of tokens e.g. <b>"A,B,C..D,*..F,G..*"</b>,
   *  which means that the attribute may have one of the following value:
   *  - A
   *  - B
   *  - greater or equal to C and less or equal to D
   *  - less or equal to F
   *  - greater or equal to G
   *
   *  Note, that spaces are not allowed.
   *
   *  The empty range means that the attribute may have any value allowed by its type.
   *  The are two exceptions:
   *  - boolean attributes have predefined range "true,false"
   *  - enumeration attributes shall have non-empty range
   *
   *  \param range    new range (max 1024 bytes, see #s_max_range_length variable)
   *
   *  \throw oks::exception is thrown in case of problems.
   */

  void
  set_range(const std::string& range);


  /**
   *  \brief Converts string to attribute OKS data type.
   *
   *  See set_type() for more information about allowed values.
   *
   *  \param type    the string value to be converted to data type
   *
   *  \return        the OKS data type; return OksData::unknown_type if the type cannot be detected
   */

  static OksData::Type
  get_data_type(const std::string& type) noexcept;


  /**
   *  \brief Converts string to attribute OKS data type (fast version).
   *
   *  See set_type() for more information about allowed values.
   *
   *  \param type    the string value to be converted to data type
   *  \param len     lenght of type value
   *
   *  \return        the OKS data type; return OksData::unknown_type if the type cannot be detected
   */

  static OksData::Type
  get_data_type(const char * type, size_t len) noexcept;


  /** Get attribute OKS data type. */

  OksData::Type
  get_data_type() const noexcept
  {
    return p_data_type;
  }


  /**
   *  \brief Get attribute format.
   *
   *  The format meaning is described by the set_format() method.
   */

  Format
  get_format() const noexcept
  {
    return p_format;
  }


  /**
   *  \brief Set attribute format.
   *
   *  There are 3 OKS formats for integer attributes:
   *  - \b dec or \a decimal (default),
   *  - \b oct or \a octal,
   *  - \b hex or \a hexadecimal.
   *  For other attributes the format does not make sense.
   *
   *  The value of an integer attribute will be printed out and stored by OKS
   *  in the format defined by the schema for that attribute accordingly to C/C++ style.
   *  E.g. decimal value \a 255 can also be presented as \a 0377 (oct) or \a 0xFF (hex).
   *
   *  The method parameter is:
   *  \param format    new format
   *
   *  \throw oks::exception is thrown in case of problems.
   */

  void
  set_format(Format format);


  /** The method returns true, if the attribute is an integer number. */

  bool
  is_integer() const noexcept;


  /** The method returns true, if the attribute is a number. */

  bool
  is_number() const noexcept;


  /** The method returns true, if the attribute is multi-values. */

  bool
  get_is_multi_values() const noexcept
  {
    return p_multi_values;
  }


  /**
   *  \brief Set attribute is a single-value or multi-value.
   *
   *  If true, the attribute value contains multiple single values, e.g. list of numbers, strings, etc.
   *
   *  \param multi_values    pass true to declare the value is multi-value
   *
   *  \throw oks::exception is thrown in case of problems.
   */

  void
  set_is_multi_values(bool multi_values);


  /** The attribute initialisation value. */

  const std::string&
  get_init_value() const noexcept
  {
    return p_init_value;
  }


  /**
   *  \brief Set attribute initialisation value.
   *
   *  When new object is created, this is default value of attribute.
   *
   *  \param multi_values    new init value
   *
   *  \throw oks::exception is thrown in case of problems.
   */

  void
  set_init_value(const std::string& init_value);


  /**
   *  \brief Return list of initial values for mv-attribute.
   *
   *  \throw oks::AttributeReadError is thrown if initial values became invalid (may happen for class_type attributes),
   */

  std::list<std::string>
  get_init_values() const;


  /** Return description of attribute */

  const std::string&
  get_description() const noexcept
  {
    return p_description;
  }


  /**
   *  \brief Set attribute description.
   *
   *  \param description    new description of attribute (max 2000 bytes, see #s_max_description_length variable)
   *
   *  \throw oks::exception is thrown in case of problems.
   */

  void
  set_description(const std::string& description);


  /** Return true, if the attribute's value cannot be equal to null. */

  bool
  get_is_no_null() const noexcept
  {
    return p_no_null;
  }


  /**
   *  \brief Set attribute is-no-null property.
   *
   *  If true, a number cannot be 0 or string cannot be empty.
   *
   *  \param no_null    pass true to declare the value cannot be null
   *
   *  \throw oks::exception is thrown in case of problems.
   */


  void
  set_is_no_null(bool no_null);

  /**
   *  \brief Finds token in given range.
   *
   *  \param token    string to find
   *  \param range    pass true to declare the value cannot be null
   *  \return The method returns true, if the token is found in the range
   */


  static bool
  find_token(const char * token, const char * range) noexcept;

  /**
   *  \brief Finds index of given string in attribute's range.
   *
   *  The method can only be applied, if attribute is enumeration.
   *
   *  \param s         string to find
   *  \param length    length of string
   *
   *  \return The method returns 0 or positive integer index, if the string was found, and negative integer, if not.
   */


  int
  get_enum_index(const char * s, size_t length) const noexcept;

  /**
   *  \brief See get_enum_index(const char *, size_t);
   */

  int
  get_enum_index(const std::string& s) const noexcept
  {
    return get_enum_index(s.data(), s.length());
  }


  /**
   *  \brief Returns pointer on internal enumerator data equal to given string, if such string is defined in attribute's range.
   *
   *  The method can only be applied, if attribute is enumeration.
   *  The returned pointer is valid unless the attribute is not destroyed or modified.
   *
   *  \param s         string to find
   *  \param length    length of string
   *
   *  \return The method returns pointer on internal enumerator data, if given string is defined in attribute's range.
   *
   *  \throw std::exception is thrown, if no such string found.
   */

  const std::string *
  get_enum_value(const char * s, size_t length) const;


  /**
   *  \brief See get_enum_value(const char *, size_t).
   */

  const std::string *
  get_enum_value(const std::string& s) const
  {
    return get_enum_value(s.data(), s.length());
  }


  /**
   *  \brief Returns index of given enumeration in attribute's range (0 or positive integer value).
   *
   *  The method can only be applied, if attribute is enumeration and the data were created for given attribute.
   *
   *  \param d         OKS data created for given attribute
   *
   *  \return The method returns integer value of given enumeration.
   */


  uint16_t
  get_enum_value(const OksData& d) const noexcept;

  /**
   *  \brief Returns enumeration string by value.
   *
   *  The method can only be applied, if attribute is enumeration and the index is within number of items in enumeration range.
   *
   *  \param idx         enumeration index
   *
   *  \return The method returns pointer on string corresponding to enumeration integer value.
   */


  const std::string *
  get_enum_string(uint16_t idx) const noexcept
  {
    return &(*p_enumerators)[idx];
  }


  /**
   *  Valid attribute types
   */

  static const char * bool_type;
  static const char * s8_int_type;
  static const char * u8_int_type;
  static const char * s16_int_type;
  static const char * u16_int_type;
  static const char * s32_int_type;
  static const char * u32_int_type;
  static const char * s64_int_type;
  static const char * u64_int_type;
  static const char * float_type;
  static const char * double_type;
  static const char * date_type;
  static const char * time_type;
  static const char * string_type;
  static const char * uid_type;
  static const char * enum_type;
  static const char * class_type;

  static Format
  str2format(const char *) noexcept;
  static const char *
  format2str(Format) noexcept;

private:

  std::string p_name;
  std::string p_range;
  OksData::Type p_data_type;
  bool p_multi_values;
  bool p_no_null;
  std::string p_init_value;
  Format p_format;
  std::string p_description;
  OksClass * p_class;
  std::vector<std::string> * p_enumerators;
  OksRange * p_range_obj;
  OksData p_init_data;
  OksData p_empty_init_data;
  bool p_ordered;

  inline void
  __set_data_type(const char * t, size_t len) noexcept;

  /** Private constructor used by OksData */

  OksAttribute(OksData::Type t, const OksClass * c) noexcept :
      p_data_type(t), p_class(const_cast<OksClass*>(c)), p_enumerators(nullptr), p_range_obj(nullptr), p_ordered(false)
  {
    ;
  }


  /** Private constructor from XML stream */

  OksAttribute(OksXmlInputStream&, OksClass *);


  /** Private method to save in XML stream */

  void
  save(OksXmlOutputStream&) const;

  void
  init_enum();

  void
  init_range();

  inline void
  clean_range()
  {
    if (p_range_obj)
      {
        delete p_range_obj;
        p_range_obj = nullptr;
      }
  }

  void
  set_init_data()
  {
    p_init_data.set_init_value(this, false);
    p_empty_init_data.set_init_value(this, true);
  }

  /**
   *  Valid xml tags and attributes
   */

  static const char attribute_xml_tag[];
  static const char name_xml_attr[];
  static const char description_xml_attr[];
  static const char type_xml_attr[];
  static const char format_xml_attr[];
  static const char range_xml_attr[];
  static const char is_multi_value_xml_attr[];
  static const char mv_implement_xml_attr[];
  static const char init_value_xml_attr[];
  static const char is_not_null_xml_attr[];
  static const char ordered_xml_attr[];

};

inline void
OksAttribute::__set_data_type(const char * t, size_t len) noexcept
{
  p_data_type = get_data_type(t, len);
}

inline void
OksData::SetE(const char *s, size_t len, const OksAttribute *a)
{
  Clear();
  type = enum_type;
  data.ENUMERATION = a->get_enum_value(s, len);
}

inline void
OksData::SetE(const std::string &s, const OksAttribute *a)
{
  Clear();
  type = enum_type;
  data.ENUMERATION = a->get_enum_value(s);
}

inline void
OksData::SetE(const OksAttribute *a)
{
  Clear();
  type = enum_type;
  data.ENUMERATION = &((*(a->p_enumerators))[0]);
}

inline void
OksData::SetE(const OksString &s, const OksAttribute *a)
{
  Clear();
  type = enum_type;
  data.ENUMERATION = a->get_enum_value(s);
}

// profit from C++ string object vs. "char *" to create new string (string_type) or known string length (enum_type)

inline void
OksData::ReadFrom(const std::string& s, const OksAttribute * a)
{
  if (type == OksData::string_type)
    data.STRING = new OksString(s);
  else if (type == OksData::enum_type)
    data.ENUMERATION = a->get_enum_value(s);
  else
    SetValue(s.c_str(), a);
}

#endif
