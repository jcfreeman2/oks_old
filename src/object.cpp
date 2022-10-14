#define _OksBuildDll_

#include "oks/object.hpp"
#include "oks/xml.hpp"
#include "oks/attribute.hpp"
#include "oks/relationship.hpp"
#include "oks/class.hpp"
#include "oks/kernel.hpp"
#include "oks/index.hpp"
#include "oks/profiler.hpp"
#include "oks/cstring.hpp"

#include "oks_utils.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <iostream>
#include <sstream>
#include <stdexcept>

#include "ers/ers.hpp"
#include "logging/Logging.hpp"

  // names of xml tags and attributes (differed for compact and extended files)

const char OksObject::obj_xml_tag []          = "obj";
const char OksObject::obj2_xml_tag []         = "o";
const char OksObject::class_xml_attribute[]   = "class";
const char OksObject::class2_xml_attribute[]  = "c";
const char OksObject::id_xml_attribute[]      = "id";
const char OksObject::id2_xml_attribute[]     = "i";


  // names of xml tags and attributes (update oks::cmp_strN calls, if touch below lines)

const char OksObject::attribute_xml_tag[]     = "attr";
const char OksObject::relationship_xml_tag[]  = "rel";
const char OksObject::name_xml_attribute[]    = "name";
const char OksObject::type_xml_attribute[]    = "type";
const char OksObject::num_xml_attribute[]     = "num";
const char OksObject::value_xml_attribute[]   = "val";

const char OksObject::data_xml_tag[]          = "data";
const char OksObject::ref_xml_tag[]           = "ref";


namespace oks {

  std::string
  FailedReadObject::fill(const OksObject * o, const std::string& what, const std::string& reason)
  {
    std::ostringstream s;
    s << "Failed to read \'" << what << "\' of object " << o << '\n' << reason;
    return s.str();
  }

  std::string
  FailedCreateObject::fill(const OksObject * o, const std::string& reason)
  {
    std::ostringstream s;
    s << "Failed to create object " << o << '\n' << reason;
    return s.str();
  }

  std::string
  FailedSaveObject::fill(const OksObject * o, const std::string& reason)
  {
    std::ostringstream s;
    s << "Failed to save object " << o << '\n' << reason;
    return s.str();
  }

  std::string
  FailedRenameObject::fill(const OksObject * o, const std::string& reason)
  {
    std::ostringstream s;
    s << "Failed to rename object " << o << '\n' << reason;
    return s.str();
  }

  std::string
  FailedDestoyObject::fill(const OksObject * o, const std::string& reason)
  {
    std::ostringstream s;
    s << "Failed to destroy object " << o << '\n' << reason;
    return s.str();
  }

  std::string
  ObjectSetError::fill(const OksObject * obj, bool is_rel, const std::string& name, const std::string& reason)
  {
    std::ostringstream s;
    s << "Failed to set " << (is_rel ? "relationship" : "attribute") << " \'" << name << "\' of object " << obj << '\n' << reason;
    return s.str();
  }

  std::string
  ObjectGetError::fill(const OksObject * obj, bool is_rel, const std::string& name, const std::string& reason)
  {
    std::ostringstream s;
    s << "Failed to get " << (is_rel ? "relationship" : "attribute") << " \'" << name << "\' of object " << obj << '\n' << reason;
    return s.str();
  }

  std::string
  AddRcrError::fill(const OksObject * obj, const std::string& name, const OksObject * p1, const OksObject * p2)
  {
    std::ostringstream s;
    s << "Object " << obj << " cannot be linked with " << p1 << " via exclusive relationship \'" << name << "\' since it is already referenced by object " << p2;
    return s.str();
  }

  std::string
  ObjectInitError::fill(const OksObject * obj, const std::string& why, const std::string& reason)
  {
    std::ostringstream s;
    s << "Failed to init object " << obj << " because " << why << '\n' << reason;
    return s.str();
  }

  std::string
  ObjectBindError::fill(const OksObject * obj, const OksRelationship * rel, const std::string& why, const std::string& reason)
  {
    p_rel = rel;
    std::ostringstream s;
    s << "Failed to bind relationship";
    if(p_rel) { s << " \"" << p_rel->get_name() << '\"'; } else { s << 's'; }
    s << " of object " << obj << " because:\n" << why;
    if(!reason.empty()) s << '\n' << reason;
    return s.str();
  }

}


void
OksObject::set_unique_id()
{ 
  OSK_PROFILING(OksProfiler::ObjectGetFreeObjectId, uid.class_id->get_kernel())

  p_duplicated_object_id_idx = uid.object_id.size();

  std::lock_guard lock(uid.class_id->p_unique_id_mutex);

  std::unique_ptr<char[]> unique_id(new char[p_duplicated_object_id_idx+16]);

  for (unsigned long i = 0; i < 0xFFFFFFFF; ++i)
    {
      sprintf(unique_id.get(), "%s^%lu", uid.object_id.c_str(), i);
      std::string id(unique_id.get());
      if(uid.class_id->get_object(id) == nullptr)
        {
          uid.object_id = id;
          return;
        }
    }
}

void
OksObject::check_ids()
{
  if (std::vector<OksClass *> * cs = uid.class_id->p_inheritance_hierarchy)
    {
      for (std::vector<OksClass *>::const_iterator j = cs->begin(); j != cs->end(); ++j)
        {
          OksObject * o = (*j)->get_object(uid.object_id);
          if (__builtin_expect((o != nullptr), 0))
            {
              std::stringstream e;
              e << "the object " << o << " from file \'" << o->get_file()->get_full_file_name() << "\' "
                  "has ID equal with \'" << uid.object_id << '@' << uid.class_id->get_name() << "\' object and their classes belong to the same inheritance hierarchy "
                  "(the OKS kernel is configured in the \"test-duplicated-objects-within-inheritance-hierarchy\" mode for compatibility with config and DAL tools)";
              throw oks::FailedCreateObject(this, e.str());
            }
        }
    }
}

void
OksObject::init1(const OksFile * fp)
{
  const OksClass * c = uid.class_id;
  const std::string & object_id = uid.object_id;
  const OksKernel * kernel(c->get_kernel());


    // do not allow to create an object of ABSTRACT class

  if( __builtin_expect((c->get_is_abstract()), 0) ) {
    std::ostringstream text;
    text << "the class \'" << c->get_name() << "\' is abstract";
    throw oks::FailedCreateObject(this, text.str());
  }


    // do not allow two objects with equal ID in the same class

  if( __builtin_expect((!object_id.empty()), 1) ) {
    if (kernel->get_test_duplicated_objects_via_inheritance_mode() && !kernel->get_allow_duplicated_objects_mode()) {
      check_ids();
    }

    OksObject * o = c->get_object(object_id);

    if( __builtin_expect((o != nullptr), 0) ) {
      if(kernel->get_allow_duplicated_objects_mode()) {
        set_unique_id();
        if(kernel->get_silence_mode() != true) {
          std::cerr
            << "WARNING [OksObject constructor]:\n"
               "  Cannot create object " << o;
          if(fp) {
            std::cerr << " while reading file \'" << fp->get_full_file_name() << '\'';
          }
          std::cerr
            << "\n"
               "  Skip above problem since OKS kernel is configured in \'ignore-duplicated-objects\' mode\n"
            << "  Create duplicated object with ID = \'" << uid.object_id << "\'\n";
        }
      }
      else {
        std::stringstream e;
        e << "the object was already loaded from file \'" << o->get_file()->get_full_file_name() << '\'';
        throw oks::FailedCreateObject(this, e.str());
      }
    }
  }
  else {
      set_unique_id();
  }

  data = new OksData[c->p_instance_size];
  p_user_data = nullptr;
  p_int32_id = 0;
  p_rcr = nullptr;
  file = kernel->get_active_data();
}

void
OksData::set_init_value(const OksAttribute * a, bool skip_init)
{
  bool si = (a->get_init_value().empty() || skip_init);

  if(a->get_is_multi_values() == false) {
    type = a->get_data_type();
    if(si) {
      SetNullValue(a);
    }
    else {
      if(type == OksData::string_type) {
        data.STRING = new OksString(a->get_init_value());
      }
      else if(type == OksData::enum_type) {
        data.ENUMERATION = a->get_enum_value(a->get_init_value());
      }
      else {
        SetValue(a->get_init_value().c_str(), a);
      }
    }
  }
  else {
    if(si) {
      SetValues("", a);
    }
    else {
      SetValues(a->get_init_value().c_str(), a);
    }
  }
}


void
OksObject::init2(bool skip_init)
{
  int count = 0;
  const OksClass * c = uid.class_id;

  if (c->p_all_attributes)
    {
      if (skip_init == false)
        {
          for (const auto& i : *c->p_all_attributes)
            data[count++] = i->p_init_data;
        }
      else
        {
          for (const auto& i : *c->p_all_attributes)
            data[count++] = i->p_empty_init_data;
        }
    }

  if(c->p_all_relationships) {
    for(const auto& i : *c->p_all_relationships) {
      OksData& d(data[count++]);
      if(i->get_high_cardinality_constraint() != OksRelationship::Many) {
        d.type = OksData::object_type;
        d.data.OBJECT = nullptr;
      }
      else {
        d.type = OksData::list_type;
        d.data.LIST = new OksData::List();
      }
    }
  }
}

void
OksObject::init3(OksClass * c)
{
  if( __builtin_expect((c->p_indices != nullptr), 0) ) {
    for(auto& i : *c->p_indices) i.second->insert(this);
  }

  c->add(this);
  c->p_kernel->define(this);
}

void
OksObject::init3()
{
  init3(const_cast<OksClass *>(uid.class_id));
  create_notify();
}

#define READ_OBJ_HEADER(F1, S1, F2, S2)                                                 \
  while(true) {                                                                         \
    OksXmlAttribute attr(read_params.s);                                                \
    if(oks::cmp_str1(attr.name(), ">") || oks::cmp_str1(attr.name(), "/")) { break; }   \
    else if(F1(attr.name(), S1)) {                                                      \
      std::shared_lock lock(read_params.oks_kernel->p_schema_mutex);                    \
      c = read_params.oks_kernel->find_class(attr.value());                             \
      if( __builtin_expect((c == nullptr), 0) ) {                                       \
        std::ostringstream text;                                                        \
        text << "cannot find class \"" << attr.value() << "\" (line "                   \
             << read_params.s.get_line_no() << ", char "                                \
             << read_params.s.get_line_pos() << ')';                                    \
        throw std::runtime_error( text.str().c_str() );                                 \
      }                                                                                 \
    }                                                                                   \
    else if(F2(attr.name(), S2)) id.assign(attr.value(), attr.value_len());             \
    else {                                                                              \
      read_params.s.throw_unexpected_attribute(attr.name());                            \
    }                                                                                   \
  }


void
oks::ReadFileParams::init()
{
  if(format != 'c') {
    object_tag = OksObject::obj_xml_tag;
    object_tag_len = sizeof(OksObject::obj_xml_tag);
  }
  else {
    object_tag = OksObject::obj2_xml_tag;
    object_tag_len = sizeof(OksObject::obj2_xml_tag);
  }
}

OksObject *
OksObject::read(const oks::ReadFileParams& read_params)
{
    // read 'object' tag header

  try {
    const char * tag_start(read_params.s.get_tag_start());

    if(memcmp(tag_start, read_params.object_tag, read_params.object_tag_len)) {
      if(!oks::cmp_str9(tag_start, "/oks-data")) {
        read_params.s.throw_unexpected_tag(tag_start, read_params.object_tag);
      }
      return 0;
    }
  }
  catch (oks::exception & e) {
    throw oks::FailedRead("start-of-object tag", e);
  }
  catch (std::exception & e) {
    throw oks::FailedRead("start-of-object tag", e.what());
  }


    // read object tag attributes

  OksClass * c(nullptr);
  std::string& id((const_cast<oks::ReadFileParams&>(read_params)).tmp);  // use temporal string for fast assignment


  try {
    if(read_params.format != 'c') {
      READ_OBJ_HEADER(oks::cmp_str5, class_xml_attribute, oks::cmp_str2, id_xml_attribute)
    }
    else {
      READ_OBJ_HEADER(oks::cmp_str1, class2_xml_attribute, oks::cmp_str1, id2_xml_attribute)
    }
  }
  catch(oks::exception & e) {
    throw oks::FailedRead("object header", e);
  }
  catch (std::exception & e) {
    throw oks::FailedRead("object header", e.what());
  }

  if(!c) {
    throw oks::BadFileData("object without class", read_params.s.get_line_no(), read_params.s.get_line_pos());
  }

  try {
    oks::validate_not_empty(id, "object id");
  }
  catch(std::exception& ex) {
    throw oks::FailedRead("oks object", oks::BadFileData(ex.what(), read_params.s.get_line_no(), read_params.s.get_line_pos()));
  }

  if(c && read_params.reload_objects) {
    if(OksObject * o = read_params.reload_objects->pop(c, id)) {
      o->read_body(read_params, true);
      return o;
    }
  }


    // check if this is the end of file

  if(c == nullptr && id.empty()) { return nullptr; }


    // create new object

  OksObject * obj = new OksObject(read_params, c, id);  // can throw exception


    // check if we re-read the database and notify if required

  if(read_params.reload_objects) {
    read_params.reload_objects->created.push_back(obj);
    obj->create_notify();

    if(read_params.oks_kernel->get_verbose_mode()) {
      TLOG_DEBUG(3) << "*** create object " << obj << " while reload data ***";
    }
  }

  return obj;
}


void
OksObject::__report_type_cvt_warning(const oks::ReadFileParams& params, const OksAttribute& a, const OksData&d, OksData::Type type, int32_t num) const
{
  const char * type_str;

  switch (type) {
    case OksData::s8_int_type:     type_str = OksAttribute::s8_int_type  ; break;
    case OksData::u8_int_type:     type_str = OksAttribute::u8_int_type  ; break;
    case OksData::s16_int_type:    type_str = OksAttribute::s16_int_type ; break;
    case OksData::u16_int_type:    type_str = OksAttribute::u16_int_type ; break;
    case OksData::s32_int_type:    type_str = OksAttribute::s32_int_type ; break;
    case OksData::u32_int_type:    type_str = OksAttribute::u32_int_type ; break;
    case OksData::s64_int_type:    type_str = OksAttribute::s64_int_type ; break;
    case OksData::u64_int_type:    type_str = OksAttribute::u64_int_type ; break;
    case OksData::float_type:      type_str = OksAttribute::float_type   ; break;
    case OksData::double_type:     type_str = OksAttribute::double_type  ; break;
    case OksData::bool_type:       type_str = OksAttribute::bool_type    ; break;
    case OksData::class_type:      type_str = OksAttribute::class_type   ; break;
    case OksData::date_type:       type_str = OksAttribute::date_type    ; break;
    case OksData::time_type:       type_str = OksAttribute::time_type    ; break;
    case OksData::string_type:     type_str = OksAttribute::string_type  ; break;
    case OksData::enum_type:       type_str = OksAttribute::enum_type    ; break;
    default:                       type_str = "unknown";
    
  }

  std::lock_guard lock(OksKernel::p_parallel_out_mutex);
  Oks::warning_msg(std::string("Read object \"") + GetId() + '@' + GetClass()->get_name() + '\"')
    << "  file: \"" << params.f->get_full_file_name() << "\" (line: " << params.s.get_line_no() << ", pos: " << params.s.get_line_pos() << ")\n"
       "  convert value from wrong type \"" << (num == -1 ? "single" : "multi") << "-value " << type_str << "\" to \"" << (a.get_is_multi_values() ? "multi" : "single")
    << "-value " << a.get_type() << "\" as required for attribute \"" << a.get_name() << "\"\n"
    << "  converted value: " << d << std::endl;
}

void
OksObject::__report_cardinality_cvt_warning(const oks::ReadFileParams& params, const OksRelationship& r) const
{
  std::lock_guard lock(OksKernel::p_parallel_out_mutex);
  Oks::warning_msg(std::string("Read object \"") + GetId() + '@' + GetClass()->get_name() + '\"')
    << "  file: \"" << params.f->get_full_file_name() << "\" (line: " << params.s.get_line_no() << ", pos: " << params.s.get_line_pos() << ")\n"
       "  mismatch between relationship \"" << r.get_name() << "\" cardinality and its value (save data file with new schema)." << std::endl;
}

static void
__throw_unknown_type(const oks::ReadFileParams& params, const char * atype)
{
  std::ostringstream s;
  s << "Unknown attribute type \"" << atype << '\"';
  throw oks::BadFileData(s.str(), params.s.get_line_no(), params.s.get_line_pos());
}

int32_t
OksObject::__get_num(const oks::ReadFileParams& params)
{
  char * __sanity;
  params.s.get_num_token('\"');
  int32_t num = static_cast<int32_t>(strtol(params.s.m_cvt_char->m_buf, &__sanity, 0));
  if(*__sanity != 0 || errno == ERANGE) OksXmlInputStream::__throw_strto("strtol", __sanity, params.s.m_cvt_char->m_buf, params.s.get_line_no(), params.s.get_line_pos());
  return num;
}


void
OksObject::read_body(const oks::ReadFileParams& read_params, bool re_read)
{
  std::string& i_name((const_cast<oks::ReadFileParams&>(read_params)).tmp);
  bool non_silent_mode(!read_params.oks_kernel->get_silence_mode());  // detect if we need to print errors

  OksData dd;                                         // temporal data
  const OksClass * c = uid.class_id;                  // pointer to object's class

  bool was_updated = false;                           // is used when object is re-read from file
  bool check_re_read = (re_read && read_params.oks_kernel->p_change_object_notify_fn);

  char * __sanity;                                    // used to check mu, token


    // set file from which this object was read

  file = read_params.f;


    // is used by ReadFrom(relationship)

  const_cast<oks::ReadFileParams&>(read_params).owner = this;

  if( __builtin_expect((read_params.format != 'c'), 1) ) {
    OksDataInfo::Map::iterator info_i;
    OksDataInfo	* info(0);


      // are used when object is re-read from file

    std::list<const OksAttribute *> * read_attributes = nullptr;
    std::list<const OksRelationship *> * read_relationships = nullptr;

    if( __builtin_expect((check_re_read), 0) ) {
      if(c->p_all_attributes && !c->p_all_attributes->empty()) {
        read_attributes = new std::list<const OksAttribute *>();
        for(auto& i : *c->p_all_attributes)
          read_attributes->push_back(i);
      }

      if(c->p_all_relationships && !c->p_all_relationships->empty()) {
        read_relationships = new std::list<const OksRelationship *>();
        for(auto& i : *c->p_all_relationships)
          read_relationships->push_back(i);
      }
    }
    else if( __builtin_expect((c != nullptr), 1) ) {
      if( __builtin_expect((re_read == true), 0) ) {
        OksData *d_end = data + c->number_of_all_attributes() + c->number_of_all_relationships();
	for(OksData *di = data; di < d_end; ++di) {
	  di->Clear();
	}
      }

      init2();
    }


      // read object body

    while(true) try {

      const char * tag_start = read_params.s.get_tag_start();

        // check for closing tag

      if(*tag_start == '/' && !memcmp(tag_start + 1, read_params.object_tag, read_params.object_tag_len)) { break; }


	// extra check, if the object is empty

      if(oks::cmp_str3(tag_start, "obj")) {
        read_params.s.seek_position(-5);
        break;
      }


        // read attribute

      else if(oks::cmp_str4(tag_start, attribute_xml_tag)) {
        OksData::Type type(OksData::unknown_type);
        int32_t num = -1; // -1 => no mv-data read
	const OksAttribute * a(0);
	OksXmlValue value;
        bool attr_is_closed(false);

          // read 'oks-attribute' tag attributes

        try {

          bool name_was_read(false);

          while(true) {
            OksXmlAttribute attr(read_params.s);

            // check for close of tag
            if(oks::cmp_str1(attr.name(), ">")) { break; }

            if(read_params.format == 'n' && oks::cmp_str1(attr.name(), "/"))
              {
                attr_is_closed = true;
                break;
              }

            // check for known oks-attribute' attributes
            if(oks::cmp_str4(attr.name(), name_xml_attribute)) {
              name_was_read = true;
              if( __builtin_expect((c != nullptr), 1) ) {
                i_name.assign(attr.value(), attr.value_len());
                info_i = c->p_data_info->find(i_name);
                if( __builtin_expect((info_i == c->p_data_info->end() || info_i->second->attribute == nullptr), 0) ) {
		  if(non_silent_mode) {
                    std::string msg = std::string("Read object \"") + GetId() + '@' + c->get_name() + '\"';
                    std::lock_guard lock(OksKernel::p_parallel_out_mutex);
                    Oks::warning_msg(msg.c_str()) << "  skip attribute \"" << attr.value() << "\", which is not defined in the schema\n";
		  }
		}
		else {
		  info = info_i->second;
		  a = info->attribute;
		}
	      }
	    }
            else if(oks::cmp_str4(attr.name(), type_xml_attribute)) {
              type = OksAttribute::get_data_type(attr.value(),attr.value_len());
              if( __builtin_expect((type == OksData::unknown_type), 0) ) {
                if(
                  attr.value()[0] != 0 &&
                  (
                    (attr.value()[0] == '-' && attr.value_len() != 1) ||
                    (attr.value()[0] != '-')
                  )
                ) {
                  __throw_unknown_type(read_params,attr.value());
                }
              }
            }
            else if(read_params.format != 'n' && oks::cmp_str3(attr.name(), num_xml_attribute)) {
              num = static_cast<int32_t>(strtol(attr.value(), &__sanity, 0));
              if( __builtin_expect((*__sanity != 0 || errno == ERANGE), 0) ) OksXmlInputStream::__throw_strto("strtoul", __sanity, attr.value(), read_params.s.get_line_no(), read_params.s.get_line_pos());
            }
            else if(read_params.format == 'n' && oks::cmp_str3(attr.name(), value_xml_attribute)) {
              value = read_params.s.get_value(attr.p_value_len);
            }
            else {
              read_params.s.throw_unexpected_attribute(attr.name());
            }
          }

          if( __builtin_expect((!name_was_read), 0) ) {
            throw oks::BadFileData("attribute without name", read_params.s.get_line_no(), read_params.s.get_line_pos());
          }

          if (read_params.format == 'n' && value.is_empty() == true)
            num = 0;
        }
        catch(oks::exception & e) {
          throw oks::FailedRead("attribute value header", e);
        }
        catch (std::exception & e) {
          throw oks::FailedRead("attribute value header", e.what());
        }

	// read 'oks-attribute' body
        if( __builtin_expect((a != nullptr), 1) ) {
          OksData * d = &data[info->offset];
          OksData * dx;

          if( __builtin_expect((check_re_read), 0) ) {
            dx = &dd;
	    read_attributes->remove(a);
	  }
          else {
            dx = d;
          }


          // set default type if it was not read from file
          if( __builtin_expect((type == OksData::unknown_type), 0) ) {
            type = a->get_data_type();
          }


          try {

              // read value and convert if necessary

            if(num == -1) {
              if( __builtin_expect((a->get_data_type() != type || a->get_is_multi_values()), 0) ) {
	        OksAttribute a2((type != OksData::enum_type ? type : OksData::string_type), c);

                OksData d2;
	        if(read_params.format != 'n')
                  d2.read(read_params, &a2);
	        else
	          d2.read(&a2, value);
                d2.cvt(dx, a);
                if(non_silent_mode) {
                  __report_type_cvt_warning(read_params, *a, *dx, type, num);
                }
              }
              else {
                if(read_params.format != 'n')
                  dx->read(read_params, a);
                else
                  dx->read(a, value);
              }
            }
            else {
              if( __builtin_expect((a->get_data_type() != type || !a->get_is_multi_values()), 0) ) {
	        OksAttribute a2((type != OksData::enum_type ? type : OksData::string_type), c);
	        OksData d2;
                if(read_params.format != 'n')
                  d2.read(read_params, &a2, num);
                else
                  {
                    if(attr_is_closed == false)
                      d2.read(&a2, read_params);
                    else
                      d2.Set(new OksData::List());
                  }
                d2.cvt(dx, a);
                if(non_silent_mode) {
                  __report_type_cvt_warning(read_params, *a, *dx, type, num);
                }
              }
              else {
                if(read_params.format != 'n')
                  dx->read(read_params, a, num);
                else
                  {
                  if(attr_is_closed == false)
                    dx->read(a, read_params);
                  else
                    dx->Set(new OksData::List());
                  }
              }
            }

              // check validity range

            dx->check_range(a);
          }
          catch (oks::exception & e) {
            throw oks::FailedReadObject(this, std::string("attribute \"") + a->get_name() + '\"', e);
          }
          catch (std::exception & e) {
            throw oks::FailedReadObject(this, std::string("attribute \"") + a->get_name() + '\"', e.what());
          }


	  if( __builtin_expect((check_re_read), 0) ) {
	    if(*dx != *d) {
	      was_updated = true;
              *d = *dx;
	    }
            dd.Clear();
	  }
        }
        else {
	  try {
	    OksAttribute a2((type != OksData::enum_type ? type : OksData::string_type), c);
            if(num == -1) {
              // note, there is no need to read single-value in case of "normal" format
              if(read_params.format != 'n')
                OksData(read_params, &a2);
            }
            else {
              if(read_params.format != 'n')
                OksData(read_params, &a2, num);
              else
                OksData(&a2, read_params);
            }
	  }
          catch (oks::exception & e) {
            throw oks::FailedReadObject(this, "dummu attribute", e);
          }
          catch (std::exception & e) {
            throw oks::FailedReadObject(this, "dummu attribute", e.what());
          }
	}


	  // read 'oks-attribute' close tag

        if(read_params.format != 'n')
          try {
            const char * eoa_tag_start = read_params.s.get_tag_start();

            if(!oks::cmp_str5(eoa_tag_start, "/attr")) {
              std::string s("\'/attr\' is expected instead of \'");
              s += eoa_tag_start;
              s += '\'';
              throw std::runtime_error( s.c_str() );
            }
          }
          catch (oks::exception & e) {
            throw oks::FailedRead(std::string("attribute\'s closing tag"), e);
          }
          catch (std::exception & e) {
            throw oks::FailedRead(std::string("attribute\'s closing tag"), e.what());
          }

      }  // end of "read attribute"
 

        // read relationship

      else if(oks::cmp_str3(tag_start, relationship_xml_tag)) {
        int32_t num = -1; // -1 => no mv-data read
	OksRelationship * r(nullptr);
        OksXmlRelValue value(read_params);

          // read 'oks-relationship' tag attributes

        try {
          while(true) {
            OksXmlAttribute attr(read_params.s);

              // check for close of tag

            if(oks::cmp_str1(attr.name(), ">") || (read_params.format == 'n' && oks::cmp_str1(attr.name(), "/"))) { break; }

              // check for known oks-relationship' attributes

            else if(oks::cmp_str4(attr.name(), name_xml_attribute)) {
              if( __builtin_expect((c != nullptr), 1) ) {
                i_name.assign(attr.value(), attr.value_len());
                info_i = c->p_data_info->find(i_name);
		if( __builtin_expect((info_i == c->p_data_info->end() || info_i->second->relationship == nullptr), 0) ) {
		  if(non_silent_mode) {
                    std::string msg = std::string("Read object \"") + GetId() + '@' + c->get_name() + '\"';
                    std::lock_guard lock(OksKernel::p_parallel_out_mutex);
                    Oks::warning_msg(msg.c_str()) << "  skip relationship \"" << attr.value() << "\", which is not defined in the schema\n";
		  }
		}
		else {
	          info = info_i->second;
                  r = const_cast<OksRelationship *>(info->relationship);
		}
	      }
              continue;
	    }
            else if(read_params.format == 'n') {
              if(oks::cmp_str5(attr.name(), class_xml_attribute)) {
                value.m_class = uid.class_id->get_kernel()->find_class(attr.value());
                if( __builtin_expect((value.m_class == nullptr && attr.value_len() > 0), 0) ) {
                  value.m_class_id = new OksString(attr.value(), attr.value_len());
                }
                continue;
              }
              else if(oks::cmp_str2(attr.name(), id_xml_attribute)) {
                value.m_value = read_params.s.get_value(attr.p_value_len);
                continue;
              }
            }
            else {
              if(oks::cmp_str3(attr.name(), num_xml_attribute)) {
                num = static_cast<int32_t>(strtol(attr.value(), &__sanity, 0));
                if( __builtin_expect((*__sanity != 0 || errno == ERANGE), 0) ) OksXmlInputStream::__throw_strto("strtoul", __sanity, attr.value(), read_params.s.get_line_no(), read_params.s.get_line_pos());
                continue;
              }
            }

            read_params.s.throw_unexpected_attribute(attr.name());
          }
        }
        catch(oks::exception & e) {
          throw oks::FailedRead("relationship value header", e);
        }
        catch (std::exception & e) {
          throw oks::FailedRead("relationship value header", e.what());
        }

        if (read_params.format == 'n' && value.is_empty() == true)
          num = 0;

	  // read 'oks-relationship' body

        if( __builtin_expect((r != nullptr), 1) ) {
          OksData *d = &data[info->offset];
          OksData * dx;

          OksRelationship::CardinalityConstraint cc = r->get_high_cardinality_constraint();

          if( __builtin_expect((check_re_read), 0) ) {
            dx = &dd;
	    read_relationships->remove(r);
	  }
          else {
            dx = d;
          }

          try {
            if(num == -1) {
              if(cc == OksRelationship::Many) {
                __report_cardinality_cvt_warning(read_params, *r);
                OksRelationship dummy(*r);
                OksData d2;
                if(read_params.format == 'n')
                  d2.read(&dummy, value);
                else
                  d2.read(read_params, &dummy);
                d2.ConvertTo(dx, r);
              }
              else {
                if(read_params.format == 'n')
                  dx->read(r, value);
                else
                  dx->read(read_params, r);
              }
            }
            else {
              if(cc != OksRelationship::Many) {
                __report_cardinality_cvt_warning(read_params, *r);
                OksRelationship dummy(*r);
                OksData d2;
                if(read_params.format == 'n')
                  d2.read(&dummy, read_params);
                else
                  d2.read(read_params, &dummy, num);
                d2.ConvertTo(dx, r);
              }
              else {
                if(read_params.format == 'n')
                  dx->read(r, read_params);
                else
                  dx->read(read_params, r, num);
              }
            }
          }
          catch (oks::exception & e) {
            throw oks::FailedReadObject(this, std::string("relationship \"") + r->get_name() + '\"', e);
          }
          catch (std::exception & e) {
            throw oks::FailedReadObject(this, std::string("relationship \"") + r->get_name() + '\"', e.what());
          }

	  if(check_re_read) {
	    if(*dx != *d) {
	      was_updated = true;
	      *d = *dx;
	    }
            dd.Clear();
	  }
        }
        else {
          OksRelationship r("_ _ *** dummy *** _ _"); // temporal relationship to use right OksData constructor call

          try {
            if(num == -1) {
              if(read_params.format != 'n')
                OksData(read_params, &r);
            }
            else {
              if(read_params.format == 'n')
                OksData(&r, read_params);
              else
                OksData(read_params, &r, num);
            }
	  }
          catch (oks::exception & e) {
            throw oks::FailedReadObject(this, "dummu relationship", e);
          }
          catch (std::exception & e) {
            throw oks::FailedReadObject(this, "dummu relationship", e.what());
          }
        }


	  // read 'oks-relationship' close tag

        if(read_params.format != 'n')
          try {
            const char * eor_tag_start = read_params.s.get_tag_start();

            if(!oks::cmp_str4(eor_tag_start, "/rel")) {
              std::string s("\'/rel\' is expected instead of \'");
              s += eor_tag_start;
              s += '\'';
              throw std::runtime_error( s.c_str() );
            }
          }
          catch (oks::exception & e) {
            throw oks::FailedRead(std::string("relationship\'s closing tag"), e);
          }
          catch (std::exception & e) {
            throw oks::FailedRead(std::string("relationship\'s closing tag"), e.what());
          }

      }  // end of "read relationship"

    }

    catch (oks::exception & e) {
      throw oks::FailedRead(std::string("object \"") + uid.object_id + '@' + c->get_name() + '\"', e);
    }
    catch (std::exception & e) {
      throw oks::FailedRead(std::string("object \"") + uid.object_id + '@' + c->get_name() + '\"', e.what());
    }


      // check that unread attributes & relationships have default values

    if( __builtin_expect((check_re_read), 0) ) {
      if(read_attributes) {
        if(!read_attributes->empty()) {
	  for(auto i = read_attributes->begin(); i != read_attributes->end(); i = read_attributes->erase(i)) {
            OksData d = (*i)->p_init_data;
            try {
	      OksData *d2(GetAttributeValue((*i)->get_name()));
	      if(*d2 != d) {
	        *d2 = d;
	        was_updated = true;
	      }
            }
            catch(oks::exception& ex) {
              throw oks::ObjectInitError(this, std::string("cannot reset default value of attribute \"") + (*i)->get_name() + '\"', ex);
            }
	  }
	}

        delete read_attributes;
      }

      if(read_relationships) {
        if(!read_relationships->empty()) {
	  for(auto i = read_relationships->begin(); i != read_relationships->end(); i = read_relationships->erase(i)) {
	    OksData d; d.ReadFrom(*i);
            try {
	      OksData *d2(GetRelationshipValue((*i)->get_name()));
	      if(d2 && *d2 != d) {
	        *d2 = d;
	        was_updated = true;
	      }
            }
            catch(oks::exception& ex) {
              throw oks::ObjectInitError(this, std::string("cannot reset value of relationship \"") + (*i)->get_name() + '\"', ex);
            }
	  }
	}

        delete read_relationships;
      }
    }
  }
  else {
    size_t count(0);

    if(c == 0) {
      throw std::runtime_error(
        "  *****     Can't read objects of undefined class     *****\n"
        "  *****    from file saved in non-extended format.    *****\n"
        "  ***** Fix the problem first. Processing terminated. *****\n"
      );
    }

    if(c->p_all_attributes) {
      for(const auto& i : *c->p_all_attributes) {
        if(check_re_read) {
          OksData d;

          if(i->get_is_multi_values()) {
            d.read(read_params, i, __get_num(read_params));
          }
          else {
            d.read(read_params, i);
          }

	  if(d != data[count]) {
	    data[count] = d;
	    was_updated = true;
	  }
	}
	else {
          if(i->get_is_multi_values()) {
            data[count].read(read_params, i, __get_num(read_params));
          }
          else {
            data[count].read(read_params, i);
          }
	}

	count++;
      }
    }

    if(c->p_all_relationships) {
      for(const auto& i : *c->p_all_relationships) {
        if(check_re_read) {
          OksData d;

          if(i->get_high_cardinality_constraint() == OksRelationship::Many) {
            d.read(read_params, i, __get_num(read_params));
          }
          else {
            d.read(read_params, i);
          }

          if(d != data[count]) {
            data[count] = d;
            was_updated = true;
          }
	}
	else {
          if(i->get_high_cardinality_constraint() == OksRelationship::Many) {
            data[count].read(read_params, i, __get_num(read_params));
          }
          else {
            data[count].read(read_params, i);
          }
	}

	count++;
      }
    }

      // read tag closing object

    try {
      const char * tag_start = read_params.s.get_tag_start();
    
      if(*tag_start != '/' || memcmp(tag_start + 1, read_params.object_tag, read_params.object_tag_len)) {
        std::ostringstream s;
	s << "Failed to read obj close tag (\'/" << read_params.object_tag << "\' is expected)";
        throw std::runtime_error( s.str().c_str() );
      }
    }
    catch (oks::exception & e) {
      throw oks::FailedRead("object close tag", e);
    }
    catch (std::exception & e) {
      throw oks::FailedRead("object close tag", e.what());
    }
  }

  if(was_updated) change_notify();
}

OksObject::OksObject(const oks::ReadFileParams& read_params, OksClass * c, const std::string& id) : data(nullptr), p_duplicated_object_id_idx(-1)
{
  if(c) {
    OSK_PROFILING(OksProfiler::ObjectStreamConstructor, c->p_kernel)
  }

  uid.object_id = id;
  uid.class_id = c;

  std::shared_lock lock(read_params.oks_kernel->p_schema_mutex);  // protect schema and all objects from changes

  if(c) {
    init1(read_params.f);  // can throw exception; to be caught by caller (i.e. OksKernel)
  }

  read_body(read_params, false);

  if(c) {
    init3(c);
  }
}


OksObject::OksObject(const OksClass* c, const char *object_id, bool skip_init) : data(nullptr), p_duplicated_object_id_idx(-1)
{
  OSK_PROFILING(OksProfiler::ObjectNormalConstructor, c->p_kernel)

  if(!c->p_kernel->get_active_data()) {
    Oks::error_msg("OksObject::OksObject(const OksClass*, const char *)") << "  Can not create new object since there is no active data file\n";
    return;
  }

  uid.class_id = c;
  uid.object_id = (object_id ? object_id : "");

  try {
    init1();
  }
  catch(oks::exception& ex) {
    uid.class_id = nullptr;
    throw;
  }

  init2(skip_init);
  init3();

  file->set_updated();
}

OksObject::OksObject(const OksObject& parentObj, const char *object_id) : data(nullptr), p_duplicated_object_id_idx(-1)
{
  OSK_PROFILING(OksProfiler::ObjectCopyConstructor, parentObj.uid.class_id->p_kernel)

  if(!parentObj.uid.class_id->p_kernel->get_active_data()) {
    Oks::error_msg("OksObject::OksObject(const OksObject&, const char *)") << "  Can not create new object since there is no active data file\n";
    return;
  }

  const OksClass *c = uid.class_id = parentObj.uid.class_id;

  uid.object_id = (object_id ? object_id : "");

  try {
    init1();
  }
  catch(oks::exception& ex) {
    uid.class_id = nullptr;
    throw;
  }

  
  int count	   = c->p_instance_size;
  int num_of_attr = c->p_all_attributes->size();
  int j	   = 0;

  while(j < num_of_attr) {
    data[j] = parentObj.data[j];
    j++;
  }

  try {
    std::list<OksRelationship *>::iterator i = c->p_all_relationships->begin();

    while(j < count) {
      const OksRelationship * r = *i;
      const char * relationshipName = r->get_name().c_str();

      if(parentObj.data[j].type == OksData::list_type) {
        data[j].Set(new OksData::List());
  		
        if(parentObj.data[j].data.LIST) {
          for(const auto& i2 : *parentObj.data[j].data.LIST) {
	    if(i2->type == OksData::object_type)
              AddRelationshipValue(relationshipName, i2->data.OBJECT);
	    else {
	      OksData * uid_d = new OksData();
	      uid_d = i2;
	      data[j].data.LIST->push_back(uid_d);
	    }
	  }
        }
      }
      else {
        if(parentObj.data[j].type == OksData::object_type) {
          data[j].Set((OksObject *)0);
          SetRelationshipValue(relationshipName, parentObj.data[j].data.OBJECT);
        }
        else
          data[j] = parentObj.data[j];
      }
      ++j;
      ++i;
    }
  }
  catch(oks::exception& ex) {
    uid.class_id = nullptr;
    throw;
  }
  
  init3();

  file->set_updated();
}

OksObject::OksObject(size_t offset, const OksData *d) : data(nullptr), p_duplicated_object_id_idx(-1)
{
  uid.class_id = nullptr;
  data = new OksData[offset+1];
  memcpy(static_cast<void *>(&data[offset]), static_cast<const void *>(d), sizeof(OksData));
}

OksObject::OksObject(OksClass * c, const std::string& id, void * user_data, int32_t int32_id, int32_t duplicated_object_id_idx, OksFile * f) :
  data            (new OksData[c->p_instance_size]),
  p_user_data     (user_data),
  p_int32_id      (int32_id),
  p_duplicated_object_id_idx (duplicated_object_id_idx),
  file            (f)
  
{
  uid.class_id = c;
  uid.object_id = id;

    // set OksData type to unknown
  if(size_t data_size = c->p_instance_size * sizeof(OksData*)) {
    bzero(data, data_size);
  }
}

void
OksObject::destroy(OksObject *o, bool fast)
{
  if(!o->uid.class_id || fast) {
    delete o;
    return;
  }

    // check lack of references on object

  std::unique_ptr<std::ostringstream> error_text;

  const OksClass::Map & classes = o->uid.class_id->p_kernel->classes();

  for(OksClass::Map::const_iterator i = classes.begin(); i != classes.end(); ++i) {
    OksClass *c(i->second);
    if(c->p_all_relationships == nullptr || c->p_all_relationships->empty()) continue;

    if(const OksObject::Map * objects = c->objects()) {
      for(OksObject::Map::const_iterator j = objects->begin(); j != objects->end(); ++j) {
        size_t offset = c->number_of_all_attributes();
	for(std::list<OksRelationship *>::iterator k = c->p_all_relationships->begin(); k != c->p_all_relationships->end(); ++k, ++offset) {
	  OksData * d = j->second->data + offset;
	  OksObject * ref = nullptr;

	  if(d->type == OksData::object_type && d->data.OBJECT == o) {
	    ref = j->second;
	  }
	  else if(d->type == OksData::list_type && d->data.LIST != nullptr) {
	    for(const auto& l : *d->data.LIST) {
	      if(l->type == OksData::object_type && l->data.OBJECT == o) {
	        ref = j->second;
		break;
	      }
	    }
	  }

	  if(ref) {
            if(!error_text.get()) {
              error_text.reset(new std::ostringstream());
              *error_text << "since it is referenced by:";
            }
            *error_text << "\n   * object " << ref << " via relationship \"" << (*k)->get_name() << '\"';
	  }
	}
      }
    }
  }

  if(error_text.get()) {
    throw oks::FailedDestoyObject(o, error_text->str().c_str());
  }

  try {
    o->file->lock();
    o->file->set_updated();
  }
  catch(oks::exception& ex) {
    throw oks::FailedDestoyObject(o, ex);
  }

  delete o;
}

OksObject::~OksObject()
{
  OksClass * c = const_cast<OksClass *>(uid.class_id);
  
  if(!c) {
    if(data) {
      delete [] data;
    }
  }
  else {
    OksKernel * k = c->p_kernel;

    OSK_PROFILING(OksProfiler::ObjectDestructor, k)

      TLOG_DEBUG(4) << "destroy object " << this;

    static std::mutex s_mutex;
    static std::set<OksObject *, std::less<OksObject *> > oset;

    if(k->p_close_all == false) {
      k->undefine(this);
      std::lock_guard lock(s_mutex);
      oset.insert(this);
    }

    delete_notify();

    if(data) {
      if(c->p_all_relationships && !c->p_all_relationships->empty()) {
        OksData *di = data + c->number_of_all_attributes();

        for(std::list<OksRelationship *>::iterator i = c->p_all_relationships->begin(); i != c->p_all_relationships->end(); ++i, ++di) {
          OksRelationship * r = *i;

          if(r->get_is_composite() == false || k->p_close_all == true) continue;

          OksData * d = di;

          if(d->type == OksData::object_type && d->data.OBJECT) {
            OksObject * o = d->data.OBJECT;

            if(!k->is_dangling(o)) {
              o->remove_RCR(this, r);

              bool delete_obj(r->get_is_dependent() && !o->p_rcr);

              if(delete_obj) {
                std::lock_guard lock(s_mutex);
                delete_obj = (oset.find(o) == oset.end());
              }

              if( delete_obj ) {
                delete o;
                d->data.OBJECT = nullptr;
              }
            }
          }
          else if(d->type == OksData::list_type && d->data.LIST) {
            for(const auto& i2 : *d->data.LIST) {
              if(i2->type == OksData::object_type && i2->data.OBJECT) {
                OksObject * o = i2->data.OBJECT;

                if(!k->is_dangling(o)) {
                  o->remove_RCR(this, r);

                  bool delete_obj(r->get_is_dependent() && !o->p_rcr);

                  if(delete_obj) {
                    std::lock_guard lock(s_mutex);
                    delete_obj = (oset.find(o) == oset.end());
                  }

                  if( delete_obj ) {
                    delete o;
                    i2->data.OBJECT = nullptr;
                  }
                }
              }
            }
          }
        }
      }

      if(c->p_indices) {
        for(const auto& i : *c->p_indices)
          i.second->remove_obj(this);
      }

      int count = c->p_instance_size;
      while(count--) data[count].Clear();

      delete[] data;

      if(k->p_close_all == false) {
        try {
          c->remove(this);
        }
        catch(oks::exception& ex) {
          Oks::error_msg("OksObject::~OksObject()") << ex.what() << std::endl;
        }
      }
    }

    if(p_rcr) {
      for(std::list<OksRCR *>::const_iterator i = p_rcr->begin(); i != p_rcr->end(); ++i) {
        delete *i;
      }

      delete p_rcr;
    }

    if(k->p_close_all == false) {
      std::lock_guard lock(s_mutex);
      oset.erase(this);
    }
  }
}


void
OksObject::set_file(OksFile * f, bool update_owner)
{
  if(file != f) {
    try {
      if(update_owner) {
        file->lock();
        file->set_updated();
      }

      f->lock();
      f->set_updated();

      file = f;
    }
    catch(oks::exception& ex) {
      throw oks::CanNotSetFile(0, this, *f, ex);
    }
  }
}


void
OksObject::set_id(const std::string &new_id)
{
  if(new_id == uid.object_id) return;

  try {
    oks::validate_not_empty(new_id, "object id");
  }
  catch(std::exception& ex) {
    throw oks::FailedRenameObject(this, ex.what());
  }

  OksClass *c = const_cast<OksClass *>(uid.class_id);

  if(c->get_object(new_id)) {
    throw oks::FailedRenameObject(this, "the object with such identity already exists");
  }


    // build set of updated files and list of updated objects

  std::set<OksFile *> updated_files;
  std::list<OksObject *> updated_objects;

  updated_files.insert(file);
  updated_objects.push_back(this);

  const OksClass::Map & classes = GetClass()->get_kernel()->classes();

  for(OksClass::Map::const_iterator i = classes.begin(); i != classes.end(); ++i) {
    const OksClass * c(i->second);
    const OksObject::Map * objs = c->objects();
    size_t d_end = c->p_instance_size;
    size_t d_begin = c->p_all_attributes->size();
    if(objs && (d_end != d_begin)) {
      for(OksObject::Map::const_iterator j = objs->begin(); j != objs->end(); ++j) {
        OksObject * o(j->second);

          // avoid multiple notification, when renamed object contains references to self
        if(o != this) {
          for(size_t k = d_begin; k != d_end && o; ++k) {
            const OksData & d = o->data[k];
            if(d.type == OksData::object_type && d.data.OBJECT == this) {
              updated_objects.push_back(o);
	      updated_files.insert(o->file);
              o = nullptr; // break loop
            }
            else if(d.type == OksData::list_type && d.data.LIST) {
              const OksData::List & list = *d.data.LIST;
              for(OksData::List::const_iterator l = list.begin(); l != list.end(); ++l) {
                OksData * d2 = *l;
                if(d2->type == OksData::object_type && d2->data.OBJECT == this) {
                  updated_objects.push_back(o);
	          updated_files.insert(o->file);
                  o = nullptr; // break loop
                  break;
                }
              }
            }
          }
        }
      }
    }
  }

  try {
    for(std::set<OksFile *>::const_iterator i = updated_files.begin(); i != updated_files.end(); ++i) {
      (*i)->lock();
    }
  }
  catch(oks::exception& ex) {
    throw oks::FailedRenameObject(this, ex);
  }

  try {
    c->remove(this);          // remove object from hash table
    uid.object_id = new_id;   // change id
    c->add(this);             // add object to hash table
  }
  catch(oks::exception& ex) {
    throw oks::FailedRenameObject(this, ex);
  }

  for(std::set<OksFile *>::const_iterator i = updated_files.begin(); i != updated_files.end(); ++i) {
    (*i)->set_updated();
  }

  if(get_change_notify()) {
    for(std::list<OksObject *>::const_iterator i = updated_objects.begin(); i != updated_objects.end(); ++i) {
      (*i)->change_notify();
    }
  }
}

bool
OksObject::are_equal_fast(const OksObject * o1, const OksObject * o2)
{
  if (o1 == o2)
    return true;

  if (o1 == nullptr || o2 == nullptr)
    return false;

  return (o1->uid.class_id->get_name() == o2->uid.class_id->get_name() && o1->uid.object_id == o2->uid.object_id);
}

bool
OksObject::operator==(const OksObject &o) const
{
  if (are_equal_fast(this, &o) == false)
    return false;

  OSK_PROFILING(OksProfiler::ObjectOperatorEqual, uid.class_id->p_kernel)

  size_t count(0);

  while (count < uid.class_id->p_instance_size)
    {
      if (data[count] == o.data[count])
        count++;
      else
        return false;
    }

  return true;
}


std::ostream&
operator<<(std::ostream& s, const OksObject * o)
{
  s << '\"';
  
  if(o)
    s << o->GetId() << '@' << o->GetClass()->get_name();
  else
    s << "(null-obj)";

  s << '\"';
  
  return s;
}


std::ostream&
operator<<(std::ostream& s, const OksObject& o)
{
  OSK_PROFILING(OksProfiler::ObjectOperatorOut, o.uid.class_id->p_kernel)
  
  const OksClass * c = o.uid.class_id;

  s << "Object " << &o << std::endl;
  
  OksData * di = o.data;


    // print attributes

  {
    const std::list<OksAttribute *> * alist = c->all_attributes();

    if(alist && !alist->empty()) {
      s << " Attributes are:\n";

      for(std::list<OksAttribute *>::const_iterator i = alist->begin(); i != alist->end(); ++i) {
        if((*i)->is_integer() && (*i)->get_format() != OksAttribute::Dec) {
          s.setf(((*i)->get_format() == OksAttribute::Hex ? std::ios::hex : std::ios::oct), std::ios::basefield);
          s.setf(std::ios::showbase);
        }

        s << "  " << (*i)->get_name() << ": " << *di++ << std::endl;

        if((*i)->is_integer() && (*i)->get_format() != OksAttribute::Dec) {
          s.setf(std::ios::dec, std::ios::basefield);
          s.unsetf(std::ios::showbase);
        }
      }
    }
    else
      s << " There are no attributes\n";
  }


    // print relationships

  {
    const std::list<OksRelationship *> * rlist = c->all_relationships();

    if(rlist && !rlist->empty()) {
      s << " Relationships are:\n";
	
      for(std::list<OksRelationship *>::const_iterator i = rlist->begin(); i != rlist->end(); ++i)
        s << "  " << (*i)->get_name() << ": " << *di++ << std::endl;
    }
    else
      s << " There are no relationships\n";
  }

  return s;
}	


  //
  // Remove dangling objects before writing to file
  // Avoid unnecessary additional checks
  //

static bool
trim_dangling(OksData & d, const OksKernel & kernel)
{
  if (d.type == OksData::object_type)
    {
      if (d.data.OBJECT && kernel.is_dangling(d.data.OBJECT))
        {
          d.data.OBJECT = nullptr;
        }

      return (d.data.OBJECT != nullptr);
    }
  else if(d.type == OksData::list_type)
    {
      if (d.data.LIST)
        {
          for (OksData::List::iterator i = d.data.LIST->begin(); i != d.data.LIST->end();)
            {
              OksData * d2 = *i;
              if (d2->type == OksData::object_type)
                {
                  if (d2->data.OBJECT == nullptr || kernel.is_dangling(d2->data.OBJECT))
                    {
                      i = d.data.LIST->erase(i);
                      delete d2;
                      continue;
                    }
                }
              ++i;
            }

          return (!d.data.LIST->empty());
        }
      else
        return false;
    }

  return true;
}


void
OksObject::put_object_attributes(OksXmlOutputStream& xmls, const OksData& d)
{
  const std::string * class_name = nullptr;
  const std::string * object_id = nullptr;

  if(d.type == OksData::object_type)
    {
      class_name = &d.data.OBJECT->GetClass()->get_name();
      object_id = &d.data.OBJECT->GetId();
    }
  else if(d.type == OksData::uid_type)
    {
      class_name = &d.data.UID.class_id->get_name();
      object_id = d.data.UID.object_id;
    }
  else
    {
      class_name = d.data.UID2.class_id;
      object_id = d.data.UID2.object_id;
    }

  xmls.put_attribute(class_xml_attribute, sizeof(class_xml_attribute)-1, class_name->c_str());
  xmls.put_attribute(id_xml_attribute, sizeof(id_xml_attribute)-1, object_id->c_str());
}

void
OksObject::put(OksXmlOutputStream& xmls, bool force_defaults) const
{
  OSK_PROFILING(OksProfiler::ObjectPutObject, uid.class_id->p_kernel)

  try
    {
      const OksClass * class_id = uid.class_id;

      xmls.put_start_tag(obj_xml_tag, sizeof(obj_xml_tag)-1);

      xmls.put_attribute(class_xml_attribute, sizeof(class_xml_attribute)-1, class_id->get_name().c_str());
      xmls.put_attribute(id_xml_attribute, sizeof(id_xml_attribute)-1, uid.object_id.c_str());

      xmls.put_eol();

      int count = 0;

      if (auto alist = class_id->all_attributes())
        {
          for (const auto& a : *alist)
            {
              OksData& d = data[count++];

              // store all attributes with value != initial, or date/time types which default values = now(), or non-numbers or numbers with non-zero values
              if (force_defaults || d != a->p_init_data || a->p_data_type == OksData::date_type || a->p_data_type == OksData::time_type || (a->get_init_value().empty() == false && (a->get_is_multi_values() == true || a->is_number() == false || a->get_init_value().size() > 1 || a->get_init_value()[0] != '0')))
                {
                  xmls.put_raw(' ');
                  xmls.put_start_tag(attribute_xml_tag, sizeof(attribute_xml_tag) - 1);
                  xmls.put_attribute(name_xml_attribute, sizeof(name_xml_attribute) - 1, a->get_name().c_str());
                  xmls.put_attribute(type_xml_attribute, sizeof(type_xml_attribute) - 1, a->get_type().c_str());

                  if (a->is_integer() && a->get_format() != OksAttribute::Dec)
                    {
                      xmls.get_stream().setf((a->get_format() == OksAttribute::Hex ? std::ios::hex : std::ios::oct), std::ios::basefield);
                      xmls.get_stream().setf(std::ios::showbase);
                    }

                  if (a->get_is_multi_values() == false)
                    {
                      xmls.put_attribute(value_xml_attribute, sizeof(value_xml_attribute) - 1, d);
                      xmls.put_end_tag();
                    }
                  else
                    {
                      if (d.data.LIST->empty())
                        {
                          xmls.put_end_tag();
                        }
                      else
                        {
                          if(a->p_ordered)
                            d.sort();

                          xmls.put_eol();

                          for (const auto& x : *d.data.LIST)
                            {
                              xmls.put_raw(' ');
                              xmls.put_raw(' ');
                              xmls.put_start_tag(data_xml_tag, sizeof(data_xml_tag) - 1);
                              xmls.put_attribute(value_xml_attribute, sizeof(value_xml_attribute) - 1, *x);
                              xmls.put_end_tag();
                            }

                          xmls.put_raw(' ');
                          xmls.put_last_tag(attribute_xml_tag, sizeof(attribute_xml_tag) - 1);
                        }
                    }

                  if (a->is_integer() && a->get_format() != OksAttribute::Dec)
                    {
                      xmls.get_stream().setf(std::ios::dec, std::ios::basefield);
                      xmls.get_stream().unsetf(std::ios::showbase);
                    }
                }
            }
        }

      if (const std::list<OksRelationship *> * rlist = class_id->all_relationships())
        {
          for (const auto& r : *rlist)
            {
              OksData& rel_data = data[count++];

              if (trim_dangling(rel_data, *class_id->p_kernel))
                {
                  xmls.put_raw(' ');
                  xmls.put_start_tag(relationship_xml_tag, sizeof(relationship_xml_tag) - 1);
                  xmls.put_attribute(name_xml_attribute, sizeof(name_xml_attribute) - 1, r->get_name().c_str());

                  if (r->get_high_cardinality_constraint() != OksRelationship::Many)
                    {
                      put_object_attributes(xmls, rel_data);
                      xmls.put_end_tag();
                    }
                  else
                    {
                      if(r->p_ordered)
                        rel_data.sort();

                      xmls.put_eol();

                      for (const auto& x : *rel_data.data.LIST)
                        {
                          xmls.put_raw(' ');
                          xmls.put_raw(' ');
                          xmls.put_start_tag(ref_xml_tag, sizeof(ref_xml_tag) - 1);
                          put_object_attributes(xmls, *x);
                          xmls.put_end_tag();
                        }

                      xmls.put_raw(' ');
                      xmls.put_last_tag(relationship_xml_tag, sizeof(relationship_xml_tag) - 1);
                    }
                }
            }
        }

      xmls.put_last_tag(obj_xml_tag, sizeof(obj_xml_tag) - 1);
    }
  catch (oks::exception & ex)
    {
      throw(oks::FailedSaveObject(this, ex));
    }
  catch (std::ios_base::failure & ex)
    {
      throw(oks::FailedSaveObject(this, std::string("caught std::ios_base::failure exception \"") + ex.what() + '\"'));
    }
  catch (std::exception & ex)
    {
      throw(oks::FailedSaveObject(this, std::string("caught std::exception \"") + ex.what() + '\"'));
    }
  catch (...)
    {
      throw(oks::FailedSaveObject(this, "unknown reason"));
    }
}

OksData *
OksObject::GetAttributeValue(const std::string& name) const
{
  OksDataInfo::Map::const_iterator i = uid.class_id->p_data_info->find(name);

  if(i == uid.class_id->p_data_info->end()) {
    std::ostringstream text;
    text << "object " << this << " has no attribute \"" << name << '\"';
    throw oks::ObjectGetError(this, false, name, text.str());
  }

  return GetAttributeValue(i->second);
}


void
OksObject::SetAttributeValue(const OksDataInfo *odi, OksData *d)
{
  size_t offset = odi->offset;
  const OksAttribute * a = odi->attribute;
  OksData d2; // can be needed by type conversion
  
  if(data[offset].type != d->type) {
    if(uid.class_id->get_kernel()->get_silence_mode() == false) {
      Oks::warning_msg("OksObject::SetAttributeValue(const OksDataInfo *, OksData *)")
        << "  * ODI attribute name: \'" << a->get_name() << "\'\n"
           "  * ODI data offset: \'" << offset << "\'\n"
           "  * OKS data (new): \'" << *d << "\' type: " << (int)d->type << "\n"
           "  * OKS data (old): \'" << data[offset] << "\' type: " << (int)data[offset].type << "\n"
           "  Object " << this << ":\n"
           "  Convert attribute \"" << a->get_name() << "\" value since the OksData::type is invalid\n";
    }

    d->cvt(&d2, a);
    d=&d2;
  }

  try {
    d->check_range(a);
  }
  catch(oks::AttributeReadError & ex) {
    throw oks::ObjectSetError(this, false, a->get_name(), ex);
  }
  catch(oks::AttributeRangeError & ex) {
    throw oks::ObjectSetError(this, false, a->get_name(), ex);
  }

  check_file_lock(a, 0);


    // set value and update index if any

  {
    OksIndex * i = nullptr;

    if(uid.class_id->p_indices) {
      OksIndex::Map::iterator j = uid.class_id->p_indices->find(a);
      if(j != uid.class_id->p_indices->end()) {
        i = j->second;
      }
    }

    if(i) i->remove_obj(this);
    data[offset] = *d;
    if(i) i->insert(this);
  }

  notify();
}


void
OksObject::SetAttributeValue(const std::string& name, OksData *d)
{
  OksDataInfo::Map::iterator i = uid.class_id->p_data_info->find(name);

  if(i == uid.class_id->p_data_info->end()) {
    std::ostringstream text;
    text << "object " << this << " has no attribute \"" << name << '\"';
    throw oks::ObjectSetError(this, false, name, text.str());
  }

  SetAttributeValue(i->second, d);
}

OksData *
OksObject::GetRelationshipValue(const std::string& name) const
{
  OksDataInfo::Map::iterator i = uid.class_id->p_data_info->find(name);
  
  if(i == uid.class_id->p_data_info->end()) {
    std::ostringstream text;
    text << "object " << this << " has no relationship \"" << name << '\"';
    throw oks::ObjectGetError(this, false, name, text.str());
  }

  return GetRelationshipValue(i->second);
}


void
OksObject::check_non_null(const OksRelationship *r, const OksObject *o)
{
  if(!o && r->get_low_cardinality_constraint() != OksRelationship::Zero) {
    throw oks::ObjectSetError(
      this, true, r->get_name(),
      "set to (null) is not allowed since the relationship has non-zero low cardinality constraint"
    );
  }
}


void
OksObject::check_class_type(const OksRelationship *r, const OksClass *class_type)
{
  if(!class_type) return;

  const OksClass * rel_class_type = r->p_class_type;

  if(class_type != rel_class_type) {
    OksClass::FList * super_classes = class_type->p_all_super_classes;

    if(super_classes && !super_classes->empty()) {
      for(const auto& i : *super_classes) {
        if(rel_class_type == i) return;
      }
    }

    std::ostringstream text;
    text << "set to an object of class \"" << class_type->get_name() << "\" is not allowed since the relationship's class type is \""
         << r->get_type() << "\" and the class \"" << class_type->get_name() << "\" has no such superclass";
    throw oks::ObjectSetError(this, true, r->get_name(), text.str());
  }
}


void
OksObject::SetRelationshipValue(const OksDataInfo *odi, OksData *d, bool skip_non_null_check)
{
  const OksRelationship * r = odi->relationship;
  size_t offset = odi->offset;

  if(
   (data[offset].type != d->type) &&
   ( (d->type == OksData::list_type) || (data[offset].type == OksData::list_type) )
  ) {
    std::ostringstream text;
    text << "OKS data type is invalid: must be " << (data[offset].type == OksData::object_type ? "an object)\n" : "a list); ")
         << "offset: " << offset << "; new: \'" << *d << "\'; old:" << data[offset] << '\'';
    throw oks::ObjectSetError(this, true, r->get_name(), text.str());
  }

    // used to update RCRs

  OksObject::FSet added_objs;
  OksObject::FSet removed_objs;

  if(d->type == OksData::object_type) {
    OksObject * add_obj = d->data.OBJECT;
    OksObject * rm_obj = (data[offset].type == OksData::object_type ? data[offset].data.OBJECT : 0);

    if(add_obj != rm_obj) {
      if(add_obj) added_objs.insert(add_obj);
      if(rm_obj) removed_objs.insert(rm_obj);
    }
    else {
      return; // new and old values are equal, nothing to do
    }

    if(skip_non_null_check == false) check_non_null(r, d->data.OBJECT);
    check_class_type(r, d->data.OBJECT);
  }
  else if(d->type == OksData::list_type && d->data.LIST) {
    if(d->data.LIST->empty()) {
      if(r->get_low_cardinality_constraint() != OksRelationship::Zero && skip_non_null_check == false) {
        throw oks::ObjectSetError(
          this, true, r->get_name(),
          "set to empty list is not allowed since the relationship has non-zero low cardinality constraint"
        );
      }
    }

    for(const auto& i : *d->data.LIST) {
      if(i->type != OksData::object_type && i->type != OksData::uid2_type && i->type != OksData::uid_type) {
        throw oks::ObjectSetError(this, true, r->get_name(), "list contains non-objects");
      }

      if(
       (i->type == OksData::object_type && !i->data.OBJECT) ||
       (i->type == OksData::uid2_type && !i->data.UID2.object_id) ||
       (i->type == OksData::uid_type && !i->data.UID.object_id)
      ) {
        throw oks::ObjectSetError(this, true, r->get_name(), "list contains (null) object");
      }

      if(i->type == OksData::object_type) {
        if(OksObject * o2 = i->data.OBJECT) {
          check_class_type(r, o2);
	  added_objs.insert(o2);
	}
      }
    }
  }
  else {
    throw oks::ObjectSetError(
      this, true, r->get_name(),
      "the OKS data type is invalid (must be a list of objects or an object)"
    );
  }

  check_file_lock(0, r);


    // put old value to objects removed from relationship

  if(data[offset].type == OksData::list_type && data[offset].data.LIST) {
      for(const auto& i : *data[offset].data.LIST) {
	if(i->type == OksData::object_type && i->data.OBJECT) {
	  removed_objs.insert(i->data.OBJECT);
	}
      }
  }


    // remove RCRs

  for(const auto& i : removed_objs) {
    if(added_objs.find(i) == added_objs.end()) {
      TLOG_DEBUG(4) << "object " << i << " was removed from relationship";
      i->remove_RCR(this, r);
    }
  }


    // add RCRs

  for(OksObject::FSet::const_iterator i = added_objs.begin(); i != added_objs.end(); ++i) {
    OksObject * add_obj = *i;
    if(removed_objs.find(add_obj) == removed_objs.end()) {
      TLOG_DEBUG(4) << "object " << add_obj << " was added to relationship";
      try {
        add_obj->add_RCR(this, r);
      }
      catch(oks::exception& ex) {

          // reset updated RCRs

        for(OksObject::FSet::const_iterator j = added_objs.begin(); j != i; ++j) {
          if(removed_objs.find(*j) == removed_objs.end()) {
            (*j)->remove_RCR(this, r);
          }
        }

        for(OksObject::FSet::const_iterator j = removed_objs.begin(); j != removed_objs.end(); ++j) {
          if(added_objs.find(*j) == added_objs.end()) {
            try { (*j)->add_RCR(this, r); } catch (oks::exception&) { ; }
          }
        }

        throw oks::ObjectSetError(this, true, r->get_name(), ex);
      }
    }
  }

  data[offset] = *d;

  notify();
}


void
OksObject::SetRelationshipValue(const std::string& name, OksData *d, bool skip_non_null_check)
{
  OksDataInfo::Map::iterator i = uid.class_id->p_data_info->find(name);

  if(i == uid.class_id->p_data_info->end()) {
    std::ostringstream text;
    text << "object " << this << " has no relationship \"" << name << '\"';
    throw oks::ObjectSetError(this, true, name, text.str());
  }

  SetRelationshipValue(i->second, d, skip_non_null_check);
}

	
void
OksObject::SetRelationshipValue(const OksDataInfo *odi, OksObject *object)
{
  const OksRelationship	*r = odi->relationship;
  OksData& d(data[odi->offset]);

  if( r->get_high_cardinality_constraint() == OksRelationship::Many ) {
    std::ostringstream text;
    text << "cannot set value " << object << " since the relationship has \'many\' high cardinality constraint";
    throw oks::ObjectSetError(this, true, r->get_name(), text.str());
  }

  if(!object) {
    check_non_null(r, object);
  }
  else {
    check_class_type(r, object);
  }

  check_file_lock(0, r);

  if((d.type == OksData::object_type) && d.data.OBJECT) {
    d.data.OBJECT->remove_RCR(this, r);
  }

  if(object) {
    try {
      object->add_RCR(this, r);
    }
    catch(oks::exception& ex) {
      // restore RCR and throw exception
      if((d.type == OksData::object_type) && d.data.OBJECT)
        try { d.data.OBJECT->add_RCR(this, r); } catch(oks::exception& ex2) { ; }
      throw oks::ObjectSetError(this, true, r->get_name(), ex);
    }
  }

  d.Set(object);

  notify();
}


void
OksObject::SetRelationshipValue(const std::string& name, OksObject *o)
{
  OksDataInfo::Map::iterator i = uid.class_id->p_data_info->find(name);

  if(i == uid.class_id->p_data_info->end()) {
    std::ostringstream text;
    text << "object " << this << " has no relationship \"" << name << '\"';
    throw oks::ObjectSetError(this, true, name, text.str());
  }

  SetRelationshipValue(i->second, o);
}

	

static bool
cmp_data(OksData * d, OksData * d2)
{
  if(d->type == OksData::object_type && d2->type == OksData::object_type)
    return ((d2->data.OBJECT == d->data.OBJECT) ? true : false);

  const std::string& object_id =
    d->type == OksData::object_type  ? d->data.OBJECT->GetId() :
    d->type == OksData::uid2_type    ? *(static_cast<std::string*>(d->data.UID2.object_id)) :
    *(static_cast<std::string*>(d->data.UID.object_id));

  const std::string& object_id2 =
    d2->type == OksData::object_type ? d2->data.OBJECT->GetId() :
    d2->type == OksData::uid2_type   ? *(static_cast<std::string*>(d2->data.UID2.object_id)) :
    *(static_cast<std::string*>(d2->data.UID.object_id));

  if(object_id != object_id2) return false;

  const OksClass *pclass_id =
    d->type == OksData::object_type  ? d->data.OBJECT->GetClass() :
    d->type == OksData::uid_type     ? d->data.UID.class_id :
    0;

  const OksClass *pclass_id2 =
    d2->type == OksData::object_type ? d2->data.OBJECT->GetClass() :
    d2->type == OksData::uid_type    ? d2->data.UID.class_id :
    0;

  if(pclass_id && pclass_id2) return (pclass_id == pclass_id2 ? true : false);

  const std::string& class_id = 
    (pclass_id ? pclass_id->get_name() : *(static_cast<std::string*>(d->data.UID2.class_id)));

  const std::string& class_id2 =
    (pclass_id2 ? pclass_id2->get_name() : *(static_cast<std::string*>(d2->data.UID2.class_id)));

  return (class_id == class_id2 ? true : false);
}


void
OksObject::AddRelationshipValue(const OksDataInfo *odi, OksObject *object)
{
  const OksRelationship	* r = odi->relationship;
  size_t offset = odi->offset;

  if(!object) {
    throw oks::ObjectSetError(this, true, r->get_name(), "the (null) value is not allowed");
  }

  check_class_type(r, object);

  if( r->get_high_cardinality_constraint() != OksRelationship::Many ) {
    std::ostringstream text;
    text << "cannot add value " << object << " since the relationship has no \'many\' high cardinality constraint";
    throw oks::ObjectSetError(this, true, r->get_name(), text.str());
  }

  try {
    object->add_RCR(this, r);
  }
  catch(oks::exception& ex) {
    throw oks::ObjectSetError(this, true, r->get_name(), ex);
  }

  check_file_lock(0, r);

  data[offset].data.LIST->push_back(new OksData(object));

  notify();
}


void
OksObject::AddRelationshipValue(const std::string& name, OksObject *object)
{
  OksDataInfo::Map::iterator i = uid.class_id->p_data_info->find(name);

  if(i == uid.class_id->p_data_info->end()) {
    std::ostringstream text;
    text << "object " << this << " has no relationship \"" << name << '\"';
    throw oks::ObjectSetError(this, true, name, text.str());
  }

  AddRelationshipValue(i->second, object);
}


void
OksObject::RemoveRelationshipValue(const OksDataInfo *odi, OksObject *object)
{
  const OksRelationship	* r = odi->relationship;
  
  if(!object) {
    throw oks::ObjectSetError(this, true, r->get_name(), "cannot remove (null) object");
  }

  if(r->get_high_cardinality_constraint() != OksRelationship::Many) {
    std::ostringstream text;
    text << "cannot remove object " << object << " since the relationship has no \'many\' high cardinality constraint";
    throw oks::ObjectSetError(this, true, r->get_name(), text.str());
  }

  OksData d(object);
  size_t offset = odi->offset;
  OksData::List * list = data[offset].data.LIST;
  
  for(OksData::List::iterator i = list->begin(); i != list->end();) {
    OksData * d2 = *i;
    if(cmp_data(d2, &d)) {
      check_file_lock(0, r);
      object->remove_RCR(this, r);
      list->erase(i);
      delete d2;
      notify();
      return;
    }
    else ++i;
  }

  std::ostringstream text;
  text << "cannot remove object " << object << " since the relationship\'s value has no such object";
  throw oks::ObjectSetError(this, true, r->get_name(), text.str());
}


void
OksObject::RemoveRelationshipValue(const std::string& name, OksObject * object)
{
  OksDataInfo::Map::iterator i = uid.class_id->p_data_info->find(name);

  if(i == uid.class_id->p_data_info->end()) {
    std::ostringstream text;
    text << "object " << this << " has no relationship \"" << name << '\"';
    throw oks::ObjectSetError(this, true, name, text.str());
  }

  RemoveRelationshipValue(i->second, object);
}

void
OksObject::SetRelationshipValue(const std::string& name, const std::string& class_id, const std::string& object_id)
{
  OksDataInfo::Map::iterator i = uid.class_id->p_data_info->find(name);
  
  if(i == uid.class_id->p_data_info->end()) {
    std::ostringstream text;
    text << "object " << this << " has no relationship \"" << name << '\"';
    throw oks::ObjectSetError(this, true, name, text.str());
  }

  OksDataInfo * odi = i->second;


  const OksRelationship	* r = odi->relationship;
  size_t offset = odi->offset;

  if(object_id.empty() || class_id.empty()) {
    check_non_null(r, 0);
    check_file_lock(0, r);

    if( (data[offset].type == OksData::object_type) && data[offset].data.OBJECT ) {
      data[offset].data.OBJECT->remove_RCR(this, r);
    }

    data[offset].Set((OksObject *)0);

    notify();

    return;
  }
  
  OksClass *c = uid.class_id->p_kernel->find_class(class_id);

  if(c) {
    check_class_type(r, c);
  }
  else if(r->get_type() != class_id) {
    std::ostringstream text;
    text << "cannot set value \"" << object_id << '@' << class_id << "\" since the class \"" << class_id
         << "\" is not defined and it is impossible to check its validity for given type of relationship";
    throw oks::ObjectSetError(this, true, r->get_name(), text.str());
  }

  if(r->get_high_cardinality_constraint() == OksRelationship::Many) {
    std::ostringstream text;
    text << "cannot set object \"" << object_id << '@' << class_id << "\" since the relationship has \'many\' high cardinality constraint";
    throw oks::ObjectSetError(this, true, r->get_name(), text.str());
  }

  OksObject *o = nullptr;

  if(c) o = c->get_object(object_id);

  if(o) {
    SetRelationshipValue(name, o);
  }
  else {
    check_file_lock(0, r);

    if( (data[offset].type == OksData::object_type) && data[offset].data.OBJECT ) {
      data[offset].data.OBJECT->remove_RCR(this, r);
    }

    data[offset].Set(class_id, object_id); 

    notify();
  }
}


void
OksObject::AddRelationshipValue(const std::string& name, const std::string& class_id, const std::string& object_id)
{
  OksDataInfo::Map::iterator i = uid.class_id->p_data_info->find(name);
  
  if(i == uid.class_id->p_data_info->end()) {
    std::ostringstream text;
    text << "object " << this << " has no relationship \"" << name << '\"';
    throw oks::ObjectSetError(this, true, name, text.str());
  }

  OksDataInfo * odi = i->second;
  const OksRelationship	* r = odi->relationship;

  if(class_id.empty() || object_id.empty()) {
    throw oks::ObjectSetError(this, true, r->get_name(), "cannot add (null) object");
  }

  const std::string& r_class_type = r->get_type();
  size_t offset = odi->offset;

  OksClass *c = uid.class_id->p_kernel->find_class(class_id);

  if(c) {
    check_class_type(r, c);
  }
  else if(r_class_type != class_id) {
    std::ostringstream text;
    text << "cannot add value \"" << object_id << '@' << class_id << "\" since the class \"" << class_id
         << "\" is not defined and it is impossible to check its validity for given type of relationship";
    throw oks::ObjectSetError(this, true, r->get_name(), text.str());
  }

  if(r->get_high_cardinality_constraint() != OksRelationship::Many) {
    std::ostringstream text;
    text << "cannot add object \"" << object_id << '@' << class_id << "\" since the relationship has no \'many\' high cardinality constraint";
    throw oks::ObjectSetError(this, true, r->get_name(), text.str());
  }

  OksObject *o = nullptr;

  if(c) o = c->get_object(object_id);

  if(o) {
    AddRelationshipValue(odi, o);
  }
  else {
    check_file_lock(0, r);
    data[offset].data.LIST->push_back(new OksData(class_id, object_id));
    notify();
  }
}


void
OksObject::RemoveRelationshipValue(const std::string& name, const std::string& class_id, const std::string& object_id)
{
  OksDataInfo::Map::iterator i = uid.class_id->p_data_info->find(name);

  if(i == uid.class_id->p_data_info->end()) {
    std::ostringstream text;
    text << "object " << this << " has no relationship \"" << name << '\"';
    throw oks::ObjectSetError(this, true, name, text.str());
  }

  OksDataInfo * odi = i->second;
  const OksRelationship	* r = odi->relationship;

  if(object_id.empty() || class_id.empty()) {
    throw oks::ObjectSetError(this, true, r->get_name(), "cannot remove (null) object");
  }

  if( r->get_high_cardinality_constraint() != OksRelationship::Many ) {
    std::ostringstream text;
    text << "cannot remove object \"" << object_id << '@' << class_id << "\" since the relationship has no \'many\' high cardinality constraint";
    throw oks::ObjectSetError(this, true, r->get_name(), text.str());
  }

  size_t offset = odi->offset;
  OksData d(class_id, object_id);
  OksData::List * list = data[offset].data.LIST;

  for(OksData::List::iterator j = list->begin(); j != list->end();) {
    OksData * d2 = *j;
    if(cmp_data(d2, &d)) {
      check_file_lock(0, r);
      list->erase(j);
      delete d2;
      notify();
      return;
    }
    else ++j;
  }

  std::ostringstream text;
  text << "cannot remove object \"" << object_id << '@' << class_id << "\" since the relationship\'s value has no such object";
  throw oks::ObjectSetError(this, true, r->get_name(), text.str());
}


void
OksObject::add_RCR(OksObject *o, const OksRelationship *r)
{
  if(r->get_is_composite() == false) return;

  TLOG_DEBUG(4) << "object " << this << " adds RCR to object " << o << " throught relationship \"" << r->get_name() << '\"';

  if(!p_rcr) {
    p_rcr = new std::list<OksRCR *>();
  }
  else {
    for(const auto& i : *p_rcr) {
      if(i->relationship == r) {
	if(i->obj == o) {
	  TLOG_DEBUG(4) << "[this=" << this << ", o=" << o << ", r=\"" << r->get_name() << "\"]: such RCR was already set.";
	  return;
  	}
        else if(r->get_is_exclusive()) {
	  throw oks::AddRcrError(this, r->get_name(), o, i->obj);
        }
      }
    }
  }

  p_rcr->push_back(new OksRCR(o, r));
}


void
OksObject::remove_RCR(OksObject *o, const OksRelationship *r) noexcept
{
  TLOG_DEBUG(4) << "object " << this << " removes RCR from object " << o << " through " << r->get_name();

  if(r->get_is_composite() == false || !p_rcr) return;

  for(std::list<OksRCR *>::iterator i = p_rcr->begin(); i != p_rcr->end(); ++i) {
    OksRCR *rcr = *i;

    if(rcr->obj == o && rcr->relationship == r) {
      p_rcr->erase(i);
      delete rcr;
      break;
    }
  }

  if(p_rcr->empty()) {
    delete p_rcr;
    p_rcr = nullptr;
  }
}


void
OksObject::bind(OksData *d, const BindInfo& info)
{
  const OksClass * c(0);
  OksObject * o(0);

  if(d->type == OksData::uid_type) {
    if(d->data.UID.class_id && !d->data.UID.object_id->empty()) {
      c = d->data.UID.class_id;
      o = c->get_object(d->data.UID.object_id);
    }
    else {
      d->Set((OksObject *)0);
      return;
    }
  }
  else {
    if(d->data.UID2.class_id->empty() || d->data.UID2.object_id->empty()) {
      d->Set((OksObject *)0);
      return;
    }

    c = info.k->find_class(*d->data.UID2.class_id);

    if(c) {
      o = c->get_object(d->data.UID2.object_id);
    }
    else {
      std::ostringstream text;
      text << "Class of object " << *d << " is not defined";
      throw oks::ObjectBindError(info.o, d, info.r, true, text.str(), "");
    }
  }

  if(c != info.r->p_class_type) {
    if(const OksClass::FList * slist = c->all_super_classes()) {
      for(OksClass::FList::const_iterator i = slist->begin(); i != slist->end(); ++i) {
        if(info.r->p_class_type == *i) {
          goto check_passed;
        }
      }
    }

    {
      std::ostringstream text;
      text << "The relationship has class type \"" << info.r->get_type() << "\" and the referenced object " << *d << " is not of that class or derived one";
      throw oks::ObjectBindError(info.o, d, info.r, true, text.str(), "");
    }

    check_passed: ;
  }

  if(!o) {
    std::ostringstream text;
    text << "Cannot find object " << *d;
    throw oks::ObjectBindError(info.o, d, info.r, false, text.str(), "");
  }

  try {
    d->Set(o);
    o->add_RCR(info.o, info.r);
  }
  catch(oks::exception& ex) {
    std::ostringstream text;
    text << "Failed to set relationship value " << o;
    throw oks::ObjectBindError(info.o, d, info.r, true, text.str(), ex);
  }
}


struct BindWarning {
  std::ostringstream * p_warnings; // in 99.999% cases there is no need to call the constructor
  unsigned int p_warnings_count;

  BindWarning() : p_warnings(0), p_warnings_count(0) { ; }
  ~BindWarning() { delete p_warnings; }

  void add(const OksData& d, const OksRelationship * r);
  void add(const OksRelationship * r);
  std::ostringstream& add();
};

std::ostringstream&
BindWarning::add()
{
  if(p_warnings_count != 0) (*p_warnings) << '\n';
  else {p_warnings = new std::ostringstream();}
  p_warnings_count++;
  return *p_warnings;
}


void
BindWarning::add(const OksData& d, const OksRelationship * r)
{
  add() << " * object " << d << " via relationship \"" << r->get_name() << '\"';
}

void
BindWarning::add(const OksRelationship * r)
{
  add() << " * relationship \"" << r->get_name() << "\" has non-zero low cardinality constraint, but it is empty";
}


void
OksObject::bind_objects()
{
  const OksClass * c = uid.class_id;

  if(c->p_all_relationships->empty()) return;  // if class has no relationships then nothing to do

  BindInfo info;
  info.k = c->p_kernel;
  info.o = this;

  OksData * d(data + c->number_of_all_attributes());

  BindWarning warnings;

  for (auto & i : (*c->p_all_relationships))
    {
      info.r = i;

      if(d->type == OksData::object_type)
        {
          if (i->get_low_cardinality_constraint() == OksRelationship::One && d->data.OBJECT == nullptr) { warnings.add(i); }
        }
      else if (d->type == OksData::uid_type)
        {
          if (d->data.UID.object_id == nullptr)
            {
              if (i->get_low_cardinality_constraint() == OksRelationship::One) { warnings.add(i); }
            }

          try
            {
              bind(d, info);
            }
          catch (oks::ObjectBindError& ex)
            {
              if (ex.p_is_error) { throw; }
              else { warnings.add(*ex.p_data, ex.p_rel); }
            }
        }
      else if (d->type == OksData::list_type)
        {
          if (d->data.LIST->empty())
            {
              if (i->get_low_cardinality_constraint() == OksRelationship::One) { warnings.add(i); }
            }
          else
            {
              for (const auto& i2 : *d->data.LIST)
                {
                  if (i2->type == OksData::uid_type || i2->type == OksData::uid2_type)
                    {
                      try
                        {
                          bind(i2, info);
                        }
                      catch (oks::ObjectBindError& ex)
                        {
                          if (ex.p_is_error) { throw; }
                          else { warnings.add(*ex.p_data, ex.p_rel); }
                        }
                    }
                }
            }
        }
      else if (d->type == OksData::uid2_type)
        {
          if (i->get_low_cardinality_constraint() == OksRelationship::One && d->data.UID2.object_id == nullptr) { warnings.add(i); }
          try
            {
              bind(d, info);
            }
          catch (oks::ObjectBindError& ex)
            {
              if (ex.p_is_error) { throw; }
              else { warnings.add(*ex.p_data, ex.p_rel); }
            }
        }

      d++;
    }

  if(warnings.p_warnings_count)
    {
      std::ostringstream text;
      const char * s1(warnings.p_warnings_count == 1 ? "is" : "are");
      const char * s2(warnings.p_warnings_count == 1 ? "" : "s");
      text << "There " << s1 << " " << warnings.p_warnings_count << " unresolved reference" << s2 << " from object " << this
           << '\n' << warnings.p_warnings->str();
      throw oks::ObjectBindError(this, 0, 0, false, text.str(), "");
    }
}


void
OksObject::unbind_file(const OksFile * f)
{
  const size_t num_of_attrs (uid.class_id->number_of_all_attributes());
  const size_t num_of_rels  (uid.class_id->number_of_all_relationships());

  bool verbose = GetClass()->get_kernel()->get_verbose_mode();

  for(size_t i = 0; i < num_of_rels; ++i) {
    OksData *d = &data[i + num_of_attrs];
    
    if(d) {
      if(d->type == OksData::object_type) {
        OksObject * o = d->data.OBJECT;

	if(o && o->file == f) {
          if(verbose)
	    std::cout << "- unbind_file(\'" << f->get_full_file_name() << "\') in " << this << ": replace " << *d;

	  d->Set(o->GetClass(), o->GetId());

          if(verbose) std::cout << " by " << *d << std::endl;
	}
      }
      else if(d->type == OksData::list_type && d->data.LIST) {
        for(const auto& j : *d->data.LIST) {
	  if(j && j->type == OksData::object_type) {
            OksObject * o = j->data.OBJECT;

	    if(o && o->file == f) {
              if(verbose)
	        std::cout << "+ unbind_file(\'" << f->get_full_file_name() << "\') in " << this << ": replace " << *j;

	      j->Set(o->GetClass(), o->GetId());

              if(verbose) std::cout << " by " << *j << std::endl;
	    }
	  }
	}
      }
    }
  }
}

OksObject::notify_obj
OksObject::get_change_notify() const
{
  return uid.class_id->p_kernel->p_change_object_notify_fn;
}

 /**
  *  Invoke create object notification.
  */

void
OksObject::create_notify()
{
  OksKernel * k = uid.class_id->p_kernel;

  if(k->p_create_object_notify_fn) {
    (*k->p_create_object_notify_fn)(this, k->p_create_object_notify_param);
  }
}

 /**
  *  Invoke change object notification.
  */

void
OksObject::change_notify()
{
  OksKernel * k = uid.class_id->p_kernel;
  if(k->p_change_object_notify_fn) {
    (*k->p_change_object_notify_fn)(this, k->p_change_object_notify_param);
  }
}

 /**
  *  Invoke delete object notification.
  */

void
OksObject::delete_notify()
{
  OksKernel * k = uid.class_id->p_kernel;

  if(k->p_delete_object_notify_fn) {
    (*k->p_delete_object_notify_fn)(this, k->p_delete_object_notify_param);
  }
}

bool
OksObject::check_links_and_report(const OksObject * o2, const std::set<OksFile *>& includes, const std::string& name, const char * msg) const
{
  if(get_file() != o2->get_file()) {
    if(includes.find(o2->get_file()) == includes.end()) {
      if(GetClass()->get_kernel()->get_silence_mode() == false) {
        std::cerr <<
          msg << ": no files inclusion path between referenced objects:\n"
	  "  object " << this << " from file \"" << get_file()->get_full_file_name() << "\" via relationship \"" << name << "\"\n"
	  "  has reference to object " << o2 << " from file \"" << o2->get_file()->get_full_file_name() << "\";\n"
          "  file \"" << get_file()->get_full_file_name() << "\" includes " << includes.size() << " files:\n";

        for(std::set<OksFile *>::const_iterator j = includes.begin(); j != includes.end(); ++j) {
          std::cerr << "  * \'" << (*j)->get_full_file_name() << "\'\n"; 
        } 
        
      }

      return false;
    }
  }
  
  return true;
}

static void
test_dangling_references(const OksObject * obj, const OksData& d, const OksRelationship& r, std::string& result)
{
  if(d.type == OksData::uid_type || d.type == OksData::uid2_type) {
    std::ostringstream text;

    if(result.empty()) {
      text << " * object " << obj << " has dangling references:\n";
    }

    text << "  - object " << d << " referenced via \'" << r.get_name() << "\'\n";

    result += text.str();
  }
}

std::string
OksObject::report_dangling_references() const
{
  unsigned short l1 = GetClass()->number_of_all_attributes();
  unsigned short l2 = l1 + GetClass()->number_of_all_relationships();

  std::list<OksRelationship *>::const_iterator ri = GetClass()->all_relationships()->begin();

  std::string result;

  while(l1 < l2) {
    OksDataInfo odi(l1, *ri);
    OksData * d(GetRelationshipValue(&odi));

    if(d->type == OksData::list_type) {
      for(OksData::List::const_iterator li = d->data.LIST->begin(); li != d->data.LIST->end(); ++li) {
        test_dangling_references(this, **li, **ri, result);
      }
    }
    else {
      test_dangling_references(this, *d, **ri, result);
    }

    ++l1;
    ++ri;
  }

  return result;
}


bool
OksObject::is_consistent(const std::set<OksFile *>& includes, const char * msg) const
{
  unsigned short l1 = GetClass()->number_of_all_attributes();
  unsigned short l2 = l1 + GetClass()->number_of_all_relationships();

  std::list<OksRelationship *>::const_iterator ri = GetClass()->all_relationships()->begin();

  bool return_value = true;

  while(l1 < l2) {
    OksDataInfo odi(l1, *ri);
    OksData * d(GetRelationshipValue(&odi));

    if(d->type == OksData::object_type) {
      if(OksObject * o2 = d->data.OBJECT) {
        check_links_and_report(o2, includes, odi.relationship->get_name(), msg);
      }
    }
    else if(d->type == OksData::list_type) {
      for(OksData::List::const_iterator li = d->data.LIST->begin(); li != d->data.LIST->end(); ++li) {
        if((*li)->type == OksData::object_type) {
          if(OksObject * o2 = (*li)->data.OBJECT) {
            check_links_and_report(o2, includes, odi.relationship->get_name(), msg);
          }
        }
      }
    }

    if(d->IsConsistent(odi.relationship, this, "ERROR") == false) {
      return_value = false;
    }

    ++l1;
    ++ri;
  }

    // check inclusion of the schema file

  if(includes.find(GetClass()->get_file()) == includes.end()) {
    if(GetClass()->get_kernel()->get_silence_mode() == false) {
      std::cerr <<
        "ERROR: the schema file is not included for object " << this << ".\n"
        "  The data file \"" << get_file()->get_full_file_name() << "\" containg above object\n"
        "  cannot be loaded or edited without inclusion of additional files.\n"
        "  The file shall explicitly or implicitly (i.e. via other included files) include schema file\n"
        "  \"" << GetClass()->get_file()->get_full_file_name() << "\" defining class \"" << GetClass()->get_name() << "\".\n";
    }

    return false;
  }


  return return_value;
}


struct RefData {
  OksObject::FSet& refs;
  OksObject::FSet tested_objs;
  oks::ClassSet * classes;

  RefData(OksObject::FSet& r, oks::ClassSet * cs) : refs(r), classes(cs) {
    if(classes && classes->empty()) classes = nullptr;
  }
};


static void _references(const OksObject * obj, unsigned long recursion_depth, RefData& data);


static void
insert2refs(const OksObject *o, unsigned long recursion_depth, RefData& data)
{
  recursion_depth--;

  std::pair<OksObject::FSet::iterator,bool> ret = data.tested_objs.insert(const_cast<OksObject *>(o));

  if(ret.second==true) {
    if(data.classes == nullptr || data.classes->find(const_cast<OksClass *>(o->GetClass())) != data.classes->end()) {
      data.refs.insert(const_cast<OksObject *>(o));
    }
    o->SetTransientData((void *)recursion_depth);
    _references(o, recursion_depth, data);
  }
  else if(o->GetTransientData() < (void *)recursion_depth) {
    o->SetTransientData((void *)recursion_depth);
    _references(o, recursion_depth, data);
  }
}

static void
_references(const OksObject * obj, unsigned long recursion_depth, RefData& data)
{
  if(recursion_depth == 0) return;

  size_t l1 = obj->GetClass()->number_of_all_attributes();
  size_t l2 = l1 + obj->GetClass()->number_of_all_relationships();

  while(l1 < l2) {
    OksDataInfo odi(l1, (OksRelationship *)0);
    OksData * d(obj->GetRelationshipValue(&odi));

    if(d->type == OksData::object_type) {
      if(OksObject * o2 = d->data.OBJECT) {
        insert2refs(o2, recursion_depth, data);
      }
    }
    else if(d->type == OksData::list_type) {
      for(OksData::List::const_iterator li = d->data.LIST->begin(); li != d->data.LIST->end(); ++li) {
        if((*li)->type == OksData::object_type) {
          if(OksObject * o2 = (*li)->data.OBJECT) {
            insert2refs(o2, recursion_depth, data);
          }
        }
      }
    }

    ++l1;
  }
}

void
OksObject::references(OksObject::FSet& refs, unsigned long recursion_depth, bool add_self, oks::ClassSet * classes) const
{
  struct RefData data(refs, classes);

  std::lock_guard lock(uid.class_id->p_kernel->p_objects_refs_mutex);

  OksObject::FSet tested_objs;

  if(add_self) {
    insert2refs(this, recursion_depth + 1, data);
  }

  _references(this, recursion_depth, data);

  TLOG_DEBUG(2) <<  "OksObject::references(" << this << ", " << recursion_depth << ") returns " << refs.size() << " objects";
}


OksObject::FList *
OksObject::get_all_rels(const std::string& name) const
{
  bool any_name = (name == "*");
  
  OksObject::FList * result = nullptr;
  
  const OksClass::Map& all_classes(GetClass()->get_kernel()->classes());

  for(OksClass::Map::const_iterator i = all_classes.begin(); i != all_classes.end(); ++i) {
    OksClass * c(i->second);
    if(any_name || c->find_relationship(name) != nullptr) {
      if(const OksObject::Map * objs = i->second->objects()) {
        for(OksObject::Map::const_iterator j = objs->begin(); j != objs->end(); ++j) {
          OksObject *o(j->second);
	  unsigned short l1, l2;
	  
	  if(any_name) {
            l1 = c->number_of_all_attributes();
            l2 = l1 + c->number_of_all_relationships();
	  }
	  else {
	    l1 = c->data_info(name)->offset;
	    l2 = l1+1;
	  }

          while(l1 < l2) {
            OksDataInfo odi(l1, (OksRelationship *)0);
            OksData * d(o->GetRelationshipValue(&odi));

            if(d->type == OksData::object_type) {
              if(this == d->data.OBJECT) {
                if(!result) result = new OksObject::FList();
		result->push_back(o);
		break;
              }
            }
            else if(d->type == OksData::list_type) {
	      bool found = false;
              for(OksData::List::const_iterator li = d->data.LIST->begin(); li != d->data.LIST->end(); ++li) {
                OksData * lid(*li);
                if(lid->type == OksData::object_type) {
                  if(this == lid->data.OBJECT) {
                    if(!result) result = new OksObject::FList();
		    result->push_back(o);
		    found = true;
		    break; // exit given relationship value iterator
                  }
                }
              }
              if(found) break; //exit relationships iterator for given object
            }
            ++l1;
          }
        }
      }
    }
  }
  
  return result;
}

void
OksObject::check_file_lock(const OksAttribute * a, const OksRelationship * r)
{
  try {
    file->lock();
  }
  catch(oks::exception& ex) {
    throw oks::ObjectSetError(this, (a == nullptr), (a ? a->get_name() : r->get_name()), ex);
  }
}
