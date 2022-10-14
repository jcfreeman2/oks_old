#define _OksBuildDll_

#include "oks/attribute.hpp"
#include "oks/xml.hpp"
#include "oks/class.hpp"
#include "oks/kernel.hpp"
#include "oks/object.hpp"
#include "oks/cstring.hpp"

#include "logging/Logging.hpp"

#include <sstream>
#include <stdexcept>

#include <string.h>


const char * OksAttribute::bool_type	= "bool";
const char * OksAttribute::s8_int_type	= "s8";
const char * OksAttribute::u8_int_type	= "u8";
const char * OksAttribute::s16_int_type	= "s16";
const char * OksAttribute::u16_int_type	= "u16";
const char * OksAttribute::s32_int_type	= "s32";
const char * OksAttribute::u32_int_type	= "u32";
const char * OksAttribute::s64_int_type	= "s64";
const char * OksAttribute::u64_int_type	= "u64";
const char * OksAttribute::float_type	= "float";
const char * OksAttribute::double_type	= "double";
const char * OksAttribute::date_type	= "date";
const char * OksAttribute::time_type	= "time";
const char * OksAttribute::string_type	= "string";
const char * OksAttribute::uid_type	= "uid";
const char * OksAttribute::enum_type	= "enum";
const char * OksAttribute::class_type	= "class";

const char OksAttribute::attribute_xml_tag[]       = "attribute";
const char OksAttribute::name_xml_attr[]           = "name";
const char OksAttribute::description_xml_attr[]    = "description";
const char OksAttribute::type_xml_attr[]           = "type";
const char OksAttribute::range_xml_attr[]          = "range";
const char OksAttribute::format_xml_attr[]         = "format";
const char OksAttribute::is_multi_value_xml_attr[] = "is-multi-value";
const char OksAttribute::init_value_xml_attr[]     = "init-value";
const char OksAttribute::is_not_null_xml_attr[]    = "is-not-null";
const char OksAttribute::ordered_xml_attr[]        = "ordered";


OksAttribute::Format
OksAttribute::str2format(const char * s) noexcept
{
  return (
    oks::cmp_str3(s, "dec") ? Dec :
    oks::cmp_str3(s, "hex") ? Hex :
    Oct
  );
}

const char *
OksAttribute::format2str(Format f) noexcept
{
  return (
    f == Dec ? "dec" :
    f == Hex ? "hex" :
    "oct"
  );
}

bool
OksAttribute::is_integer() const noexcept
{
  return (p_data_type >= OksData::s8_int_type && p_data_type <= OksData::u64_int_type);
}

bool
OksAttribute::is_number() const noexcept
{
  return (p_data_type >= OksData::s8_int_type && p_data_type <= OksData::bool_type);
}

void
OksAttribute::init_enum()
{
  if (p_data_type == OksData::enum_type)
    {
      if (p_enumerators)
        {
          p_enumerators->clear();
        }
      else
        {
          p_enumerators = new std::vector<std::string>();
        }

      Oks::Tokenizer t(p_range, ",");
      std::string token;

      while (t.next(token))
        {
          p_enumerators->push_back(token);
        }

      if (p_enumerators->empty())
        {
          std::ostringstream text;
          text << "range of enumeration attribute \"" << p_name << "\" is empty";
          throw std::runtime_error(text.str().c_str());
        }
    }
}

void
OksAttribute::init_range()
{
  clean_range();

  if (p_range.empty() == false)
    {
      p_range_obj = new OksRange(p_range, this);
      if (p_range_obj->is_empty())
        {
          clean_range();
        }
    }
}


int
OksAttribute::get_enum_index(const char * s, size_t length) const noexcept
{
  if (p_data_type == OksData::enum_type)
    {
      for (int i = 0; i < (int) p_enumerators->size(); ++i)
        {
          const std::string * x = &((*p_enumerators)[i]);
          if (x->length() == length)
            {
              if (memcmp(s, x->data(), length) == 0)
                {
                  return i;
                }
            }
        }

      return (-1);
    }

  return (-2);
}

const std::string *
OksAttribute::get_enum_value(const char * s, size_t length) const
{
  if (p_data_type == OksData::enum_type)
    {
      for (unsigned int i = 0; i < p_enumerators->size(); ++i)
        {
          const std::string * x = &((*p_enumerators)[i]);
          if (x->length() == length)
            {
              if (memcmp(s, x->data(), length) == 0)
                {
                  return x;
                }
            }
        }

      std::ostringstream text;
      text << "value \'" << s << "\' is out of range \'" << get_range() << '\'';
      throw std::runtime_error(text.str().c_str());
    }

  throw std::runtime_error("attribute is not enumeration");
}

uint16_t
OksAttribute::get_enum_value(const OksData& d) const noexcept
{
  const std::string * p_enumerators_first(&((*p_enumerators)[0]));
  return (d.data.ENUMERATION - p_enumerators_first);
}

void
validate_init2range(const OksAttribute * a)
{
  if (!a->get_range().empty() && a->get_data_type() != OksData::class_type)
    {
      try
        {
          OksData d;
          d.set_init_value(a, false);
          d.check_range(a);
        }
      catch (std::exception& ex)
        {
          std::ostringstream text;
          text << "failed to set initial value \'" << a->get_init_value() << "\' of attribute \'" << a->get_name() << "\':\n" << ex.what();
          throw std::runtime_error(text.str().c_str());
        }
    }
}


OksAttribute::OksAttribute(const std::string& nm, OksClass * p) :
  p_name	   (nm),
  p_data_type      (OksData::string_type),
  p_multi_values   (false),
  p_no_null	   (false),
  p_format         (OksAttribute::Dec),
  p_class          (p),
  p_enumerators    (nullptr),
  p_range_obj      (nullptr),
  p_ordered        (false)

{
  oks::validate_not_empty(p_name, "attribute name");
}


OksAttribute::OksAttribute(const std::string& nm, const std::string& t, bool is_mv,
                           const std::string& r, const std::string& init_v, const std::string& ds,
			   bool no_null, Format f, OksClass * p) :
  p_name	   (nm),
  p_range	   (r),
  p_data_type      (get_data_type(t)),
  p_multi_values   (is_mv),
  p_no_null        (no_null),
  p_init_value	   (init_v),
  p_format         (f),
  p_description	   (ds),
  p_class	   (p),
  p_enumerators    (nullptr),
  p_range_obj      (nullptr),
  p_ordered        (false)
{
  set_type(t, true);
  oks::validate_not_empty(p_name, "attribute name");
  init_enum();
  init_range();
  // cannot call set_init_data() because of the CLASS data type, since other class may not be known yet;
  // initialize the init_data in the register_all_classes() call
}

bool
OksAttribute::operator==(const class OksAttribute &a) const
{
  return (
    ( this == &a ) ||
    (
      ( p_name == a.p_name ) &&
      ( p_range == a.p_range ) &&
      ( p_data_type == a.p_data_type ) &&
      ( p_multi_values == a.p_multi_values ) &&
      ( p_init_value == a.p_init_value ) &&
      ( p_description == a.p_description ) &&
      ( p_no_null == a.p_no_null ) &&
      ( p_ordered == a.p_ordered )
   )
  );
}

std::ostream&
operator<<(std::ostream& s, const OksAttribute& a)
{
  s << "Attribute name: \"" << a.p_name << "\"\n"
       " type: \"" << a.get_type() << "\"\n"
       " range: \"" << a.p_range << "\"\n";

  if(a.is_integer()) {
    s << " format: \"" << OksAttribute::format2str(a.p_format) << "\"\n";
  }

  if(a.p_multi_values == true) {
    s << " is \'multi values\'\n";
  }
  else {
    s << " is \'single value\'\n";
  }

  s << " initial value: \"" << a.p_init_value << "\"\n"
       " has description: \"" << a.p_description << "\"\n"
    << (a.p_no_null == true ? " can not be null\n" : " can be null\n")
    << " is " << (a.p_ordered == true ? "ordered" : "unordered") << std::endl;

  return s;
}


void
OksAttribute::save(OksXmlOutputStream& s) const
{
  if (p_data_type == OksData::class_type && p_init_value.empty() && p_multi_values == false)
    {
      std::ostringstream text;
      text << "single-value attribute \"" << p_name << "\" is of \"class_type\" and has empty init value";
      throw std::runtime_error(text.str().c_str());
    }

  validate_init2range(this);

  s.put("  ");
  s.put_start_tag(attribute_xml_tag, sizeof(attribute_xml_tag) - 1);

  s.put_attribute(name_xml_attr, sizeof(name_xml_attr) - 1, p_name.c_str());

  if (!p_description.empty())
    s.put_attribute(description_xml_attr, sizeof(description_xml_attr) - 1, p_description.c_str());

  s.put_attribute(type_xml_attr, sizeof(type_xml_attr) - 1, get_type().c_str());

  if (!p_range.empty())
    s.put_attribute(range_xml_attr, sizeof(range_xml_attr) - 1, p_range.c_str());

  if (is_integer() && p_format != OksAttribute::Dec)
    s.put_attribute(format_xml_attr, sizeof(format_xml_attr) - 1, format2str(p_format));

  if (p_multi_values)
    s.put_attribute(is_multi_value_xml_attr, sizeof(is_multi_value_xml_attr) - 1, oks::xml::bool2str(p_multi_values));

  if (!p_init_value.empty())
    s.put_attribute(init_value_xml_attr, sizeof(init_value_xml_attr) - 1, p_init_value.c_str());

  if (p_no_null)
    s.put_attribute(is_not_null_xml_attr, sizeof(is_not_null_xml_attr) - 1, oks::xml::bool2str(p_no_null));

  if (p_ordered)
    s.put_attribute(ordered_xml_attr, sizeof(ordered_xml_attr) - 1, oks::xml::bool2str(p_ordered));

  s.put_end_tag();
}


OksAttribute::OksAttribute(OksXmlInputStream& s, OksClass *parent) :
  p_data_type      (OksData::unknown_type),
  p_multi_values   (false),
  p_no_null	   (false),
  p_format         (OksAttribute::Dec),
  p_class          (parent),
  p_enumerators    (nullptr),
  p_range_obj      (nullptr),
  p_ordered        (false)
{
  // read attributes of OksAttribute from xml

  try
    {
      while (true)
        {
          OksXmlAttribute attr(s);

          // check for close of tag

          if (oks::cmp_str1(attr.name(), "/"))
            {
              break;
            }

          // check for known oks-attribute' attributes

          else if (oks::cmp_str4(attr.name(), name_xml_attr))
            p_name.assign(attr.value(), attr.value_len());
          else if (oks::cmp_str11(attr.name(), description_xml_attr))
            p_description.assign(attr.value(), attr.value_len());
          else if (oks::cmp_str4(attr.name(), type_xml_attr))
            {
              __set_data_type(attr.value(), attr.value_len());
              if (p_data_type == OksData::unknown_type)
                {
                  throw oks::BadFileData(std::string("Value \'") + attr.value() + "\' is not a valid attribute type", s.get_line_no(), s.get_line_pos());
                }
            }
          else if (oks::cmp_str5(attr.name(), range_xml_attr))
            p_range.assign(attr.value(), attr.value_len());
          else if (oks::cmp_str6(attr.name(), format_xml_attr))
            p_format = str2format(attr.value());
          else if (oks::cmp_str14(attr.name(), is_multi_value_xml_attr))
            p_multi_values = oks::xml::str2bool(attr.value());
          else if (oks::cmp_str10(attr.name(), init_value_xml_attr))
            p_init_value.assign(attr.value(), attr.value_len());
          else if (oks::cmp_str11(attr.name(), is_not_null_xml_attr))
            p_no_null = oks::xml::str2bool(attr.value());
          else if (oks::cmp_str7(attr.name(), ordered_xml_attr))
            p_ordered = oks::xml::str2bool(attr.value());
          else if (!strcmp(attr.name(), "multi-value-implementation"))
            s.error_msg("OksAttribute::OksAttribute(OksXmlInputStream&)") << "Obsolete oks-attribute\'s attribute \'" << attr.name() << "\'\n";
          else
            s.throw_unexpected_attribute(attr.name());
        }
    }
  catch (oks::exception & e)
    {
      throw oks::FailedRead("xml attribute", e);
    }
  catch (std::exception & e)
    {
      throw oks::FailedRead("xml attribute", e.what());
    }

  // check validity of read values

  if (p_data_type == OksData::unknown_type)
    {
      throw oks::FailedRead("oks attribute", oks::BadFileData("attribute type is not set", s.get_line_no(), s.get_line_pos()));
    }

  try
    {
      oks::validate_not_empty(p_name, "attribute name");
      init_enum();
      init_range();
    }
  catch (std::exception& ex)
    {
      throw oks::FailedRead("oks attribute", oks::BadFileData(ex.what(), s.get_line_no(), s.get_line_pos()));
    }

  if (p_init_value.empty())
    {
      if (p_data_type == OksData::class_type && p_multi_values == false)
        {
          std::ostringstream text;
          text << "single-value attribute \"" << p_name << "\" is of \"class_type\" and has empty init value";
          throw oks::FailedRead("oks attribute", oks::BadFileData(text.str(), s.get_line_no(), s.get_line_pos()));
        }
    }
  else
    {
      if (p_data_type == OksData::date_type || p_data_type == OksData::time_type)
        {
          try
            {
              OksData _d;
              if (p_multi_values)
                {
                  _d.SetValues(p_init_value.c_str(), this);
                }
              else
                {
                  _d.type = p_data_type;
                  _d.SetValue(p_init_value.c_str(), 0);
                }
            }
          catch (oks::exception& ex)
            {
              std::ostringstream text;
              text << "attribute \"" << p_name << "\" has bad init value:\n" << ex.what();
              throw oks::FailedRead("oks attribute", oks::BadFileData(text.str(), s.get_line_no(), s.get_line_pos()));
            }
        }
    }

  try
    {
      validate_init2range(this);
    }
  catch (std::exception& ex)
    {
      std::ostringstream text;
      text << "attribute \"" << p_name << "\" has mismatch between init value and range:\n" << ex.what();
      throw oks::FailedRead("oks attribute", oks::BadFileData(text.str(), s.get_line_no(), s.get_line_pos()));
    }

//  set_init_data();
}


OksData::Type
OksAttribute::get_data_type(const std::string& t) noexcept
{
  return get_data_type(t.c_str(), t.size());
}


OksData::Type
OksAttribute::get_data_type(const char * t, size_t len) noexcept
{
  switch(len) {
    case 3:
      if     ( oks::cmp_str3n  (t, uid_type)     ) return OksData::uid2_type;     // "uid"
      else if( oks::cmp_str3n  (t, u32_int_type) ) return OksData::u32_int_type;  // "u32"
      else if( oks::cmp_str3n  (t, s32_int_type) ) return OksData::s32_int_type;  // "s32"
      else if( oks::cmp_str3n  (t, u16_int_type) ) return OksData::u16_int_type;  // "u16"
      else if( oks::cmp_str3n  (t, s16_int_type) ) return OksData::s16_int_type;  // "s16"
      else if( oks::cmp_str3n  (t, s64_int_type) ) return OksData::s64_int_type;  // "s64"
      else if( oks::cmp_str3n  (t, u64_int_type) ) return OksData::u64_int_type;  // "u64"
      break;

    case 6:
      if(      oks::cmp_str6n  (t, string_type)  ) return OksData::string_type;   // "string"
      else if( oks::cmp_str6n  (t, double_type)  ) return OksData::double_type;   // "double"
      break;

    case 4:
      if(      oks::cmp_str4n (t, bool_type)     ) return OksData::bool_type;    // "bool"
      else if( oks::cmp_str4n (t, enum_type)     ) return OksData::enum_type;    // "enum"
      else if( oks::cmp_str4n (t, date_type)     ) return OksData::date_type;    // "date"
      else if( oks::cmp_str4n (t, time_type)     ) return OksData::time_type;    // "time"
      break;

    case 5:
      if(      oks::cmp_str5n  (t, float_type)   ) return OksData::float_type;   // "float"
      else if( oks::cmp_str5n  (t, class_type)   ) return OksData::class_type;   // "class"
      break;

    case 2:
      if(      oks::cmp_str2n  (t, s8_int_type)  ) return OksData::s8_int_type;  // "s8"
      else if( oks::cmp_str2n  (t, u8_int_type)  ) return OksData::u8_int_type;  // "u8"
      break;
  }

  return OksData::unknown_type;
}


const std::string&
OksAttribute::get_type() const throw()
{
  static std::string __types [] = {
    "unknown",     // unknown_type = 0
    "s8",          // s8_int_type  = 1
    "u8",          // u8_int_type  = 2
    "s16",         // s16_int_type = 3,
    "u16",         // u16_int_type = 4,
    "s32",         // s32_int_type = 5,
    "u32",         // u32_int_type = 6,
    "s64",         // s64_int_type = 7,
    "u64",         // u64_int_type = 8,
    "float",       // float_type   = 9,
    "double",      // double_type  = 10,
    "bool",        // bool_type    = 11,
    "class",       // class_type   = 12,
    "object",      // object_type  = 13,
    "date",        // date_type    = 14,
    "time",        // time_type    = 15,
    "string",      // string_type  = 16,
    "list",        // list_type    = 17,
    "uid",         // uid_type     = 18,
    "uid2",        // uid2_type    = 19,
    "enum"         // enum_type    = 20
  };

  return __types[(int)p_data_type];
}


void
OksAttribute::set_type(const std::string& t, bool skip_init)
{
  OksData::Type p_dt = get_data_type(t);

  if (p_dt == OksData::unknown_type)
    {
      std::ostringstream text;
      text << "the type \'" << t << "\' is not valid";
      throw oks::SetOperationFailed("OksAttribute::set_type", text.str());
    }

  if (p_data_type == p_dt)
    return;

  if (p_class)
    p_class->lock_file("OksAttribute::set_type");

  p_data_type = p_dt;

  if (skip_init == false)
    {
      try
        {
          init_enum();
          init_range();
        }
      catch (std::exception& ex)
        {
          throw oks::SetOperationFailed("OksAttribute::set_type", ex.what());
        }
    }

  if (p_class)
    {
      p_class->registrate_attribute_change(this);
      p_class->registrate_class_change(OksClass::ChangeAttributeType, (const void *) this);
    }
}

void
OksAttribute::set_name(const std::string& new_name)
{
  // ignore when name is the same

  if (p_name == new_name)
    return;

  // additional checks are required,
  // if the attribute already belongs to some class

  if (p_class)
    {
      // check allowed length for attribute name

      try
        {
          oks::validate_not_empty(new_name, "name");
        }
      catch (std::exception& ex)
        {
          throw oks::SetOperationFailed("OksAttribute::set_name", ex.what());
        }

      // having a direct attribute with the same name is an error

      if (p_class->find_direct_attribute(new_name) != 0)
        {
          std::ostringstream text;
          text << "Class \"" << p_class->get_name() << "\" already has direct attribute \"" << new_name << '\"';
          throw oks::SetOperationFailed("OksAttribute::set_name", text.str());
        }

      // check that the file can be updated

      p_class->lock_file("OksAttribute::set_name");

      // probably a non-direct attribute already exists

      OksAttribute * a = p_class->find_attribute(new_name);

      // change the name

      p_name = new_name;

      // registrate the change

      p_class->registrate_class_change(OksClass::ChangeAttributesList, (const void *) a);
    }
  else
    {
      p_name = new_name;
    }
}

void
OksAttribute::set_is_multi_values(bool is_mv)
{
  if (p_multi_values != is_mv)
    {
      if (p_class)
        p_class->lock_file("OksAttribute::set_is_multi_values");

      p_multi_values = is_mv;

      if (p_class)
        {
          p_class->registrate_class_change(OksClass::ChangeAttributeMultiValueCardinality, (const void *) this);
          p_class->registrate_attribute_change(this);
        }
    }
}

void
OksAttribute::set_init_value(const std::string& init_v)
{
  if (p_init_value != init_v)
    {
      std::string old_value = p_init_value;

      if (p_class)
        p_class->lock_file("OksAttribute::set_init_value");

      p_init_value = init_v;

      try
        {
          validate_init2range(this);
        }
      catch (...)
        {
          p_init_value = old_value;
          throw;
        }

      if (p_class)
        p_class->registrate_class_change(OksClass::ChangeAttributeInitValue, (const void *) this);
    }
}

void
OksAttribute::set_description(const std::string& ds)
{
  if (p_description != ds)
    {
      if (p_class)
        p_class->lock_file("OksAttribute::set_description");

      p_description = ds;

      if (p_class)
        p_class->registrate_class_change(OksClass::ChangeAttributeDescription, (const void *) this);
    }
}


void
OksAttribute::set_is_no_null(bool no_null)
{
  if (p_no_null != no_null)
    {
      if (p_class)
        p_class->lock_file("OksAttribute::set_is_no_null");

      p_no_null = no_null;

      if (p_class)
        p_class->registrate_class_change(OksClass::ChangeAttributeIsNoNull, (const void *) this);
    }
}

void
OksAttribute::set_format(Format f)
{
  if (f != p_format)
    {
      if (p_class)
        p_class->lock_file("OksAttribute::set_format");

      p_format = f;

      if (p_class)
        p_class->registrate_class_change(OksClass::ChangeAttributeFormat, (const void *) this);
    }
}

void
OksAttribute::set_range(const std::string& s)
{
  if (s != p_range)
    {
      std::string old_value = p_range;

      if (p_class)
        p_class->lock_file("OksAttribute::set_range");

      p_range.erase();
      int count = 0;
      Oks::Tokenizer t(s, ", \t");
      std::string token;

      while (t.next(token))
        {
          if (count++ != 0)
            {
              p_range += ',';
            }

          p_range += token;
        }

      if (count != 0)
        {
          if (p_data_type == OksData::bool_type)
            {
              p_range.erase();
              throw oks::SetOperationFailed("OksAttribute::set_range", "boolean type can't have user-defined range!");
            }
        }

      try
        {
          init_enum();
          init_range();
          validate_init2range(this);
        }
      catch (std::exception& ex)
        {
          p_range = old_value;

          try
            {
              init_enum();
            }
          catch (...)
            {
              ;
            }

          try
            {
              init_range();
            }
          catch (...)
            {
              ;
            }

          throw oks::SetOperationFailed("OksAttribute::set_range", ex.what());
        }

      if (p_class)
        p_class->registrate_class_change(OksClass::ChangeAttributeRange, (const void *) this);
    }
}

bool
OksAttribute::find_token(const char * token, const char * range) noexcept
{
  const char * p;

  if (token && (p = strstr(range, token)) != 0)
    {
      int len = strlen(token);

      do
        {
          if (((p != range) && (p[-1] != ',')) || ((p[len] != ',') && (p[len] != '\0')))
            {
              p = strstr(p + 1, token);
            }
          else
            {
              return true;
            }

        }
      while (p != 0);
    }

  return false;
}

inline bool
is_star(const std::string& s)
{
  return (s.size() == 1 && s[0] == '*');
}


void
OksRange::reset(const std::string& range, OksAttribute * a)
{
  clear();

  if (a->get_data_type() != OksData::string_type)
    {
      if (!range.empty())
        {
          Oks::Tokenizer t(range, ",");
          std::string token, token1, token2;

          while (t.next(token))
            {
              if (is_star(token))
                {
                  TLOG_DEBUG(2) << "token \'" << token << "\' of \'" << range << "\' allows any value";
                  clear();
                  return;
                }

              static const char __dot_dot_str[] = "..";
              std::string::size_type p = token.find(__dot_dot_str, 0, (sizeof(__dot_dot_str) - 1));

              bool pi; // if true, then it is plus infinity, i.e. x..*

              if (p != std::string::npos)
                {
                  token1.assign(token, 0, p);
                  token2.assign(token, p + 2, std::string::npos);
                  pi = (is_star(token2));
                }
              else
                {
                  token1.assign(token);
                  token2.clear();
                  pi = false;
                }

              bool mi = (is_star(token1));    // if true, then it is minus infinity, i.e. *..x

              if (mi && pi)
                {
                  TLOG_DEBUG(2) << "token \'" << token << "\' of \'" << range << "\' allows any value";
                  clear();
                  return;
                }

              OksData d1, d2;

              if (!mi)
                {
                  d1.type = a->get_data_type();
                  d1.ReadFrom(token1, a);
                }

              if (token2.empty())
                {
                  TLOG_DEBUG(2) <<  "token \'" << token << "\' of \'" << range << "\' defines equality condition";
                  m_equal.emplace_back(d1);
                }
              else
                {
                  if (!pi)
                    {
                      d2.type = a->get_data_type();
                      d2.ReadFrom(token2, a);
                    }

                  if (mi)
                    {
                      TLOG_DEBUG(2) << "token \'" << token << "\' of \'" << range << "\' defines smaller condition";
                      m_less.emplace_back(d2);
                    }
                  else if (pi)
                    {
                      TLOG_DEBUG(2) << "token \'" << token << "\' of \'" << range << "\' defines greater condition";
                      m_great.emplace_back(d1);
                    }
                  else
                    {
                      TLOG_DEBUG(2) << "token \'" << token << "\' of \'" << range << "\' defines interval condition";
                      m_interval.emplace_back(d1, d2);
                    }
                }
            }
        }
    }
  else
    {
      try
        {
          m_like.emplace_back(range);
        }
      catch (std::exception& ex)
        {
          throw oks::BadReqExp(range, ex.what());
        }
    }
}

bool
OksRange::validate(const OksData& d) const
{
  for (const auto& x : m_less)
    {
      if (d <= x)
        return true;
    }

  for (const auto& x : m_great)
    {
      if (d >= x)
        return true;
    }

  for (const auto& x : m_equal)
    {
      if (d == x)
        return true;
    }

  for (const auto& x : m_interval)
    {
      if (d >= x.first && d <= x.second)
        return true;
    }

  for (const auto& x : m_like)
    {
      if (OksKernel::get_skip_string_range())
        return true;

      if (boost::regex_match(d.str(),x))
        return true;
    }

  return false;
}
