#define _OksBuildDll_

#include <oks/object.h>
#include <oks/xml.h>
#include <oks/attribute.h>
#include <oks/relationship.h>
#include <oks/class.h>
#include <oks/kernel.h>
#include <oks/cstring.h>

#include "oks_utils.h"

#include "logging/Logging.hpp"

#include <string.h>
#include <stdlib.h>

#include <string>
#include <sstream>
#include <stdexcept>
#include <limits>

#include <boost/date_time/posix_time/time_formatters.hpp>
#include <boost/date_time/posix_time/time_parsers.hpp>
#include <boost/date_time/gregorian/gregorian.hpp>


  // ugly reinterpret staff to convert date and time to integers

void OksData::SetFast(boost::gregorian::date d)
{
  data.DATE = d.day_number();
}

void OksData::SetFast(boost::posix_time::ptime t)
{
  data.TIME = *reinterpret_cast<uint64_t*>(&t);
}

boost::gregorian::date OksData::date() const noexcept
{
  return boost::gregorian::date(boost::gregorian::gregorian_calendar::from_day_number(data.DATE));
}

boost::posix_time::ptime OksData::time() const noexcept
{
  return *reinterpret_cast<boost::posix_time::ptime*>(const_cast<uint64_t*>(&data.TIME));
}


inline void free_list(OksData::List& dlist)
{
  for(OksData::List::const_iterator i = dlist.begin(); i != dlist.end(); ++i) {
    delete *i;
  }
  dlist.clear();
}


static bool
operator==(const OksData::List & l1, const OksData::List & l2)
{
  if(&l1 == &l2) return true;

  size_t l1Lenght = l1.size();
  size_t l2Lenght = l2.size();

  if(!l1Lenght && !l2Lenght) return true;
  if(l1Lenght != l2Lenght) return false;

  size_t pos = 0;

  auto i1 = l1.begin();
  auto i2 = l2.begin();

  for(;i1 != l1.end(); ++i1, ++i2) {
    OksData * d1 = *i1;
    OksData * d2 = *i2;

    if( !d1 && !d2 ) return true;
    if( !d1 || !d2 ) return false;

    if(
     (
       (d1->type == d2->type) &&
       (d1->type == OksData::object_type) &&
       (
	 (!d1->data.OBJECT && !d2->data.OBJECT) ||
	 (
	  (d1->data.OBJECT && d2->data.OBJECT) &&
  	  (d1->data.OBJECT->GetClass()->get_name() == d2->data.OBJECT->GetClass()->get_name()) &&
  	  (d1->data.OBJECT->GetId() == d2->data.OBJECT->GetId())
	 )
       )
     ) ||
     (
       (*d1 == *d2)
     )
    ) {
       pos++;
       continue;
    }

    return false;
  }
  
  return true;
}


bool
OksData::operator==(const OksData& d) const {
  return (
    (d.type == type) &&
    (
      ( (type == string_type) && (*d.data.STRING == *data.STRING) ) ||
      ( (type == u32_int_type) && (d.data.U32_INT == data.U32_INT) ) ||
      ( (type == s32_int_type) && (d.data.S32_INT == data.S32_INT) ) ||
      ( (type == u16_int_type) && (d.data.U16_INT == data.U16_INT) ) ||
      ( (type == s16_int_type) && (d.data.S16_INT == data.S16_INT) ) ||
      ( (type == s8_int_type) && (d.data.S8_INT == data.S8_INT) ) ||
      ( (type == u8_int_type) && (d.data.U8_INT == data.U8_INT) ) ||
      ( (type == s64_int_type) && (d.data.S64_INT == data.S64_INT) ) ||
      ( (type == u64_int_type) && (d.data.U64_INT == data.U64_INT) ) ||
      ( (type == float_type) && (d.data.FLOAT == data.FLOAT) ) ||
      ( (type == double_type) && (d.data.DOUBLE == data.DOUBLE) ) ||
      ( (type == bool_type) && (d.data.BOOL == data.BOOL) ) ||
      ( (type == date_type) && (d.data.DATE == data.DATE) ) ||
      ( (type == time_type) && (d.data.TIME == data.TIME) ) ||
      ( (type == object_type) && OksObject::are_equal_fast(d.data.OBJECT, data.OBJECT) ) ||
      ( (type == list_type) && (*d.data.LIST == *data.LIST) ) ||
      ( (type == uid_type) && (d.data.UID.class_id->get_name() == data.UID.class_id->get_name() && *d.data.UID.object_id == *data.UID.object_id) ) ||
      ( (type == uid2_type) && (*d.data.UID2.class_id == *data.UID2.class_id && *d.data.UID2.object_id == *data.UID2.object_id) ) ||
      ( (type == enum_type) && (*d.data.ENUMERATION == *data.ENUMERATION) ) ||
      ( (type == class_type) && (d.data.CLASS->get_name() == data.CLASS->get_name()) )
    )
  );
}

bool
OksData::operator!=(const OksData& d) const {
  return ( (d == *this) ? false : true );
}


static bool
test_comparable(OksData::Type type1, OksData::Type type2)
{
  const char * fname = "OksData::operator[<|<=|>|>=](const OksData&) const";
  const char * h3 = "Can't compare ";

  if(type1 != type2 && !(OksData::is_object(type1) && OksData::is_object(type2))) {
    Oks::error_msg(fname) << h3 << "OKS data of different types\n";
    return false;
  }
  else if(type1 == OksData::list_type) {
    Oks::error_msg(fname) << h3 << "lists\n";
    return false;
  }

  return true;
}

const std::string&
OksData::__oid() const
{
  if(type == object_type)
    return data.OBJECT->uid.object_id;
  else if(type == uid_type)
    return *data.UID.object_id;
  else
    return *data.UID2.object_id;
}

const std::string&
OksData::__cn() const
{
  if(type == object_type)
    return data.OBJECT->uid.class_id->get_name();
  else if(type == uid_type)
    return data.UID.class_id->get_name();
  else
    return *data.UID2.class_id;
}

#define CMP_OBJ( OP, c1, c2, id1, id2 )  (c1 OP c2) || (!(c2 OP c1) && id1 OP id2)

#define CMP_DATA( OP ) \
  switch(type) {                                                                        \
    case string_type:   return (*data.STRING OP *d.data.STRING);                        \
    case u32_int_type:  return (data.U32_INT OP d.data.U32_INT);                        \
    case s32_int_type:  return (data.S32_INT OP d.data.S32_INT);                        \
    case u16_int_type:  return (data.U16_INT OP d.data.U16_INT);                        \
    case s16_int_type:  return (data.S16_INT OP d.data.S16_INT);                        \
    case s8_int_type:   return (data.S8_INT OP d.data.S8_INT);                          \
    case u8_int_type:   return (data.U8_INT OP d.data.U8_INT);                          \
    case s64_int_type:  return (data.S64_INT OP d.data.S64_INT);                        \
    case u64_int_type:  return (data.U64_INT OP d.data.U64_INT);                        \
    case float_type:    return (data.FLOAT OP d.data.FLOAT);                            \
    case double_type:   return (data.DOUBLE OP d.data.DOUBLE);                          \
    case bool_type:     return (data.BOOL OP d.data.BOOL);                              \
    case date_type:     return (data.DATE OP d.data.DATE);                              \
    case time_type:     return (data.TIME OP d.data.TIME);                              \
    case enum_type:     return (*data.ENUMERATION OP *d.data.ENUMERATION);              \
    case class_type:    return (data.CLASS->get_name() OP d.data.CLASS->get_name());    \
    case object_type:                                                                   \
    case uid_type:                                                                      \
    case uid2_type:                                                                     \
                        return CMP_OBJ ( OP, __cn(), d.__cn(), __oid(), d.__oid() );    \
    default:            return false;                                                   \
  }


bool OksData::is_le(const OksData &d) const noexcept
{
  CMP_DATA( <= )
}

bool OksData::is_ge(const OksData &d) const noexcept
{
  CMP_DATA( >= )
}

bool OksData::is_l(const OksData &d) const noexcept
{
  CMP_DATA( < )
}

bool OksData::is_g(const OksData &d) const noexcept
{
  CMP_DATA( > )
}


bool
OksData::operator<=(const OksData& d) const {
  if(test_comparable(type, d.type) == false) return false;
  return is_le(d);
}


bool
OksData::operator>=(const OksData& d) const {
  if(test_comparable(type, d.type) == false) return false;
  return is_ge(d);
}


bool
OksData::operator>(const OksData& d) const {
  if(test_comparable(type, d.type) == false) return false;
  return is_g(d);
}


bool
OksData::operator<(const OksData& d) const {
  if(test_comparable(type, d.type) == false) return false;
  return is_l(d);
}


void
OksData::copy(const OksData & d)
{
  type = d.type;

  if(type == list_type) {
    data.LIST = new List();

    for(List::iterator i = d.data.LIST->begin(); i != d.data.LIST->end(); ++i) {
      OksData *d2 = new OksData();
      *d2 = *(*i);
      data.LIST->push_back(d2);
    }
  }
  else if(type == string_type) data.STRING = new OksString(*d.data.STRING);
  else if(type == object_type) data.OBJECT = d.data.OBJECT;  // Don't allow deep copy for objects
  else if(type == u32_int_type) data.U32_INT = d.data.U32_INT;
  else if(type == s32_int_type) data.S32_INT = d.data.S32_INT;
  else if(type == u16_int_type) data.U16_INT = d.data.U16_INT;
  else if(type == s16_int_type) data.S16_INT = d.data.S16_INT;
  else if(type == double_type) data.DOUBLE = d.data.DOUBLE;
  else if(type == float_type) data.FLOAT = d.data.FLOAT;
  else if(type == s8_int_type) data.S8_INT = d.data.S8_INT;
  else if(type == u8_int_type) data.U8_INT = d.data.U8_INT;
  else if(type == s64_int_type) data.S64_INT = d.data.S64_INT;
  else if(type == u64_int_type) data.U64_INT = d.data.U64_INT;
  else if(type == class_type) data.CLASS = d.data.CLASS;
  else if(type == uid2_type) {
    data.UID2.class_id  = new OksString(*d.data.UID2.class_id);
    data.UID2.object_id  = new OksString(*d.data.UID2.object_id);
  }
  else if(type == uid_type) {
    data.UID.class_id = d.data.UID.class_id;
    data.UID.object_id = new OksString(*d.data.UID.object_id);
  }
  else if(type == bool_type) data.BOOL = d.data.BOOL;
  else if(type == date_type) data.DATE = d.data.DATE;
  else if(type == time_type) data.TIME = d.data.TIME;
  else if(type == enum_type) data.ENUMERATION = d.data.ENUMERATION;
}


std::ostream&
operator<<(std::ostream& s, const OksData & d)
{
  switch (d.type) {
    case OksData::list_type: {
      s << '(';
      for(OksData::List::iterator i = d.data.LIST->begin(); i != d.data.LIST->end(); ++i) {
        s << *(*i);
        if((*i) != d.data.LIST->back()) s << ", ";
      }
      s << ')';
      break; }

    case OksData::string_type:
      s << '\"' << *(d.data.STRING) << '\"';
      break;

    case OksData::object_type:
      if(d.data.OBJECT)
        s << '[' << d.data.OBJECT->GetId() << '@' << d.data.OBJECT->GetClass()->get_name() << ']';
      else
        s << "[NIL]";

      break;

    case OksData::uid2_type:
      s << '#' << '[' << *d.data.UID2.object_id << '@' << *d.data.UID2.class_id << ']';
      break;

    case OksData::uid_type:
      s << '#' << '[' << *d.data.UID.object_id << '@' << d.data.UID.class_id->get_name() << ']';
      break;

    case OksData::s32_int_type:
      s << d.data.S32_INT;
      break;

    case OksData::u32_int_type:
      s << d.data.U32_INT;
      break;

    case OksData::s16_int_type:
      s << d.data.S16_INT;
      break;

    case OksData::u16_int_type:
      s << d.data.U16_INT;
      break;

    case OksData::float_type: {
      std::streamsize p = s.precision();
      s.precision(std::numeric_limits< float >::digits10);
      s << d.data.FLOAT;
      s.precision(p);
      break; }

    case OksData::double_type: {
      std::streamsize p = s.precision();
      s.precision(std::numeric_limits< double >::digits10);
      s << d.data.DOUBLE;
      s.precision(p);
      break; }

    case OksData::s8_int_type:
      s << static_cast<int16_t>(d.data.S8_INT);
      break;

    case OksData::u8_int_type:
      s << static_cast<uint16_t>(d.data.U8_INT);
      break;

    case OksData::s64_int_type:
      s << d.data.S64_INT;
      break;

    case OksData::u64_int_type:
      s << d.data.U64_INT;
      break;

    case OksData::bool_type:
      s << ( (d.data.BOOL == true) ? "true" : "false" );
      break;

    case OksData::date_type:
      s << boost::gregorian::to_simple_string(d.date());
      break;

    case OksData::time_type:
      s << boost::posix_time::to_simple_string(d.time());
      break;

    case OksData::enum_type:
      s << '\"' << *(d.data.ENUMERATION) << '\"';
      break;

    case OksData::class_type:
      s << '[' << d.data.CLASS->get_name() << ']';
      break;

    case OksData::unknown_type:
      Oks::error_msg("operator<<(ostream&, const OksData&)")
        << "Can't put to stream \'OksData::unknown_type\'";
      break;
  }

  return s;
}


void
OksData::Clear()
{
  if(type >= string_type) {
    switch(type) {
      case list_type:
        if(List * dlist = data.LIST) {
          if(!dlist->empty()) {
            free_list(*dlist);
          }
          delete dlist;
        }
        break;

      case string_type:
        if(data.STRING) { delete data.STRING; }
        break;

      case uid2_type:
        if(data.UID2.object_id) { delete data.UID2.object_id; }
        if(data.UID2.class_id) { delete data.UID2.class_id; }
        break;

      case uid_type:
        if(data.UID.object_id) { delete data.UID.object_id; }
        break;

      default:
        /* Make compiler happy */
        break;
    }
  }

  type = unknown_type;
}


namespace oks {

  std::string
  AttributeRangeError::fill(const OksData * d, const std::string& range)
  {
    std::ostringstream s;
    s << "value ";
    if (d->type == OksData::string_type || d->type == OksData::enum_type)
      s << *d;
    else
      s << '\'' << *d << '\'';
    s << " is out of range \'" << range << '\'';
    return s.str();
  }
  
  std::string
  AttributeReadError::fill(const char * value, const char * type, const std::string& reason)
  {
    std::ostringstream s;

    if(value) { s << "string \'" << value << '\''; }
    else      { s << "empty string"; }

    s << " is not a valid value for \'" << type << "\' type";

    if(!reason.empty()) s << ":\n" << reason;

    return s.str();
  }

  std::string
  AttributeReadError::fill(const char * type, const std::string& reason)
  {
    std::ostringstream s;

    s << "the string is not a valid value for \'" << type << "\' type";

    if(!reason.empty()) s << ":\n" << reason;

    return s.str();
  }

}


void
OksData::check_range(const OksAttribute * a) const
{
  if (a->p_range_obj == nullptr)
    return;

  if (type == list_type)
    {
      if (data.LIST)
        {
          for (const auto& i : *data.LIST)
            {
              i->check_range(a);
            }
        }
    }
  else
    {
      if (a->p_range_obj->validate(*this) == false)
        {
          throw oks::AttributeRangeError(this, a->get_range());
        }
    }
}

void
OksData::SetNullValue(const OksAttribute * a)
{
  switch(type) {
    case s8_int_type:  data.S8_INT      = 0;                                        return;
    case u8_int_type:  data.U8_INT      = 0;                                        return;
    case bool_type:    data.BOOL        = false;                                    return;
    case u32_int_type: data.U32_INT     = 0L;                                       return;
    case s32_int_type: data.S32_INT     = 0L;                                       return;
    case s16_int_type: data.S16_INT     = 0;                                        return;
    case u16_int_type: data.U16_INT     = 0;                                        return;
    case s64_int_type: data.S64_INT     = (int64_t)(0);                             return;
    case u64_int_type: data.U64_INT     = (uint64_t)(0);                            return;
    case float_type:   data.FLOAT       = (float)(.0);                              return;
    case double_type:  data.DOUBLE      = (double)(.0);                             return;
    case string_type:  data.STRING      = new OksString();                          return;
    case enum_type:    data.ENUMERATION = &((*(a->p_enumerators))[0]);              return;  // first value
    case class_type:   data.CLASS       = a->p_class;                               return;  // some class
    case date_type:    SetFast(boost::gregorian::day_clock::universal_day());       return;
    case time_type:    SetFast(boost::posix_time::second_clock::universal_time());  return;
    default:           Clear2(); throw oks::AttributeReadError("", "non-attribute-type", "internal OKS error");
  }
}


void
OksData::SetValue(const char * s, const OksAttribute * a)
{
  if(*s == '\0') {
    SetNullValue(a);
  }
  else {
    switch(type) {
      case s8_int_type:  data.S8_INT      = static_cast<int8_t>(strtol(s, 0, 0));     return;
      case u8_int_type:  data.U8_INT      = static_cast<uint8_t>(strtoul(s, 0, 0));   return;
      case bool_type:    switch(strlen(s)) {
                           case 4: data.BOOL = (oks::cmp_str4n(s, "true") || oks::cmp_str4n(s, "TRUE") ||  oks::cmp_str4n(s, "True")); return;
                           case 1: data.BOOL = (*s == 't' || *s == 'T' || *s == '1'); return;
                           default: data.BOOL = false; return;
                         }
      case u32_int_type: data.U32_INT     = static_cast<uint32_t>(strtoul(s, 0, 0));  return;
      case s32_int_type: data.S32_INT     = static_cast<int32_t>(strtol(s, 0, 0));    return;
      case s16_int_type: data.S16_INT     = static_cast<int16_t>(strtol(s, 0, 0));    return;
      case u16_int_type: data.U16_INT     = static_cast<uint16_t>(strtoul(s, 0, 0));  return;
      case s64_int_type: data.S64_INT     = static_cast<int64_t>(strtoll(s, 0, 0));   return;
      case u64_int_type: data.U64_INT     = static_cast<uint64_t>(strtoull(s, 0, 0)); return;
      case float_type:   data.FLOAT       = strtof(s, 0);                             return;
      case double_type:  data.DOUBLE      = strtod(s, 0);                             return;

      case string_type:  data.STRING      = new OksString(s);                         return;
      case enum_type:    try {
                           data.ENUMERATION = a->get_enum_value(s, strlen(s));        return;
                         }
                         catch(std::exception& ex) {
                           throw oks::AttributeReadError(s, "enum", ex.what());
                         }
      case date_type:    try {
                           if(strchr(s, '-') || strchr(s, '/')) {
                             SetFast(boost::gregorian::from_simple_string(s));        return;
                           }
                           else {
                             SetFast(boost::gregorian::from_undelimited_string(s));   return;
                           }
                         }
                         catch(std::exception& ex) {
                           throw oks::AttributeReadError(s, "date", ex.what());
                         }
      case time_type:    try {
                           if(strlen(s) == 15 && s[8] == 'T') {
                             SetFast(boost::posix_time::from_iso_string(s));          return;
                           }
                           else {
                             SetFast(boost::posix_time::time_from_string(s));         return;
                           }
                         }
                         catch(std::exception& ex) {
                           throw oks::AttributeReadError(s, "time", ex.what());
                         }
      case class_type:   if(OksClass * c = a->p_class->get_kernel()->find_class(s)) {
                           data.CLASS = c;                                            return;
                         }
                         else {
                           Clear2(); throw oks::AttributeReadError(s, "class", "the value is not a name of valid OKS class");
                         }
      default:           Clear2(); throw oks::AttributeReadError(s, "non-attribute-type", "internal OKS error");
    }
  }

}


std::list<std::string>
OksAttribute::get_init_values() const
{
  std::list<std::string> val;

    // check, if the attribute is multi-value
  if(get_is_multi_values() == true) {
    OksData d;
    d.SetValues(get_init_value().c_str(), this);

    for(OksData::List::const_iterator i = d.data.LIST->begin(); i != d.data.LIST->end(); ++i) {
      val.push_back((*i)->str());
    }
  }

  return val;
}


void
OksData::SetValues(const char *s, const OksAttribute *a)
{
  if( !a->get_is_multi_values() ) {
    ReadFrom(s, a->get_data_type(), a);
    return;
  }

  if(type != list_type) {
    Set(new List());
  }
  else if(!data.LIST->empty()) {
    free_list(*data.LIST);
  }

  if( !s ) return;
  while( *s == ' ' ) s++;
  if( *s == '\0' ) return;

  std::string str(s);

  while( str.length() ) {
    while( str.length() && str[0] == ' ') str.erase(0, 1);
    if( !str.length() ) return;
    char delimeter = (
      str[0] == '\"' ? '\"' :
      str[0] == '\'' ? '\'' :
      str[0] == '`'  ? '`'  :
      ','
    );

    if(delimeter != ',') str.erase(0, 1);
    std::string::size_type p = str.find(delimeter);

    OksData *d = new OksData();
    d->type = a->get_data_type();

    if(d->type == OksData::string_type) {
      d->data.STRING = new OksString(str, p);
    }
    else if(d->type == OksData::enum_type) {
      std::string token(str, 0, p);
      d->data.ENUMERATION = a->get_enum_value(token);  // FIXME, create more efficient using compare(idx, len ,str)
    }
    else {
      std::string token(str, 0, p);
      d->SetValue(token.c_str(), a);
    }

    data.LIST->push_back(d);

    str.erase(0, p);
    if(str.length()) {
      if(delimeter != ',') {
        p = str.find(',');
        if( p == std::string::npos )
          p = str.length();
        else
          p++;
        str.erase(0, p);
      }
      else
        str.erase(0, 1);
    }
  }
}


void
OksData::ReadFrom(const OksRelationship *r) noexcept
{
  if(r->get_high_cardinality_constraint() != OksRelationship::Many) {
    Set((OksObject *)0);
  }
  else {
    Set(new List());
  }
}


void
OksXmlInputStream::get_num_token(char __last)
{
  size_t pos(1);

  try {
    char c = get_first_non_empty();

    if(c == __last) {
      m_cvt_char->m_buf[0] = 0;
      throw std::runtime_error("empty numeric value");
    }

    m_cvt_char->m_buf[0] = c;

    while(true) {
      c = get();

      if(c == __last) {
        m_cvt_char->m_buf[pos] = '\0';
        f->unget();
        return;
      }
      else if(c == ' ' || c == '\n' || c == '\r' || c == '\t') {
        m_cvt_char->m_buf[pos] = '\0';
        return;
      }

      m_cvt_char->realloc(pos);
      m_cvt_char->m_buf[pos++] = c;
    }
  }
  catch(std::exception& ex) {
    m_cvt_char->m_buf[pos] = '\0';
    throw oks::BadFileData(std::string("failed to read numeric value: ") + ex.what(), line_no, line_pos);
  }
}

void
OksData::read(const oks::ReadFileParams& read_params, const OksAttribute * a, /*atype,*/ int32_t num)
{
  if(type != list_type) {
    Set(new List());
  }
  else if(!data.LIST->empty()) {
    free_list(*data.LIST);
  }

  while(num-- > 0) {
    data.LIST->push_back(new OksData(read_params, a));
  }
}


void
OksData::read(const oks::ReadFileParams& read_params, const OksAttribute *a)
{
  const char * read_value="";  // is used for report in case of exception
  char * __sanity;
  OksXmlInputStream& fxs(read_params.s);

  try {
    switch(a->get_data_type()) {
      case string_type:
        read_value = "quoted string";
	{
          size_t len = fxs.get_quoted();
          if(type == string_type && data.STRING) {
            data.STRING->assign(fxs.get_xml_token().m_buf, len);
          }
          else {
            Clear();
            type = string_type;
            data.STRING = new OksString(fxs.get_xml_token().m_buf, len);
          }
	}
        break;

      case s32_int_type:
        read_value = "signed 32 bits integer";
        fxs.get_num_token('<');
        Set(static_cast<int32_t>(strtol(fxs.m_cvt_char->m_buf, &__sanity, 0)));
        if( __builtin_expect((*__sanity != 0 || errno == ERANGE), 0) ) OksXmlInputStream::__throw_strto("strtol", __sanity, fxs.m_cvt_char->m_buf, fxs.line_no, fxs.line_pos);
        break;

      case u32_int_type:
        read_value = "unsigned 32 bits integer";
        fxs.get_num_token('<');
        Set(static_cast<uint32_t>(strtoul(fxs.m_cvt_char->m_buf, &__sanity, 0)));
        if( __builtin_expect((*__sanity != 0 || errno == ERANGE), 0) ) OksXmlInputStream::__throw_strto("strtoul", __sanity, fxs.m_cvt_char->m_buf, fxs.line_no, fxs.line_pos);
        break;

      case s16_int_type:
        read_value = "signed 16 bits integer";
        fxs.get_num_token('<');
        Set(static_cast<int16_t>(strtol(fxs.m_cvt_char->m_buf, &__sanity, 0)));
        if( __builtin_expect((*__sanity != 0 || errno == ERANGE), 0) ) OksXmlInputStream::__throw_strto("strtol", __sanity, fxs.m_cvt_char->m_buf, fxs.line_no, fxs.line_pos);
        break;

      case u16_int_type:
        read_value = "unsigned 16 bits integer";
        fxs.get_num_token('<');
        Set(static_cast<uint16_t>(strtoul(fxs.m_cvt_char->m_buf, &__sanity, 0)));
        if( __builtin_expect((*__sanity != 0 || errno == ERANGE), 0) ) OksXmlInputStream::__throw_strto("strtoul", __sanity, fxs.m_cvt_char->m_buf, fxs.line_no, fxs.line_pos);
        break;

      case s8_int_type:
        read_value = "signed 8 bits integer";
        fxs.get_num_token('<');
        Set(static_cast<int8_t>(strtol(fxs.m_cvt_char->m_buf, &__sanity, 0)));
        if( __builtin_expect((*__sanity != 0 || errno == ERANGE), 0) ) OksXmlInputStream::__throw_strto("strtol", __sanity, fxs.m_cvt_char->m_buf, fxs.line_no, fxs.line_pos);
        break;

      case u8_int_type:
        read_value = "unsigned 8 bits integer";
        fxs.get_num_token('<');
        Set(static_cast<uint8_t>(strtoul(fxs.m_cvt_char->m_buf, &__sanity, 0)));
        if( __builtin_expect((*__sanity != 0 || errno == ERANGE), 0) ) OksXmlInputStream::__throw_strto("strtoul", __sanity, fxs.m_cvt_char->m_buf, fxs.line_no, fxs.line_pos);
        break;

      case s64_int_type:
        read_value = "signed 64 bits integer";
        fxs.get_num_token('<');
        Set(static_cast<int64_t>(strtoll(fxs.m_cvt_char->m_buf, &__sanity, 0)));
        if( __builtin_expect((*__sanity != 0 || errno == ERANGE), 0) ) OksXmlInputStream::__throw_strto("strtoll", __sanity, fxs.m_cvt_char->m_buf, fxs.line_no, fxs.line_pos);
        break;

      case u64_int_type:
        read_value = "unsigned 64 bits integer";
        fxs.get_num_token('<');
        Set(static_cast<uint64_t>(strtoull(fxs.m_cvt_char->m_buf, &__sanity, 0)));
        if( __builtin_expect((*__sanity != 0 || errno == ERANGE), 0) ) OksXmlInputStream::__throw_strto("strtoull", __sanity, fxs.m_cvt_char->m_buf, fxs.line_no, fxs.line_pos);
        break;

      case float_type:
        read_value = "float";
        fxs.get_num_token('<');
        Set(strtof(fxs.m_cvt_char->m_buf, &__sanity));
        if( __builtin_expect((*__sanity != 0 || errno == ERANGE), 0) ) OksXmlInputStream::__throw_strto("strtof", __sanity, fxs.m_cvt_char->m_buf, fxs.line_no, fxs.line_pos);
        break;

      case double_type:
        read_value = "double";
        fxs.get_num_token('<');
        Set(strtod(fxs.m_cvt_char->m_buf, &__sanity));
        if( __builtin_expect((*__sanity != 0 || errno == ERANGE), 0) ) OksXmlInputStream::__throw_strto("strtod", __sanity, fxs.m_cvt_char->m_buf, fxs.line_no, fxs.line_pos);
        break;

      case bool_type:
        read_value = "boolean";
        fxs.get_num_token('<');
        Set(static_cast<bool>(strtol(fxs.m_cvt_char->m_buf, &__sanity, 0)));
        if( __builtin_expect((*__sanity != 0 || errno == ERANGE), 0) ) OksXmlInputStream::__throw_strto("strtol", __sanity, fxs.m_cvt_char->m_buf, fxs.line_no, fxs.line_pos);
        break;

      case date_type:
        read_value = "quoted date string";
        {
          size_t len = fxs.get_quoted();
	  Set(oks::str2date(fxs.get_xml_token().m_buf, len));
        }
        break;

      case time_type:
        read_value = "quoted time string";
        {
          size_t len = fxs.get_quoted();
	  Set(oks::str2time(fxs.get_xml_token().m_buf, len));
        }
        break;

      case enum_type:
        read_value = "quoted enum string";
        {
          size_t len = fxs.get_quoted();
	  SetE(fxs.get_xml_token().m_buf, len, a);
        }
        break;

      case uid2_type:
        read_value = "two quoted class-name / object-id strings";
        Set(new OksString(), new OksString());
	{
          size_t len = fxs.get_quoted();
	  data.UID2.class_id->assign(fxs.get_xml_token().m_buf, len);
          len = fxs.get_quoted();
	  data.UID2.object_id->assign(fxs.get_xml_token().m_buf, len);
	}
        break;

      case class_type:
        read_value = "quoted class-type string";
        type = class_type;
        fxs.get_quoted();
	SetValue(fxs.get_xml_token().m_buf, a);
        break;

      default:
        type = unknown_type;
        {
          std::ostringstream s;
          s << "Unknown attribute type \"" << (int)a->get_data_type() << "\" (line " << fxs.get_line_no() << ", char " << fxs.get_line_pos() << ')';
          throw std::runtime_error( s.str().c_str() );
        }
    }
  }
  catch (oks::exception & e) {
    throw oks::FailedRead(read_value, e);
  }
  catch (std::exception & e) {
    throw oks::FailedRead(read_value, e.what());
  }
}


void
OksData::read(const OksAttribute *a, const OksXmlValue& value)
{
  const char * read_value="";  // is used for report in case of exception
  char * __sanity;

  try {
    switch(a->get_data_type()) {
      case string_type:
        {
          if(type == string_type && data.STRING) {
            data.STRING->assign(value.buf(), value.len());
          }
          else {
            Clear();
            type = string_type;
            data.STRING = new OksString(value.buf(), value.len());
          }
        }
        break;

      case s32_int_type:
        read_value = "signed 32 bits integer";
        Set(static_cast<int32_t>(strtol(value.buf(), &__sanity, 0)));
        if( __builtin_expect((*__sanity != 0 || errno == ERANGE), 0) ) OksXmlInputStream::__throw_strto("strtol", __sanity, value.buf(), value.line_no(), value.line_pos());
        break;

      case u32_int_type:
        read_value = "unsigned 32 bits integer";
        Set(static_cast<uint32_t>(strtoul(value.buf(), &__sanity, 0)));
        if( __builtin_expect((*__sanity != 0 || errno == ERANGE), 0) ) OksXmlInputStream::__throw_strto("strtoul", __sanity, value.buf(), value.line_no(), value.line_pos());
        break;

      case s16_int_type:
        read_value = "signed 16 bits integer";
        Set(static_cast<int16_t>(strtol(value.buf(), &__sanity, 0)));
        if( __builtin_expect((*__sanity != 0 || errno == ERANGE), 0) ) OksXmlInputStream::__throw_strto("strtol", __sanity, value.buf(), value.line_no(), value.line_pos());
        break;

      case u16_int_type:
        read_value = "unsigned 16 bits integer";
        Set(static_cast<uint16_t>(strtoul(value.buf(), &__sanity, 0)));
        if( __builtin_expect((*__sanity != 0 || errno == ERANGE), 0) ) OksXmlInputStream::__throw_strto("strtoul", __sanity, value.buf(), value.line_no(), value.line_pos());
        break;

      case s8_int_type:
        read_value = "signed 8 bits integer";
        Set(static_cast<int8_t>(strtol(value.buf(), &__sanity, 0)));
        if( __builtin_expect((*__sanity != 0 || errno == ERANGE), 0) ) OksXmlInputStream::__throw_strto("strtol", __sanity, value.buf(), value.line_no(), value.line_pos());
        break;

      case u8_int_type:
        read_value = "unsigned 8 bits integer";
        Set(static_cast<uint8_t>(strtoul(value.buf(), &__sanity, 0)));
        if( __builtin_expect((*__sanity != 0 || errno == ERANGE), 0) ) OksXmlInputStream::__throw_strto("strtoul", __sanity, value.buf(), value.line_no(), value.line_pos());
        break;

      case s64_int_type:
        read_value = "signed 64 bits integer";
        Set(static_cast<int64_t>(strtoll(value.buf(), &__sanity, 0)));
        if( __builtin_expect((*__sanity != 0 || errno == ERANGE), 0) ) OksXmlInputStream::__throw_strto("strtoll", __sanity, value.buf(), value.line_no(), value.line_pos());
        break;

      case u64_int_type:
        read_value = "unsigned 64 bits integer";
        Set(static_cast<uint64_t>(strtoull(value.buf(), &__sanity, 0)));
        if( __builtin_expect((*__sanity != 0 || errno == ERANGE), 0) ) OksXmlInputStream::__throw_strto("strtoull", __sanity, value.buf(), value.line_no(), value.line_pos());
        break;

      case float_type:
        read_value = "float";
        Set(strtof(value.buf(), &__sanity));
        if( __builtin_expect((*__sanity != 0 || errno == ERANGE), 0) ) OksXmlInputStream::__throw_strto("strtof", __sanity, value.buf(), value.line_no(), value.line_pos());
        break;

      case double_type:
        read_value = "double";
        Set(strtod(value.buf(), &__sanity));
        if( __builtin_expect((*__sanity != 0 || errno == ERANGE), 0) ) OksXmlInputStream::__throw_strto("strtod", __sanity, value.buf(), value.line_no(), value.line_pos());
        break;

      case bool_type:
        read_value = "boolean";
        Set(static_cast<bool>(strtol(value.buf(), &__sanity, 0)));
        if( __builtin_expect((*__sanity != 0 || errno == ERANGE), 0) ) OksXmlInputStream::__throw_strto("strtol", __sanity, value.buf(), value.line_no(), value.line_pos());
        break;

      case date_type:
        read_value = "quoted date string";
        Set(oks::str2date(value.buf(), value.len()));
        break;

      case time_type:
        read_value = "quoted time string";
        Set(oks::str2time(value.buf(), value.len()));
        break;

      case enum_type:
        read_value = "quoted enum string";
        SetE(value.buf(), value.len(), a);
        break;

      case uid2_type:
        read_value = "two quoted class-name / object-id strings";
        {
          std::ostringstream s;
          s << "Unexpected uid2 type at (line " << value.line_no() << ", char " << value.line_pos() << ')';
          throw std::runtime_error( s.str().c_str() );
        }
        break;

      case class_type:
        read_value = "quoted class-type string";
        type = class_type;
        SetValue(value.buf(), a);
        break;

      default:
        type = unknown_type;
        {
          std::ostringstream s;
          s << "Unknown attribute type \"" << (int)a->get_data_type() << "\" (line " << value.line_no() << ", char " << value.line_pos() << ')';
          throw std::runtime_error( s.str().c_str() );
        }
    }
  }
  catch (oks::exception & e) {
    throw oks::FailedRead(read_value, e);
  }
  catch (std::exception & e) {
    throw oks::FailedRead(read_value, e.what());
  }
}

void
OksData::read(const OksAttribute *a, const oks::ReadFileParams& read_params)
{
  if (type != list_type)
    Set(new List());
  else if (!data.LIST->empty())
    free_list(*data.LIST);

  while (true)
    try
      {
        const char * tag_start = read_params.s.get_tag_start();

        // check for closing tag
        if (*tag_start == '/' && oks::cmp_str5n(tag_start+1, OksObject::attribute_xml_tag))
          break;

        if (oks::cmp_str4(tag_start, OksObject::data_xml_tag))
          {
              {
                OksXmlAttribute attr(read_params.s);

                if (oks::cmp_str3(attr.name(), OksObject::value_xml_attribute))
                  {
                    OksXmlValue value(read_params.s.get_value(attr.p_value_len));
                    data.LIST->push_back(new OksData(a, value));
                  }
                else
                  {
                    std::ostringstream s;
                    s << "Unexpected attribute \"" << attr.name() << "\" instead of \"" << OksObject::value_xml_attribute << "\" (line " << read_params.s.get_line_no() << ", char " << read_params.s.get_line_pos() << ')';
                    throw std::runtime_error(s.str().c_str());
                  }
              }
              {
                OksXmlAttribute attr(read_params.s);

                if (oks::cmp_str1(attr.name(), "/") == false)
                  {
                    std::ostringstream s;
                    s << "Unexpected tag \"" << attr.name() << "\" instead of close tag (line " << read_params.s.get_line_no() << ", char " << read_params.s.get_line_pos() << ')';
                    throw std::runtime_error(s.str().c_str());
                  }
              }
          }
        else
          {
            std::ostringstream s;
            s << "Unexpected tag \"" << tag_start << "\" (line " << read_params.s.get_line_no() << ", char " << read_params.s.get_line_pos() << ')';
            throw std::runtime_error(s.str().c_str());
          }
      }
    catch (oks::exception & e)
      {
        throw oks::FailedRead("multi-value", e);
      }
    catch (std::exception & e)
      {
        throw oks::FailedRead("multi-value", e.what());
      }
}

void
OksData::read(const oks::ReadFileParams& read_params, const OksRelationship * r, int32_t num)
{
  if( __builtin_expect((type != list_type), 0) ) {
    Set(new List());
  }
  else if( __builtin_expect((!data.LIST->empty()), 0) ) {
    free_list(*data.LIST);
  }

  while(num-- > 0) {
    OksData * d = new OksData(read_params, r);

      // ignore dangling objects saved previously
    if( __builtin_expect((d->type == OksData::uid2_type && d->data.UID2.class_id->empty()), 0) ) continue;

      // add real object
    data.LIST->push_back(d);
  }
}

void
OksData::read(const oks::ReadFileParams& read_params, const OksRelationship * r)
{
  const OksClass * rel_class(r->p_class_type);   // class of relationship
  OksString * class_id(0);                       // class-name of read object
  const OksClass * c(0);                         // class of read object
  OksXmlInputStream& fxs(read_params.s);

  try {

      // read class name from stream and store it either on 'c' (pointer-to-class) or new OksString

    size_t len = fxs.get_quoted();

    if( __builtin_expect((len > 0), 1) ) {
      if( __builtin_expect((!read_params.alias_table), 1) ) {
        if( __builtin_expect((rel_class != 0), 1) ) {
          c = rel_class->get_kernel()->find_class(fxs.get_xml_token().m_buf);
        }

        if( __builtin_expect((c == 0), 0) ) {
          class_id = new OksString(fxs.get_xml_token().m_buf, len);
        }
      }
      else {
        const char * class_name_str(fxs.get_xml_token().m_buf);

        if(class_name_str[0] == '@') {
          class_name_str++;
          len--;

          c = ( rel_class ? rel_class->get_kernel()->find_class(class_name_str) : 0 );

          if(c) {
            read_params.alias_table->add_value(0, c);
          }
          else {
            read_params.alias_table->add_value(new OksString(class_name_str, len), 0);
          }
        }
        else {
          if(rel_class) {
            if(const OksAliasTable::Value * value = read_params.alias_table->get(class_name_str)) {
              c = (
                value->class_ptr
                  ? value->class_ptr
                  : rel_class->get_kernel()->find_class(*value->class_name)
              );
            }
            else {
              Oks::warning_msg("OksData::read(const oks::ReadFileParams&, const OksRelationship*")
                << "  Can't find alias for class \'" << class_name_str << "\'\n"
                   "  Possibly data file has been saved in old format\n";

              c = rel_class->get_kernel()->find_class(class_name_str);
            }
          }
        }

        if(!c) {
          class_id = new OksString(class_name_str, len);
        }
      }
    }


      // read object id from stream and try to find OksObject

    len = fxs.get_quoted();

    if( __builtin_expect((c == 0), 0) ) {
      if(class_id) {
        Set(class_id, new OksString(fxs.get_xml_token().m_buf, len));
        class_id = 0;
      }
      else {
        Set((OksObject *)0);
      }

      goto final;
    }


    std::string& obj_id((const_cast<oks::ReadFileParams&>(read_params)).tmp);  // use temporal string for fast assignment
    obj_id.assign(fxs.get_xml_token().m_buf, len);
    OksObject * obj = c->get_object(obj_id);

    if(!obj) {
      Set(c, new OksString(obj_id));
      goto final;
    }


      // final checks

    read_params.owner->OksObject::check_class_type(r, c);
    Set(obj);
    obj->add_RCR(read_params.owner, r);
  }
  catch(oks::exception& ex) {
    if(class_id) delete class_id;
    throw oks::FailedRead("relationship value", ex);
  }
  catch (std::exception & ex) {
    if(class_id) delete class_id;
    throw oks::FailedRead("relationship value", ex.what());
  }

final:

  IsConsistent(r, read_params.owner, "WARNING");

}


void
OksData::read(const OksRelationship * r, const OksXmlRelValue& value)
{
  try
    {
      if (__builtin_expect((value.m_class == nullptr), 0))
        {
          if (value.m_class_id)
            {
              Set(value.m_class_id, new OksString(value.m_value.buf(), value.m_value.len()));
              value.m_class_id = nullptr;
            }
          else
            {
              Set((OksObject *) nullptr);
            }

          goto final;
        }

      OksObject * obj = value.m_class->get_object(value.m_value.buf());

      if (!obj)
        {
          Set(value.m_class, new OksString(value.m_value.buf(), value.m_value.len()));
          goto final;
        }

      // final checks

      value.m_file_params.owner->OksObject::check_class_type(r, value.m_class);
      Set(obj);
      obj->add_RCR(value.m_file_params.owner, r);
    }
  catch (oks::exception& ex)
    {
      if (value.m_class_id)
        delete value.m_class_id;
      throw oks::FailedRead("relationship value", ex);
    }
  catch (std::exception & ex)
    {
      if (value.m_class_id)
        delete value.m_class_id;
      throw oks::FailedRead("relationship value", ex.what());
    }

  final:

  IsConsistent(r, value.m_file_params.owner, "WARNING");
}

void
OksData::read(const OksRelationship *r, const oks::ReadFileParams& read_params)
{
  if (type != list_type)
    Set(new List());
  else if (!data.LIST->empty())
    free_list(*data.LIST);

  while (true)
    try
      {
        const char * tag_start = read_params.s.get_tag_start();

        // check for closing tag
        if (*tag_start == '/' && oks::cmp_str4n(tag_start+1, OksObject::relationship_xml_tag))
          break;

        if (oks::cmp_str3(tag_start, OksObject::ref_xml_tag))
          {
            OksXmlRelValue value(read_params);

            try {
              while(true) {
                OksXmlAttribute attr(read_params.s);

                  // check for close of tag

                if(oks::cmp_str1(attr.name(), ">") || oks::cmp_str1(attr.name(), "/")) { break; }

                  // check for known oks-relationship' attributes

                else if(oks::cmp_str5(attr.name(), OksObject::class_xml_attribute)) {
                    value.m_class = read_params.owner->uid.class_id->get_kernel()->find_class(attr.value());
                    if( __builtin_expect((value.m_class == nullptr && attr.value_len() > 0), 0) ) {
                      value.m_class_id = new OksString(attr.value(), attr.value_len());
                    }
                }
                else if(oks::cmp_str2(attr.name(), OksObject::id_xml_attribute)) {
                  value.m_value = read_params.s.get_value(attr.p_value_len);
                }
                else
                  read_params.s.throw_unexpected_attribute(attr.name());
              }

              if(value.is_empty() == false)
                {
                  OksData * d = new OksData(r, value);

                  // ignore dangling objects saved previously
                  if( __builtin_expect((d->type == OksData::uid2_type && d->data.UID2.class_id->empty()), 0) ) continue;

                  // add real object
                  data.LIST->push_back(d);
                }
            }
            catch(oks::exception & e) {
              throw oks::FailedRead("multi-value relationship", e);
            }
            catch (std::exception & e) {
              throw oks::FailedRead("multi-value relationship", e.what());
            }
          }
        else
          {
            std::ostringstream s;
            s << "Unexpected tag \"" << tag_start << "\" (line " << read_params.s.get_line_no() << ", char " << read_params.s.get_line_pos() << ')';
            throw std::runtime_error(s.str().c_str());
          }
      }
    catch (oks::exception & e)
      {
        throw oks::FailedRead("multi-value", e);
      }
    catch (std::exception & e)
      {
        throw oks::FailedRead("multi-value", e.what());
      }
}


    //
    // Writes to stream any OKS data except relationship types
    // (i.e. object, uid, uid2)
    //

void
OksData::WriteTo(OksXmlOutputStream& xmls, bool put_number) const
{
  switch (type) {
    case list_type: {
      if(!data.LIST->empty()) {
        if(put_number) xmls.put_value((unsigned long)data.LIST->size());
        for(List::iterator i = data.LIST->begin(); i != data.LIST->end(); ++i) {
	  if(!put_number) {
	    xmls.put_raw('\n');
	    xmls.put_raw(' ');
	  }
	  
	  xmls.put_raw(' ');

	  (*i)->WriteTo(xmls, put_number);
        }

	if(!put_number)
	  {
	    xmls.put_raw('\n');
	    xmls.put_raw(' ');
	  }
      }
      else {
        if(put_number) xmls.put_raw('0');
      }

      break; }

    case string_type:
      xmls.put_quoted(data.STRING->c_str());
      break;

    case s32_int_type:
      xmls.put_value(data.S32_INT);
      break;

    case u32_int_type:
      xmls.put_value(data.U32_INT);
      break;

    case s16_int_type:
      xmls.put_value(data.S16_INT);
      break;

    case u16_int_type:
      xmls.put_value(data.U16_INT);
      break;

    case float_type: {
      std::ostream& s = xmls.get_stream();
      std::streamsize p = s.precision();
      s.precision(std::numeric_limits< float >::digits10);
      xmls.put_value(data.FLOAT);
      s.precision(p);
      break; }

    case double_type: {
      std::ostream& s = xmls.get_stream();
      std::streamsize p = s.precision();
      s.precision(std::numeric_limits< double >::digits10);
      xmls.put_value(data.DOUBLE);
      s.precision(p);
      break; }

    case s8_int_type:
      xmls.put_value(static_cast<int16_t>(data.S8_INT));
      break;

    case u8_int_type:
      xmls.put_value(static_cast<uint16_t>(data.U8_INT));
      break;

    case s64_int_type:
      xmls.put_value(data.S64_INT);
      break;

    case u64_int_type:
      xmls.put_value(data.U64_INT);
      break;

    case bool_type:
      xmls.put_value((short)(data.BOOL));
      break;

    case date_type:
      xmls.put_quoted(boost::gregorian::to_iso_string(date()).c_str());
      break;

    case time_type:
      xmls.put_quoted(boost::posix_time::to_iso_string(time()).c_str());
      break;

    case enum_type:
      xmls.put_quoted(data.ENUMERATION->c_str());
      break;

    case class_type:
      xmls.put_quoted(data.CLASS->get_name().c_str());
      break;

    case uid2_type:
    case uid_type:
    case object_type:
    case unknown_type:
      Oks::error_msg("OksData::WriteTo(OksXmlOutputStream&, bool)")
        << "Can't write \'" << (type == unknown_type ? "unknown" : "relationship") << "_type\'\n";
      break;
  }
}


void
OksData::WriteTo(OksXmlOutputStream& xmls) const
{
  switch (type) {
    case string_type:
      xmls.put(data.STRING->c_str());
      break;

    case s32_int_type:
      xmls.put_value(data.S32_INT);
      break;

    case u32_int_type:
      xmls.put_value(data.U32_INT);
      break;

    case s16_int_type:
      xmls.put_value(data.S16_INT);
      break;

    case u16_int_type:
      xmls.put_value(data.U16_INT);
      break;

    case float_type: {
      std::ostream& s = xmls.get_stream();
      std::streamsize p = s.precision();
      s.precision(std::numeric_limits< float >::digits10);
      xmls.put_value(data.FLOAT);
      s.precision(p);
      break; }

    case double_type: {
      std::ostream& s = xmls.get_stream();
      std::streamsize p = s.precision();
      s.precision(std::numeric_limits< double >::digits10);
      xmls.put_value(data.DOUBLE);
      s.precision(p);
      break; }

    case s8_int_type:
      xmls.put_value(static_cast<int16_t>(data.S8_INT));
      break;

    case u8_int_type:
      xmls.put_value(static_cast<uint16_t>(data.U8_INT));
      break;

    case s64_int_type:
      xmls.put_value(data.S64_INT);
      break;

    case u64_int_type:
      xmls.put_value(data.U64_INT);
      break;

    case bool_type:
      xmls.put_value((short)(data.BOOL));
      break;

    case date_type:
      xmls.put(boost::gregorian::to_iso_string(date()).c_str());
      break;

    case time_type:
      xmls.put(boost::posix_time::to_iso_string(time()).c_str());
      break;

    case enum_type:
      xmls.put(data.ENUMERATION->c_str());
      break;

    case class_type:
      xmls.put(data.CLASS->get_name().c_str());
      break;

    case list_type:
      throw std::runtime_error("internal error: call OksData::WriteTo() on list");

    case uid2_type:
    case uid_type:
    case object_type:
    case unknown_type:
      Oks::error_msg("OksData::WriteTo(OksXmlOutputStream&, bool)")
        << "Can't write \'" << (type == unknown_type ? "unknown" : "relationship") << "_type\'\n";
      break;
  }
}



    //
    // Writes to XML stream OKS data relationship types
    //

void
OksData::WriteTo(OksXmlOutputStream& xmls, OksKernel *k, OksAliasTable *alias_table, bool /*test_existance*/, bool put_number) const
{
  const char * fname = "WriteTo(OksXmlOutputStream&, ...)";

  static std::string emptyName(""); // to save NIL name

  const std::string * class_name = 0;
  const std::string * object_id = 0;

  switch (type) {
    case object_type:
      if(data.OBJECT) {
        class_name = &data.OBJECT->GetClass()->get_name();
        object_id = &data.OBJECT->GetId();
      }
      else
        class_name = object_id = &emptyName;

      break;

    case list_type: {
      if(!data.LIST->empty()) {
        if(put_number) xmls.put_value((unsigned long)data.LIST->size());

	for(List::iterator i = data.LIST->begin(); i != data.LIST->end(); ++i) {
          if(!put_number) {
            xmls.put_raw('\n');
            xmls.put_raw(' ');
          }

          xmls.put_raw(' ');

	  (*i)->WriteTo(xmls, k, alias_table, false, put_number);
	}

	if(!put_number) {
	  xmls.put_raw('\n');
	  xmls.put_raw(' ');
	}
      }
      else {
        if(put_number) xmls.put_raw('0');
      }

      return; }

    case uid2_type:
      class_name = data.UID2.class_id;
      object_id = data.UID2.object_id;
      break;
		
    case uid_type:
      class_name = &data.UID.class_id->get_name();
      object_id = data.UID.object_id;
      break;

    default:
      Oks::error_msg(fname)
        << "Can't write \'" << (type == unknown_type ? "unknown" : "attribute") << "_type\'\n";
      return;
  }


  if(!class_name->empty()) {

      // if alias table is not defined, store object in extended format

    if(!alias_table) {
      xmls.put_quoted(class_name->c_str());
    }


      // otherwise store object in compact format

    else {
      const OksAliasTable::Value *value = alias_table->get(static_cast<const OksString *>(class_name));

        // class' alias was already defined
      if(value)
        xmls.put_quoted(value->class_name->c_str());

        //makes new class' alias
      else {
        alias_table->add_key(static_cast<OksString *>(const_cast<std::string *>(class_name)));

        std::string s(*class_name);
        s.insert(0, "@");	// add '@' to distinguish alias from class name when load
        xmls.put_quoted(s.c_str());
      }
    }

  }
  else {
    xmls.put_quoted("");
  }

  xmls.put_raw(' ');
  xmls.put_quoted(object_id->c_str());
}

void
OksData::ConvertTo(OksData *to, const OksRelationship *r) const
{
  if(r->get_high_cardinality_constraint() == OksRelationship::Many) {
    to->Set(new List());

    if(type != list_type) {
      OksData *item = new OksData();

      if(type == OksData::object_type)    item->Set(data.OBJECT);
      else if(type == OksData::uid_type)  item->Set(data.UID.class_id, *data.UID.object_id);
      else if(type == OksData::uid2_type) item->Set(*data.UID2.class_id, *data.UID2.object_id);

      to->data.LIST->push_back(item);

      return;
    }
    else if(data.LIST && !data.LIST->empty()){
      for(List::iterator i = data.LIST->begin(); i != data.LIST->end(); ++i) {
        OksData *item = new OksData();
        OksData *dd = *i;

        if(dd->type == OksData::object_type)    item->Set(dd->data.OBJECT);
        else if(dd->type == OksData::uid_type)  item->Set(dd->data.UID.class_id, *dd->data.UID.object_id);
        else if(dd->type == OksData::uid2_type) item->Set(*dd->data.UID2.class_id, *dd->data.UID2.object_id);				

        to->data.LIST->push_back(item);
      }

      return;
    }

  }
  else {
    const OksData * from = ((type == list_type) ? data.LIST->front() : this);

    if(from->type == OksData::object_type)    to->Set(from->data.OBJECT);
    else if(from->type == OksData::uid_type)  to->Set(from->data.UID.class_id, *from->data.UID.object_id);
    else if(from->type == OksData::uid2_type) to->Set(*from->data.UID2.class_id, *from->data.UID2.object_id);
  }
}


void
OksData::cvt(OksData *to, const OksAttribute *a) const
{
  if(a->get_is_multi_values()) {
    to->Set(new List());

    if(type != list_type) {
      OksData *item = new OksData();

      std::string cvts = str();
      item->ReadFrom(cvts.c_str(), a->get_data_type(), a);

      to->data.LIST->push_back(item);

      return;
    }
    else if(data.LIST && !data.LIST->empty()) {
      for(List::iterator i = data.LIST->begin(); i != data.LIST->end(); ++i) {
        OksData *item = new OksData();
        OksData *dd = *i;

        std::string cvts = dd->str();
        item->ReadFrom(cvts.c_str(), a->get_data_type(), a);

        to->data.LIST->push_back(item);
      }

      return;
    }
  }
  else {
    const OksData * from = ((type == list_type) ? (data.LIST->empty() ? 0 : data.LIST->front() ) : this);
    std::string cvts;
    if(from) cvts = from->str();
    to->ReadFrom(cvts.c_str(), a->get_data_type(), a);
  }
}


static std::string
list2str(const OksData::List * l, const OksKernel * k, int base)
{
  std::ostringstream s;

  s << '(';

  if(l) {
    bool is_first = true;

    for(OksData::List::const_iterator i = l->begin(); i != l->end(); ++i) {
      std::string s2 = (k ? (*i)->str(k) : (*i)->str(base));
	  
      if(!s2.empty()) {
        if(is_first == false) { s << ", "; }
        else { is_first = false; }
        s << s2;
      }
    }
  }

  s << ')';

  return s.str();
}



  // only is used for OksData pointing to objects

std::string
OksData::str(const OksKernel * kernel) const
{
  if(type == list_type) {
    return list2str(data.LIST, kernel, 0);
  }

  std::ostringstream s;

  switch (type) {
    case uid2_type:
      s << "#[" << *data.UID2.object_id << '@' << *data.UID2.class_id << ']';
      break;

    case uid_type:
      s << "#[" << *data.UID.object_id << '@' << data.UID.class_id->get_name() << ']';
      break;

    case object_type:
      if(data.OBJECT && !kernel->is_dangling(data.OBJECT) && data.OBJECT->GetClass()->get_name().length()) {
        s << '[' << data.OBJECT->GetId() << '@' << data.OBJECT->GetClass()->get_name() << ']';
      }
      break;

    default:
      std::cerr << "ERROR [OksData::str(const OksKernel *)]: wrong use for such data: " << *this << std::endl;
      return "";
  }

  return s.str();
}


  // is only used for primitive data types

std::string
OksData::str(int base) const
{
  if(type == string_type)     { return *data.STRING;      }
  else if(type == enum_type)  { return *data.ENUMERATION; }
  else if(type == bool_type)  { return (data.BOOL ? "true" : "false"); }
  else if(type == list_type)  { return list2str(data.LIST, 0, base); }
  else if(type == date_type)  { return boost::gregorian::to_simple_string(date()); }
  else if(type == time_type)  { return boost::posix_time::to_simple_string(time()); }
  else if(type == class_type) { return data.CLASS->get_name(); }


  std::ostringstream s;

  if(base) {
    s.setf((base == 10 ? std::ios::dec : base == 16 ? std::ios::hex : std::ios::oct), std::ios::basefield );
    s.setf(std::ios::showbase);
  }

  switch (type) {
    case s8_int_type:
      s << static_cast<int16_t>(data.S8_INT);
      break;

    case u8_int_type:
      s << static_cast<uint16_t>(data.U8_INT);
      break;

    case s16_int_type:
      s << data.S16_INT;
      break;

    case u16_int_type:
      s << data.U16_INT;
      break;

    case s32_int_type:
      s << data.S32_INT;
      break;

    case u32_int_type:
      s << data.U32_INT;
      break;

    case s64_int_type:
      s << data.S64_INT;
      break;

    case u64_int_type:
      s << data.U64_INT;
      break;

    case float_type:
      s.precision(std::numeric_limits< float >::digits10);
      s << data.FLOAT;
      break;

    case double_type:
      s.precision(std::numeric_limits< double >::digits10);
      s << data.DOUBLE;
      break;
    
    default:
      std::cerr << "ERROR [OksData::str(int)]: wrong use for such data: " << *this << std::endl;
      return "";
  }

  return s.str();
}


bool
OksData::IsConsistent(const OksRelationship *r, const OksObject *o, const char *msg)
{
  if(r->get_low_cardinality_constraint() != OksRelationship::Zero) {
    if(
      ( type == OksData::object_type && data.OBJECT == 0 ) ||
      ( type == OksData::uid_type && (!data.UID.object_id || data.UID.object_id->empty() || !data.UID.class_id) ) ||
      ( type == OksData::uid2_type && (!data.UID2.object_id || !data.UID2.class_id || data.UID2.object_id->empty() || data.UID2.class_id->empty()) )
    ) {
      if(o->GetClass()->get_kernel()->get_silence_mode() != true) {
        std::cerr << msg << ": value of \"" << r->get_name() << "\" relationship in object " << o << " must be non-null\n";
      }
      return false;
    }
    else if(type == OksData::list_type && data.LIST->empty()) {
      if(o->GetClass()->get_kernel()->get_silence_mode() != true) {
        std::cerr << msg << ": \"" << r->get_name() << "\" relationship in object " << o << " must contain at least one object\n";
      }
      return false;
    }
  }

  return true;
}



void
OksData::sort(bool ascending)
{
  if (type != list_type || data.LIST->size() < 2)
    return;

  if (ascending)
    data.LIST->sort( []( const OksData* a, const OksData* b ) { return *a < *b; } );
  else
    data.LIST->sort( []( const OksData* a, const OksData* b ) { return *a > *b; } );
}

ERS_DECLARE_ISSUE(
  oks,
  DeprecatedFormat,
  "the file " << file << " contains OKS time data stored in deprecated format \'" << data << "\'. Please refresh it using an oks application. Support for such format will be removed in a future release.",
  ((const char *)file)
  ((const char *)data)
)

namespace oks
{

  boost::posix_time::ptime str2time(const char * value, size_t len, const char * file_name)
  {
    if(len == 15 && value[8] == 'T') {
      try {
        return boost::posix_time::from_iso_string(value);
      }
      catch (std::exception& ex) {
        //throw TimeCvtFailed(value, ex.what());
        throw AttributeReadError(value, "time", ex.what());
      }
    }
    else {
      try {
        oks::Time t(value);
        std::ostringstream text;
        text << std::setfill('0')
          << std::setw(4) << t.year()
          << std::setw(2) << (t.month() + 1)
          << std::setw(2) << t.day()
          << 'T'
          << std::setw(2) << t.hour()
          << std::setw(2) << t.min()
          << std::setw(2) << t.sec();
        TLOG_DEBUG( 1 ) << "parse OKS time: " << t << " => " << text.str() ;

        if(file_name)
          ers::warning(oks::DeprecatedFormat(ERS_HERE, file_name, value));
        else
          Oks::warning_msg("oks str2time") << "The file is using deprecated OKS time format \"" << value << "\"\nPlease refresh it using an oks application\nSupport for such format will be removed in a future release.\n";

        return boost::posix_time::from_iso_string(text.str());
      }
      catch (oks::exception& ex) {
        //throw TimeCvtFailed(value, ex);
        throw AttributeReadError(value, "time", ex);
      }
      catch (std::exception& ex) {
        //throw TimeCvtFailed(value, ex.what());
        throw AttributeReadError(value, "time", ex.what());
      }
    }
  }

  boost::gregorian::date str2date(const char * value, size_t len)
  {
    if(len == 8 && value[2] != '/' && value[3] != '/') {
      try {
        return boost::gregorian::from_undelimited_string(value);
      }
      catch (std::exception& ex) {
        throw AttributeReadError(value, "date", ex.what());
      }
    }
    else {
      try {
        oks::Date t(value);
        std::ostringstream text;
        text << std::setfill('0')
          << std::setw(4) << t.year()
          << std::setw(2) << (t.month() + 1)
          << std::setw(2) << t.day();
        TLOG_DEBUG( 1 ) <<  "parse OKS date: " << t << " => " << text.str() ;

        Oks::warning_msg("oks str2date") << "The file is using deprecated OKS date format \"" << value << "\"\nPlease refresh it using an oks application.\nSupport for such format will be removed in a future release.\n";

        return boost::gregorian::from_undelimited_string(text.str());
      }
      catch (oks::exception& ex) {
        throw AttributeReadError(value, "date", ex);
      }
      catch (std::exception& ex) {
        throw AttributeReadError(value, "date", ex.what());
      }
    }
  }

}
