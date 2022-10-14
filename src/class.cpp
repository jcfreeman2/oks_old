#define _OksBuildDll_

#include "oks/class.hpp"
#include "oks/xml.hpp"
#include "oks/relationship.hpp"
#include "oks/method.hpp"
#include "oks/kernel.hpp"
#include "oks/object.hpp"
#include "oks/index.hpp"
#include "oks/profiler.hpp"
#include "oks/cstring.hpp"

#include "ers/ers.hpp"
#include "logging/Logging.hpp"

#include <stdlib.h>

#include <string>
#include <algorithm>
#include <stdexcept>



const char OksClass::class_xml_tag[]        = "class";
const char OksClass::name_xml_attr[]        = "name";
const char OksClass::description_xml_attr[] = "description";
const char OksClass::is_abstract_xml_attr[] = "is-abstract";
const char OksClass::superclass_xml_attr[]  = "superclass";


OksClass::NotifyFN	 OksClass::create_notify_fn;
OksClass::ChangeNotifyFN OksClass::change_notify_fn;
OksClass::NotifyFN	 OksClass::delete_notify_fn;


namespace oks {

  std::string
  CannotFindSuperClass::fill(const OksClass& c, const std::string& name) noexcept
  {
    return std::string("cannot find superclass \'") + name + "\' of class \'" + c.get_name() + '\'';
  }

  std::string
  AttributeConversionFailed::fill(const OksAttribute& a, const OksObject * o, const std::string& reason) noexcept
  {
    std::ostringstream s;
    s << "failed to convert \'" << a.get_name() << "\" value of object " << o <<  " because:\n" + reason;
    return s.str();
  }

  std::string
  CannotRegisterClass::fill(const OksClass& c, const std::string& what, const std::string& reason) noexcept
  {
    return std::string("cannot register class \'") + c.get_name() + "\' " + what + "because:\n" + reason;
  }

  std::string
  CannotDestroyClass::fill(const OksClass& c, const std::string& reason) noexcept
  {
    return std::string("cannot destroy class \'") + c.get_name() + "\' because:\n" + reason;
  }

  std::string
  ObjectOperationFailed::fill(const OksClass& c, const std::string& oid, const char * op, const std::string& reason) noexcept
  {
    return std::string("cannot ") + op + " object \'" + oid + "\' in class \'" + c.get_name() + "\' because:\n" + reason;
  }

  std::string
  SetOperationFailed::fill(const char * op, const std::string& reason) noexcept
  {
    return std::string("method \'") + op + "()\' failed because:\n" + reason;
  }

}

OksClass::OksClass(const std::string& nm, OksKernel * k, bool t) :
  p_name		  (nm),
  p_super_classes	  (0),
  p_attributes		  (0),
  p_relationships	  (0),
  p_methods		  (0),
  p_abstract		  (false),
  p_transient		  (t),
  p_to_be_deleted         (false),
  p_all_super_classes	  (0),
  p_all_sub_classes	  (0),
  p_all_attributes	  (0),
  p_all_relationships	  (0),
  p_all_methods		  (0),
  p_inheritance_hierarchy (0),
  p_kernel		  (k),
  p_instance_size	  (0),
  p_data_info		  (0),
  p_objects		  (0),
  p_indices		  (0)
{
  OSK_PROFILING(OksProfiler::ClassConstructor, p_kernel)

  if(p_transient == false && p_kernel)
    {
      oks::validate_not_empty(p_name, "class name");

      std::unique_lock lock(p_kernel->p_kernel_mutex);
      p_kernel->k_add(this);
    }
}


OksClass::OksClass(const std::string& nm, const std::string& d, bool a, OksKernel * k, bool t) :
  p_name		  (nm),
  p_description		  (d),
  p_super_classes	  (0),
  p_attributes		  (0),
  p_relationships	  (0),
  p_methods		  (0),
  p_abstract		  (a),
  p_transient		  (t),
  p_to_be_deleted         (false),
  p_all_super_classes	  (0),
  p_all_sub_classes	  (0),
  p_all_attributes	  (0),
  p_all_relationships	  (0),
  p_all_methods		  (0),
  p_inheritance_hierarchy (0),
  p_kernel		  (k),
  p_instance_size	  (0),
  p_data_info		  (0),
  p_objects		  (0),
  p_indices		  (0)
{
  OSK_PROFILING(OksProfiler::ClassConstructor, p_kernel)

  if(p_transient == false && p_kernel)
    {
      oks::validate_not_empty(p_name, "class name");

      std::unique_lock lock(p_kernel->p_kernel_mutex);
      p_kernel->k_add(this);
    }
}


OksClass::OksClass (OksKernel * kernel, const std::string& name, const std::string& description, bool is_abstract) :
  p_name		  (name),
  p_description		  (description),
  p_super_classes	  (0),
  p_attributes		  (0),
  p_relationships	  (0),
  p_methods		  (0),
  p_abstract		  (is_abstract),
  p_transient		  (false),
  p_to_be_deleted         (false),
  p_all_super_classes	  (0),
  p_all_sub_classes	  (0),
  p_all_attributes	  (0),
  p_all_relationships	  (0),
  p_all_methods		  (0),
  p_inheritance_hierarchy (0),
  p_file                  (kernel->p_active_schema),
  p_kernel		  (kernel),
  p_instance_size	  (0),
  p_data_info		  (0),
  p_objects		  (0),
  p_indices		  (0)
{
  OSK_PROFILING(OksProfiler::ClassConstructor, p_kernel)
  p_kernel->p_classes[p_name.c_str()] = this;
}


void
OksClass::destroy(OksClass *c)
{
    // obtain write mutex, if non-transient class is registered in the oks kernel

  if(!c->p_transient && c->p_kernel) {
    std::unique_lock lock(c->p_kernel->p_kernel_mutex);

    try {
      c->p_file->lock();
      c->p_file->set_updated();
    }
    catch(oks::exception& ex) {
      throw oks::CannotDestroyClass(*c, ex);
    }

    delete c;
  }

    // destroy class without get write mutex

  else {
    delete c;
  }
}

template<class T>
  void
  OksClass::destroy_list(T list)
  {
    if (list)
      {
        for (const auto &x : *list)
          delete x;

        delete list;
      }
  }

template<class T>
  void
  OksClass::destroy_map(T map)
  {
    if (map)
      {
        for (const auto &x : *map)
          delete x.second;

        delete map;
      }
  }

OksClass::~OksClass()
{
  OSK_PROFILING(OksProfiler::ClassDestructor, p_kernel)

    TLOG_DEBUG(2) << "destruct " << (p_transient ? "TRANSIENT" : "NORMAL") << " class \'" << get_name() << '\'' ;

  // ~OksIndex() removes 'SELF' from OksClass and deletes 'indices' when there is no an index in it
  while (p_indices)
    delete (*(p_indices->begin())).second;

  destroy_map(p_objects);
  destroy_map(p_data_info);

  if (!p_transient && p_kernel)
    {
      p_kernel->k_remove(this);

      if (p_all_super_classes)
        for (const auto &c : *p_all_super_classes)
          {
            if (p_kernel->is_dangling(c) || c->p_to_be_deleted)
              continue;

            c->create_sub_classes();

            if (OksClass::change_notify_fn)
              (*OksClass::change_notify_fn)(c, ChangeSubClassesList, (const void*) &p_name);
          }

      if (p_all_sub_classes)
        for (const auto &c : *p_all_sub_classes)
          {
            if (p_kernel->is_dangling(c) || c->p_to_be_deleted)
              continue;

            try
              {
                c->registrate_class_change(ChangeSuperClassesList, (const void*) &p_name, false);
              }
            catch (oks::exception &ex)
              {
                std::cerr << "internal OKS error: registrate_class_change() failed during \'" << get_name() << "\' class destruction\ncaused by: " << ex.what() << std::endl;
              }
          }
    }

  destroy_list(p_super_classes);
  destroy_list(p_attributes);
  destroy_list(p_relationships);
  destroy_list(p_methods);

  if (!p_transient)
    {
      delete p_all_super_classes;
      delete p_all_sub_classes;
      delete p_all_attributes;
      delete p_all_relationships;
      delete p_all_methods;
      delete p_inheritance_hierarchy;
    }
}


void
OksClass::set_file(OksFile * f, bool update_owner)
{
  if(p_file != f) {
    try {
      if(update_owner) {
        p_file->lock();
        p_file->set_updated();
      }

      f->lock();
      f->set_updated();

      p_file = f;
    }
    catch(oks::exception& ex) {
      throw oks::CanNotSetFile(this, 0, *f, ex);
    }
  }
}


bool
OksClass::compare_without_methods(const OksClass & c) const noexcept
{
  if(this == &c) return true;

  if(
	(p_name != c.p_name) ||
	(p_description != c.p_description) ||
	(p_abstract != c.p_abstract)
  ) return false;
  
  
  int l1 = (p_attributes == 0) ? 0 : p_attributes->size();
  int l2 = (c.p_attributes == 0) ? 0 : c.p_attributes->size();
  
  if(l1 != l2) return false;

  if(l1 > 0) {
    std::list<OksAttribute *>::const_iterator i1 = p_attributes->begin();
    std::list<OksAttribute *>::const_iterator i2 = c.p_attributes->begin();
    
    for(;i1 != p_attributes->end();++i1, ++i2)
      if( !(*(*i1) == *(*i2)) ) return false;
  }

  l1 = (p_relationships == 0) ? 0 : p_relationships->size();
  l2 = (c.p_relationships == 0) ? 0 : c.p_relationships->size();
 
  if(l1 != l2) return false;
  
  if(l1 > 0) {
    std::list<OksRelationship *>::const_iterator i1 = p_relationships->begin();
    std::list<OksRelationship *>::const_iterator i2 = c.p_relationships->begin();
    
    for(;i1 != p_relationships->end();++i1, ++i2)
      if( !(*(*i1) == *(*i2)) ) return false;
  }
  
  l1 = (p_super_classes == 0) ? 0 : p_super_classes->size();
  l2 = (c.p_super_classes == 0) ? 0 : c.p_super_classes->size();
 
  if(l1 != l2) return false;

  if(l1 > 0) {
    std::list<std::string *>::const_iterator i1 = p_super_classes->begin();
    std::list<std::string *>::const_iterator i2 = c.p_super_classes->begin();
    
    for(;i1 != p_super_classes->end();++i1, ++i2)
      if( !(*(*i1) == *(*i2)) ) return false;
  }
  
  return true;
}

bool
OksClass::operator!=(const OksClass &c) const
{
  if(this == &c) return false;

  if(compare_without_methods(c) == false) return true;
  
  int l1 = (p_methods == 0) ? 0 : p_methods->size();
  int l2 = (c.p_methods == 0) ? 0 : c.p_methods->size();
  
  if(l1 != l2) return true;

  if(l1 > 0) {
    std::list<OksMethod *>::const_iterator i1 = p_methods->begin();
    std::list<OksMethod *>::const_iterator i2 = c.p_methods->begin();
    
    for(;i1 != p_methods->end();++i1, ++i2)
      if( !(*(*i1) == *(*i2)) ) return true;
  }

  return false;
}


std::ostream&
operator<<(std::ostream& s, const OksClass& c)
{
  OSK_PROFILING(OksProfiler::ClassOperatorOut, c.p_kernel)

  s	<< "Class name: \"" << c.p_name << "\"\n"
	   " has description: \"" << c.p_description << "\"\n"
	<< (c.p_abstract == true ? " is abstract\n" : " is not abstract\n");

  if(c.p_super_classes) {
    s << "The class has superclasses:\n";
    for(std::list<std::string *>::const_iterator i = c.p_super_classes->begin(); i != c.p_super_classes->end(); ++i) {
      s << " - " << *(*i) << std::endl;
    }
  }
  else
    s << "The class has no superclasses\n";


  if(c.p_attributes) {
    s << "The attributes are:\n";
    for(std::list<OksAttribute *>::const_iterator i = c.p_attributes->begin(); i != c.p_attributes->end(); ++i) {
      s << *(*i);
    }
  }
  else
    s << "There are no attributes\n";


  if(c.p_relationships) {
    s << "The relationships are:\n";
    for(std::list<OksRelationship *>::const_iterator i = c.p_relationships->begin(); i != c.p_relationships->end(); ++i) {
      s << *(*i);
    }
 }
  else
    s << "There are no relationships\n";


  if(c.p_methods) {
    s << "The methods are:\n";
    for(std::list<OksMethod *>::const_iterator i = c.p_methods->begin(); i != c.p_methods->end(); ++i) {
      s << *(*i);
    }
  }
  else
    s << "There are no methods\n";


  if(c.p_objects) {
    OksObject::SMap sorted;

    s << "The objects are:\n";

    for(OksObject::Map::iterator i = c.p_objects->begin(); i != c.p_objects->end(); ++i) {
      sorted[i->first] = i->second;
    }

    for(OksObject::SMap::iterator i = sorted.begin(); i != sorted.end(); ++i) {
      s << *(i->second);
    }
  }
  else
    s << "There are no objects\n";


  s << "End of Class \"" << c.p_name << "\"\n";
  
  return s;
}


void
OksClass::save(OksXmlOutputStream& s) const
{
  try {
    s.put_raw(' ');
    s.put_start_tag(class_xml_tag, sizeof(class_xml_tag)-1);
    s.put_attribute(name_xml_attr, sizeof(name_xml_attr)-1, p_name.c_str());
  
    // store non-empty description only
    if (!p_description.empty())
      s.put_attribute(description_xml_attr, sizeof(description_xml_attr)-1, p_description.c_str());

    // store abstract attribute only when the class is abstract
    if (p_abstract)
      s.put_attribute(is_abstract_xml_attr, sizeof(is_abstract_xml_attr)-1, oks::xml::bool2str(p_abstract));

    s.put_raw('>');
    s.put_raw('\n');

    if (p_super_classes)
      for(const auto& i : *p_super_classes) {
        s.put_raw(' ');
        s.put_raw(' ');
        s.put_start_tag(superclass_xml_attr, sizeof(superclass_xml_attr)-1);
        s.put_attribute(name_xml_attr, sizeof(name_xml_attr)-1, i->c_str());
        s.put_end_tag();
      }

    if (p_attributes)
      for (const auto &i : *p_attributes)
        i->save(s);

    if (p_relationships)
      for (const auto &i : *p_relationships)
        i->save(s);

    if (p_methods)
      for (const auto &i : *p_methods)
        i->save(s);

    s.put_raw(' ');
    s.put_last_tag(class_xml_tag, sizeof(class_xml_tag)-1);
    s.put_raw('\n');
  }

  catch (oks::exception & e) {
    throw oks::FailedSave("oks-class", p_name, e);
  }

  catch (std::exception & e) {
   throw oks::FailedSave("oks-class", p_name, e.what());
  }

}


OksClass::OksClass(OksXmlInputStream& s, OksKernel * k) :
  p_super_classes	  (0),
  p_attributes		  (0),
  p_relationships	  (0),
  p_methods		  (0),
  p_abstract		  (false),
  p_transient		  (false),
  p_to_be_deleted         (false),
  p_all_super_classes	  (0),
  p_all_sub_classes	  (0),
  p_all_attributes	  (0),
  p_all_relationships	  (0),
  p_all_methods		  (0),
  p_inheritance_hierarchy (0),
  p_kernel		  (k),
  p_instance_size	  (0),
  p_data_info		  (0),
  p_objects		  (0),
  p_indices		  (0)
{

    // read 'relationship' tag header

  {
    try {
      const char * tag_start = s.get_tag_start();

      if(strcmp(tag_start, class_xml_tag)) {
        if(!strcmp(tag_start, "/oks-schema")) {
          goto end_of_stream; // re-throw of exception takes too much time; go to is better
	}
        s.throw_unexpected_tag(tag_start, class_xml_tag);
      }
    }
    catch (oks::exception & e) {
      throw oks::FailedRead("oks-class tag", e);
    }
    catch (std::exception & e) {
      throw oks::FailedRead("oks-class tag", e.what());
    }
  }

    // read class attributes

  try {
    while(true) {
      OksXmlAttribute attr(s);

        // check for close of tag

      if(oks::cmp_str1(attr.name(), ">")) { break; }

        // check for known oks-class' attributes

      else if(oks::cmp_str4(attr.name(), name_xml_attr)) p_name.assign(attr.value(), attr.value_len());
      else if(oks::cmp_str11(attr.name(), description_xml_attr)) p_description.assign(attr.value(), attr.value_len());
      else if(oks::cmp_str11(attr.name(), is_abstract_xml_attr)) p_abstract = oks::xml::str2bool(attr.value());
      else {
        s.throw_unexpected_tag(attr.name(), "oks-class\' attribute");
      }
    }
  }
  catch (oks::exception & e) {
    throw oks::FailedRead("oks-class description", e);
  }
  catch (std::exception & e) {
    throw oks::FailedRead("oks-class description", e.what());
  }


  try {
    oks::validate_not_empty(p_name, "class name");
  }
  catch(std::exception& ex) {
    throw oks::FailedRead("oks class", oks::BadFileData(ex.what(), s.get_line_no(), s.get_line_pos()));
  }


    // read class body

  while(true) {

    try {

      const char * tag_start = s.get_tag_start();

        // check for closing tag and exit loop

      if(oks::cmp_str6(tag_start, "/class")) { break; }


        // create superclass

      else if(oks::cmp_str10(tag_start, superclass_xml_attr)) {
        unsigned long scl_pos = s.get_line_no();
        try {
          OksXmlAttribute attr(s);

	  if(!p_super_classes) p_super_classes = new std::list<std::string *>();
	  std::string * scl = new std::string(attr.value(), attr.value_len());
          if(has_direct_super_class(*scl))
            {
              std::ostringstream text;
              text << "second inclusion of super-class \"" + *scl + "\" at line " << scl_pos;
              delete scl;
              throw std::runtime_error(text.str().c_str());
            }
	  p_super_classes->push_back(scl);

          OksXmlAttribute attr2(s);
        }
        catch (oks::exception & e) {
          throw oks::FailedRead(std::string("a superclass-name of class \"") + p_name + '\"', e);
        }
        catch (std::exception & e) {
          throw oks::FailedRead(std::string("a superclass-name of class \"") + p_name + '\"', e.what());
        }
      }


        // create attribute from stream and check attribute status

      else if(oks::cmp_str9(tag_start, "attribute")) {
        unsigned long attr_pos = s.get_line_no();
        try {
	  if(!p_attributes) p_attributes = new std::list<OksAttribute *>();
	  OksAttribute * a = new OksAttribute(s, this);
	  if(find_direct_attribute(a->get_name()) != nullptr)
	    {
	      std::ostringstream text;
	      text << "redefinition of attribute with name \"" + a->get_name() + "\" at line " << attr_pos;
              delete a;
	      throw std::runtime_error(text.str().c_str());
	    }
	  p_attributes->push_back(a);
        }
        catch (oks::exception & e) {
          throw oks::FailedRead(std::string("an attribute of class \"") + p_name + '\"', e);
        }
        catch (std::exception & e) {
          throw oks::FailedRead(std::string("an attribute of class \"") + p_name + '\"', e.what());
        }
      }


        // create relationship from stream and check relationship status

      else if(oks::cmp_str12(tag_start, "relationship")) {
        unsigned long rel_pos = s.get_line_no();
        try {
          if(!p_relationships) p_relationships= new std::list<OksRelationship *>();
          OksRelationship * r = new OksRelationship(s,this);
          if(find_direct_relationship(r->get_name()) != nullptr)
            {
              std::ostringstream text;
              text << "redefinition of relationship with name \"" + r->get_name() + "\" at line " << rel_pos;
              delete r;
              throw std::runtime_error(text.str().c_str());
            }
	  p_relationships->push_back(r);
        }
        catch (oks::exception & e) {
          throw oks::FailedRead(std::string("a relationship of class \"") + p_name + '\"', e);
        }
        catch (std::exception & e) {
          throw oks::FailedRead(std::string("a relationship of class \"") + p_name + '\"', e.what());
        }
      }


        // create method from stream and check method status

      else if(oks::cmp_str6(tag_start, "method")) {
        unsigned long method_pos = s.get_line_no();
        try {
	  if(!p_methods) p_methods = new std::list<OksMethod *>();
	  OksMethod * m = new OksMethod(s,this);
          if(find_direct_method(m->get_name()) != nullptr)
            {
              std::ostringstream text;
              text << "redefinition of method with name \"" + m->get_name() + "\" at line " << method_pos;
              delete m;
              throw std::runtime_error(text.str().c_str());
            }
	  p_methods->push_back(m);
        }
        catch (oks::exception & e) {
          throw oks::FailedRead(std::string("a method of class \"") + p_name + '\"', e);
        }
        catch (std::exception & e) {
          throw oks::FailedRead(std::string("a method of class \"") + p_name + '\"', e.what());
        }
      }


        // report error if red something differnt from above
        // and set bad status of constructor (suppose 'name' can't be empty)

      else {
        s.throw_unexpected_tag(tag_start);
      }

    }
    catch (oks::FailedRead & e) {
      throw;  // re-throw attribute, relationship or method exception
    }
    catch (oks::exception & e) {
      throw oks::FailedRead(std::string("a tag of class \"") + p_name + '\"', e);
    }
    catch (std::exception & e) {
      throw oks::FailedRead(std::string("a tag of class \"") + p_name + '\"', e.what());
    }

  }
  
  return;


end_of_stream:
  
  throw oks::EndOfXmlStream("oks-schema");
}


/******************************************************************************/
/****************************** OKS SUPERCLASSES ******************************/
/******************************************************************************/

OksClass *
OksClass::find_super_class(const std::string& nm) const noexcept
{
  if(p_all_super_classes && !p_all_super_classes->empty()) {
    for(FList::const_iterator i = p_all_super_classes->begin(); i != p_all_super_classes->end(); ++i) {
      if((*i)->get_name() == nm) return *i;
    }
  }

  return 0;
}

bool
OksClass::has_direct_super_class(const std::string& nm) const noexcept
{
  if(p_super_classes) {
    for(std::list<std::string *>::const_iterator i = p_super_classes->begin(); i != p_super_classes->end(); ++i)
      if(nm == *(*i)) return true;
  }

  return false;
}


void
OksClass::lock_file(const char * fname)
{
  if(p_kernel) {
    try {
      p_file->lock();
    }
    catch(oks::exception& ex) {
      throw oks::SetOperationFailed(fname, ex);
    }
  }
}


void
OksClass::set_description(const std::string& s)
{
  if(p_description != s) {
    lock_file("OksClass::set_description");

    p_description = s;

    if(p_kernel) {
      p_file->set_updated();
      if(OksClass::change_notify_fn) {
        (*OksClass::change_notify_fn)(this, ChangeDescription, 0);
      }
    }
  }
}


void
OksClass::set_is_abstract(bool b)
{
  if(p_abstract != b) {
    lock_file("OksClass::set_is_abstract");

    p_abstract = b;

    if(p_kernel) {
      p_file->set_updated();
      if(OksClass::change_notify_fn) {
        (*OksClass::change_notify_fn)(this, ChangeIsAbstaract, 0);
      }
    }
  }
}


  // Function to report problem with arguments of swap(...) methods

static void
check_and_report_empty_parameter(const char * fname, bool b1, bool b2)
{
  if(b1) {
    throw oks::SetOperationFailed(fname, "First parameter is (null)");
  }

  if(b2) {
    throw oks::SetOperationFailed(fname, "Second parameter is (null)");
  }
}


  // Function to report problem with found items of swap(...) methods

static void
check_and_report_found_items(const char * fname, const char * item_type,
                             const std::string& item1_name, const std::string& item2_name,
			     const std::string& class_name, bool b1, bool b2)
{
  struct {
    bool b;
    const std::string * name;
  } info[] = {
    {b1, &item1_name},
    {b2, &item2_name}
  };

  for(int i = 0; i < 2; ++i) {
    if(info[i].b) {
      std::string text("cannot find direct ");
      text += item_type;
      text += " \"";
      text += *info[i].name;
      text += "\" in class \"";
      text += class_name;
      text += '\"';
      throw oks::SetOperationFailed(fname, text);
    }
  }
}

static std::string
add_super_class_error(const std::string& c1, const std::string& c2)
{
  return (std::string("cannot add superclass \"") + c1 + "\" to class \"" + c2 + '\"');
}


void
OksClass::k_add_super_class(const std::string& nm)
{
  if(!p_super_classes) {
    p_super_classes = new std::list<std::string *>();
  }

  p_super_classes->push_back((new std::string(nm)));
}


void
OksClass::add_super_class(const std::string& nm)
{
  if(!p_super_classes) {
    p_super_classes = new std::list<std::string *>();
  }
  else {
    if( has_direct_super_class(nm) ) {
      std::string text(add_super_class_error(nm, p_name));
      text += " because class \"" + p_name + "\" already has such superclass.";
      throw oks::SetOperationFailed("OksClass::add_super_class", text);
    }
  }

  if(nm == p_name) {
    std::string text(add_super_class_error(nm, p_name));
    text += " because names of classes are the same.";
    throw oks::SetOperationFailed("OksClass::add_super_class", text);
  }

  if(p_kernel) {
    OksClass * nmc = p_kernel->find_class(nm);
    if(nmc) {
      if(nmc->has_direct_super_class(p_name)) {
        std::string text(add_super_class_error(nm, p_name));
        text += " because class \"" + nm + "\" is already direct superclass of \"" + p_name + "\".";
        throw oks::SetOperationFailed("OksClass::add_super_class", text);
      }
      else if(nmc->find_super_class(p_name)) {
        std::string text(add_super_class_error(nm, p_name));
        text += " because class \"" + nm + "\" is already non-direct superclass of \"" + p_name + "\".";
        throw oks::SetOperationFailed("OksClass::add_super_class", text);
      }
    }
  }

  lock_file("OksClass::add_super_class");

  p_super_classes->push_back((new std::string(nm)));

  registrate_class_change(ChangeSuperClassesList, (const void *)&nm);
}

static std::string
remove_super_class_error(const std::string& c1, const std::string& c2)
{
  return (std::string("cannot remove superclass \"") + c1 + "\" from class \"" + c2 + '\"');
}

void
OksClass::remove_super_class(const std::string& nm)
{
  if( !p_super_classes ) {
    std::string text(remove_super_class_error(nm, p_name));
    text += " because class \"" + p_name + "\" has no superclasses at all.";
    throw oks::SetOperationFailed("OksClass::remove_super_class", text);
  }


  for(std::list<std::string *>::iterator i = p_super_classes->begin(); i != p_super_classes->end(); ++i) {
    std::string * si(*i);
    if(nm == *si) {
      lock_file("OksClass::remove_super_class");

      p_super_classes->erase(i);
      delete si;

      if(p_super_classes->empty()) {
        delete p_super_classes;
        p_super_classes = 0;
      }

      registrate_class_change(ChangeSuperClassesList, (const void *)&nm);

      return;
    }
  }

  std::string text(remove_super_class_error(nm, p_name));
  text += " because class \"" + p_name + "\" has no superclass with this name.";
  throw oks::SetOperationFailed("OksClass::remove_super_class", text);
}


void
OksClass::swap_super_classes(const std::string& c1, const std::string& c2)
{
  const char * fname = "OksClass::swap_super_classes()";


    // if classs are equal, return success

  if(c1 == c2) return;


    // check that an class name is not empty

  check_and_report_empty_parameter(fname, c1.empty(), c2.empty());


    // find requested classes in the directed classs list

  std::list<std::string *>::iterator i1 = p_super_classes->end();
  std::list<std::string *>::iterator i2 = p_super_classes->end();

  for(std::list<std::string *>::iterator i = p_super_classes->begin(); i != p_super_classes->end(); ++i) {
    if(*(*i) == c1) i1 = i;
    else if(*(*i) == c2) i2 = i;
  }

    // check that the classes were found

  check_and_report_found_items(
    "OksClass::swap_super_classes", "superclass", c1, c2, p_name,
    (i1 == p_super_classes->end()), (i2 == p_super_classes->end())
  );


    // replace the classs and make notification

  lock_file("OksClass::swap_super_classes");

  std::string * s1 = *i1;
  std::string * s2 = *i2;

  *i1 = s2;
  *i2 = s1;

  registrate_class_change(ChangeSuperClassesList, 0);
}


/******************************************************************************/
/******************************* OKS ATTRIBUTES *******************************/
/******************************************************************************/

OksAttribute *
OksClass::find_direct_attribute(const std::string& _name) const noexcept
{
  if(p_attributes) {
    for(std::list<OksAttribute *>::const_iterator i = p_attributes->begin(); i != p_attributes->end(); ++i)
      if(_name == (*i)->get_name()) return *i;
  }

  return 0;
}


OksAttribute *
OksClass::find_attribute(const std::string& _name) const noexcept
{
  if(p_all_attributes) {
    for(std::list<OksAttribute *>::const_iterator i = p_all_attributes->begin(); i != p_all_attributes->end(); ++i)
      if(_name == (*i)->get_name()) return *i;
    
    return 0;
  }
  else
    return find_direct_attribute(_name);
}


void
OksClass::k_add(OksAttribute * attribute)
{
  if(!p_attributes) {
    p_attributes = new std::list<OksAttribute *>();
  }

  p_attributes->push_back(attribute);
  attribute->p_class = this;
}


void
OksClass::add(OksAttribute * attribute)
{
  const char * fname = "OksClass::add(OksAttribute *)";

  if(!p_attributes) {
    p_attributes = new std::list<OksAttribute *>();
  }
  else {
    if(find_direct_attribute(attribute->get_name()) != 0) {
      std::ostringstream text;
      text << "cannot add attribute \"" << attribute->get_name() << "\" to class \"" << p_name << "\"\n"
	      "because the class already has attribute with this name.\n";
      throw oks::SetOperationFailed(fname, text.str());
    }
  }

  lock_file("OksClass::add[OksAttribute]");

  p_attributes->push_back(attribute);
  attribute->p_class = this;

  registrate_class_change(ChangeAttributesList, (const void *)attribute);
}


void
OksClass::swap(const OksAttribute * a1, const OksAttribute * a2)
{
  const char * fname = "OksClass::swap(const OksAttribute *, const OksAttribute *)";


    // if attributes are equal, return success

  if(a1 == a2) return;


    // check that an attributes is not (null)

  check_and_report_empty_parameter(fname, (a1 == 0), (a2 == 0));


    // find requested attributes in the directed attributes list

  std::list<OksAttribute *>::iterator i1 = p_attributes->end();
  std::list<OksAttribute *>::iterator i2 = p_attributes->end();

  for(std::list<OksAttribute *>::iterator i = p_attributes->begin(); i != p_attributes->end(); ++i) {
    if((*i) == a1) i1 = i;
    else if((*i) == a2) i2 = i;
  }

    // check that the attributes were found

  check_and_report_found_items(
    fname, "attribute", a1->get_name(), a2->get_name(), p_name,
    (i1 == p_attributes->end()), (i2 == p_attributes->end())
  );


    // check that the file can be locked

  lock_file("OksClass::swap[OksAttribute]");


    // replace the attributes and make notification

  *i1 = const_cast<OksAttribute *>(a2);
  *i2 = const_cast<OksAttribute *>(a1);

  registrate_class_change(ChangeAttributesList, 0);
}

void
OksClass::remove(const OksAttribute *attribute)
{
  const char * fname = "OksClass::remove(OksAttribute *)";

  if(!p_attributes) {
    std::ostringstream text;
    text << "cannot remove attribute \"" << attribute->get_name() << "\" from class \"" << p_name << "\"\n"
	    "because the class has no attributes.\n";
    throw oks::SetOperationFailed(fname, text.str());
  }

  for(std::list<OksAttribute *>::iterator i = p_attributes->begin(); i != p_attributes->end(); ++i) {
    if(attribute == *i) {
      lock_file("OksClass::remove[OksAttribute]");

      p_attributes->erase(i);
      delete const_cast<OksAttribute *>(attribute);

      if(p_attributes->empty()) {
        delete p_attributes;
        p_attributes = 0;
      }

      registrate_class_change(ChangeAttributesList, 0);

      return;
    }
  }

  std::ostringstream text;
  text << "cannot remove attribute \"" << attribute->get_name() << "\" from class \"" << p_name << "\"\n"
          "because the class has no such attribute with this name.\n";
  throw oks::SetOperationFailed(fname, text.str());
}


OksClass *
OksClass::source_class(const OksAttribute *a) const noexcept
{
  return a->p_class;
}


/******************************************************************************/
/***************************** OKS  RELATIONSHIPS *****************************/
/******************************************************************************/

OksRelationship *
OksClass::find_direct_relationship(const std::string& _name) const noexcept
{
  if(p_relationships) {
    for(std::list<OksRelationship *>::const_iterator i = p_relationships->begin(); i != p_relationships->end(); ++i)
      if(_name == (*i)->get_name()) return *i;
  }

  return 0;
}

OksRelationship *
OksClass::find_relationship(const std::string& _name) const noexcept
{
  if(p_all_relationships) {
    for(std::list<OksRelationship *>::const_iterator i = p_all_relationships->begin(); i != p_all_relationships->end(); ++i)
      if(_name == (*i)->get_name()) return *i;
  
    return 0;
  }
  else
    return find_direct_relationship(_name);
}

void
OksClass::k_add(OksRelationship * relationship)
{
  if(!p_relationships) {
    p_relationships = new std::list<OksRelationship *>();
  }

  p_relationships->push_back(relationship);
  relationship->p_class = this;
}

void
OksClass::add(OksRelationship * relationship)
{
  const char * fname = "OksClass::add(OksRelationship *)";

  if(!p_relationships) {
    p_relationships = new std::list<OksRelationship *>();
  }
  else {
    if(find_direct_relationship(relationship->get_name())) {
      std::ostringstream text;
      text << "cannot add relationship \"" << relationship->get_name() << "\" to class \"" << p_name << "\"\n"
	      "because the class already has relationship with this name.\n";
      throw oks::SetOperationFailed(fname, text.str());
    }
  }

  lock_file("OksClass::add[OksRelationship]");

  p_relationships->push_back(relationship);
  relationship->p_class = this;
  
  registrate_class_change(ChangeRelationshipsList, (const void *)relationship);
}

void
OksClass::remove(const OksRelationship * relationship, bool call_delete)
{
  const char * fname = "OksClass::remove(OksRelationship *)";
  
  if(!p_relationships) {
    std::ostringstream text;
    text << "cannot remove relationship \"" << relationship->get_name() << "\" from class \"" << p_name << "\"\n"
	    "because the class has no relationships.\n";
    throw oks::SetOperationFailed(fname, text.str());
  }

  for(std::list<OksRelationship *>::iterator i = p_relationships->begin(); i != p_relationships->end(); ++i) {
    if(relationship == *i) {
      lock_file("OksClass::remove[OksRelationship]");

      p_relationships->erase(i);
      if(call_delete) {
        delete const_cast<OksRelationship *>(relationship);
      }

      if(p_relationships->empty()) {
        delete p_relationships;
        p_relationships = 0;
      }
  
      registrate_class_change(ChangeRelationshipsList, 0);

      return;
    }
  }

  std::ostringstream text;
  text << "cannot remove relationship \"" << relationship->get_name() << "\" from class \"" << p_name << "\"\n"
          "because the class has no such relationship with this name.\n";
  throw oks::SetOperationFailed(fname, text.str());
}


void
OksClass::swap(const OksRelationship * r1, const OksRelationship * r2)
{
  const char * fname = "OksClass::swap(const OksRelationship *, const OksRelationship *)";


    // if relationships are equal, return success

  if(r1 == r2) return;


    // check that an relationships is not (null)

  check_and_report_empty_parameter(fname, (r1 == 0), (r2 == 0));


    // find requested relationships in the directed relationships list

  std::list<OksRelationship *>::iterator i1 = p_relationships->end();
  std::list<OksRelationship *>::iterator i2 = p_relationships->end();

  for(std::list<OksRelationship *>::iterator i = p_relationships->begin(); i != p_relationships->end(); ++i) {
    if((*i) == r1) i1 = i;
    else if((*i) == r2) i2 = i;
  }

    // check that the relationships were found

  check_and_report_found_items(
    fname, "relationship", r1->get_name(), r2->get_name(), p_name,
    (i1 == p_relationships->end()), (i2 == p_relationships->end())
  );

    // check that the file can be locked

  lock_file("OksClass::swap[OksRelationship]");

    // replace the relationships and make notification

  *i1 = const_cast<OksRelationship *>(r2);
  *i2 = const_cast<OksRelationship *>(r1);

  registrate_class_change(ChangeRelationshipsList, 0);
}


OksClass*
OksClass::source_class(const OksRelationship *r) const noexcept
{
  return r->p_class;
}


/******************************************************************************/
/******************************** OKS  METHODS ********************************/
/******************************************************************************/


OksMethod *
OksClass::find_direct_method(const std::string& _name) const noexcept
{
  if(p_methods) {
    for(std::list<OksMethod *>::const_iterator i = p_methods->begin(); i != p_methods->end(); ++i)
      if(_name == (*i)->get_name()) return *i;
  }

  return 0;
}

OksMethod *
OksClass::find_method(const std::string& _name) const noexcept
{
  if(p_all_methods) {
    for(std::list<OksMethod *>::const_iterator i = p_all_methods->begin(); i != p_all_methods->end(); ++i)
      if(_name == (*i)->get_name()) return *i;

    return 0;
  }
  else 
    return find_direct_method(_name);
}
  
void
OksClass::add(OksMethod * method)
{
  const char * fname = "OksClass::add(OksMethod *)";

  if(!p_methods) {
    p_methods = new std::list<OksMethod *>();
  }
  else {
    if(find_direct_method(method->get_name())) {
      std::ostringstream text;
      text << "cannot add method \"" << method->get_name() << "\" to class \"" << p_name << "\"\n"
	      "because the class already has method with this name.\n";
      throw oks::SetOperationFailed(fname, text.str());
    }
  }

    // check that the file can be locked

  lock_file("OksClass::add[OksMethod]");

  p_methods->push_back(method);
  method->p_class = this;

  registrate_class_change(ChangeMethodsList, (const void *)method);
}

void
OksClass::remove(const OksMethod * method)
{
  const char * fname = "OksClass::remove(OksMethod *)";

  if(!p_methods) {
    std::ostringstream text;
    text << "cannot remove method \"" << method->get_name() << "\" from class \"" << p_name << "\"\n"
	    "because the class has no methods.\n";
    throw oks::SetOperationFailed(fname, text.str());
  }

  for(std::list<OksMethod *>::iterator i = p_methods->begin(); i != p_methods->end(); ++i) {
    if(method == *i) {
      lock_file("OksClass::remove[OksMethod]");

      p_methods->erase(i);
      delete const_cast<OksMethod *>(method);

      if(p_methods->empty()) {
        delete p_methods;
        p_methods = 0;
      }

      registrate_class_change(ChangeMethodsList, 0);

      return;
    }
  }

  std::ostringstream text;
  text << "cannot remove method \"" << method->get_name() << "\" from class \"" << p_name << "\"\n"
          "because the class has no such method with this name.\n";
  throw oks::SetOperationFailed(fname, text.str());
}

void
OksClass::swap(const OksMethod * m1, const OksMethod * m2)
{
  const char * fname = "OksClass::swap(const OksMethod *, const OksMethod *)";


    // if methods are equal, return success

  if(m1 == m2) return;


    // check that an methods is not (null)

  check_and_report_empty_parameter(fname, (m1 == 0), (m2 == 0));


    // find requested methods in the directed methods list

  std::list<OksMethod *>::iterator i1 = p_methods->end();
  std::list<OksMethod *>::iterator i2 = p_methods->end();

  for(std::list<OksMethod *>::iterator i = p_methods->begin(); i != p_methods->end(); ++i) {
    if((*i) == m1) i1 = i;
    else if((*i) == m2) i2 = i;
  }

    // check that the methods were found

  check_and_report_found_items(
    fname, "method", m1->get_name(), m2->get_name(), p_name,
    (i1 == p_methods->end()), (i2 == p_methods->end())
  );

    // check that the file can be locked

  lock_file("OksClass::swap[OksMethod]");

    // replace the methods and make notification

  *i1 = const_cast<OksMethod *>(m2);
  *i2 = const_cast<OksMethod *>(m1);

  registrate_class_change(ChangeMethodsList, 0);
}


OksClass*
OksClass::source_class(const OksMethod *m) const noexcept
{
  return m->p_class;
}


/******************************************************************************/
/******************************** OKS  OBJECTS ********************************/
/******************************************************************************/

size_t
OksClass::number_of_objects() const noexcept
{
  return (!p_objects ? 0 : p_objects->size());
}


	//
	// Creates list of class instances and instances of
	// all subclasses
	//

std::list<OksObject *> *
OksClass::create_list_of_all_objects() const noexcept
{
  std::list<OksObject *> * olist = 0;

    // add instances of class

  if(p_objects && !p_objects->empty()) {
    olist = new std::list<OksObject *>();

    for(OksObject::Map::const_iterator i = p_objects->begin(); i != p_objects->end(); ++i)
      olist->push_back((*i).second);
  }

    // build iterator over subclasses

  if(p_all_sub_classes) {
    for(FList::const_iterator j = p_all_sub_classes->begin(); j != p_all_sub_classes->end(); ++j) {
      OksClass *sc = *j;

         // add instances of subclass

      if(sc->p_objects && !sc->p_objects->empty()) {
        if(!olist) olist = new std::list<OksObject *>();

        for(OksObject::Map::const_iterator i = sc->p_objects->begin(); i != sc->p_objects->end(); ++i)
          olist->push_back((*i).second);
      }
    }
  }

  return olist;
}


OksObject*
OksClass::get_object(const std::string& id) const noexcept
{
  std::shared_lock lock(p_mutex);  // protect p_objects

  if(p_objects) {
    OksObject::Map::const_iterator i = p_objects->find(&id);
    if(i != p_objects->end()) return i->second;
  }

  return nullptr;
}


/******************************************************************************/
/****************************** PRIVATE  METHODS ******************************/
/******************************************************************************/

void
OksClass::add(OksObject *object)
{
  std::unique_lock lock(p_mutex);  // protect p_objects

  if(!p_objects->insert(std::pair<const std::string *,OksObject *>(&object->uid.object_id,object) ).second) {
    throw oks::ObjectOperationFailed(*this, object->uid.object_id, "add", "object already exists");
  }
}

void
OksClass::remove(OksObject *object)
{
  std::unique_lock lock(p_mutex);  // protect p_objects

  if(!p_objects) {
    throw oks::ObjectOperationFailed(*this, object->uid.object_id, "remove", "OKS Kernel is not inited");
  }

  OksObject::Map::iterator i = p_objects->find(&object->uid.object_id);

  if(i != p_objects->end()) p_objects->erase(i);
  else {
    throw oks::ObjectOperationFailed(*this, object->uid.object_id, "remove", "object does not exist");
  }
}

inline void add_if_not_found(OksClass::FList& clist, OksClass *c)
{
  for(OksClass::FList::const_iterator i = clist.begin(); i != clist.end(); ++i) {
    if(*i == c) return;
  }
  clist.push_back(c);
}


void
OksClass::add_super_classes(FList * clist) const
{
  if(p_super_classes) {
    for(std::list<std::string *>::const_iterator i = p_super_classes->begin(); i != p_super_classes->end(); ++i) {
      if(OksClass * c = p_kernel->find_class(**i)) {
        c->add_super_classes(clist);
	add_if_not_found(*clist, c);
      }
      else {
        throw oks::CannotFindSuperClass(*this, **i);
      }
    }
  }
}


void
OksClass::create_super_classes()
{
  if(!p_all_super_classes) p_all_super_classes = new FList();
  else p_all_super_classes->clear();

  add_super_classes(p_all_super_classes);

#ifndef ERS_NO_DEBUG
  if(ers::debug_level() >= 5) {
    std::ostringstream text;
    text << "found " << p_all_super_classes->size() << " superclass(es) in class \'" << get_name() << "\':\n";
    if(p_all_super_classes->size()) {
      for(FList::iterator i = p_all_super_classes->begin(); i != p_all_super_classes->end(); ++i) {
        text << " - class \'" << (*i)->get_name() << "\'\n";
      }
    }

    TLOG_DEBUG(5) << text.str();
  }
#endif
}


void
OksClass::create_sub_classes()
{
  if(!p_all_sub_classes) p_all_sub_classes = new FList();
  else p_all_sub_classes->clear();

  if(!p_kernel->p_classes.empty()) {
    for(Map::iterator i = p_kernel->p_classes.begin(); i != p_kernel->p_classes.end(); ++i) {
      OksClass *c = i->second;

      if(const FList* scl = c->p_all_super_classes) {
        for(FList::const_iterator j = scl->begin(); j != scl->end(); ++j) {
          if(*j == this) { p_all_sub_classes->push_back(c); break; }
        }
      }
    }
  }
}


inline const char *
bool2value_type(bool v)
{
  return (v ? "multi-value" : "single-value");
}

  // skip differency between enum and string

inline bool
are_types_different(const OksAttribute * a1, const OksAttribute * a2)
{
  static const std::string __enum_str__("enum");
  static const std::string __string_str__("string");

  const std::string& s1 = a1->get_type();
  const std::string& s2 = a2->get_type();

  return (
    !((s1 == s2) || ((s1 == __enum_str__ && s2 == __string_str__) || (s1 == __string_str__ && s2 == __enum_str__)))
  );
}

void
OksClass::create_attributes()
{
  if(p_attributes)
    for(const auto& x : *p_attributes)
      x->set_init_data();

  if(!p_all_attributes) p_all_attributes = new std::list<OksAttribute *>();
  else p_all_attributes->clear();

  if(p_all_super_classes) {
    for(FList::iterator i = p_all_super_classes->begin(); i != p_all_super_classes->end(); ++i) {
      OksClass * c(*i);
      if(c->p_attributes) {
        for(std::list<OksAttribute *>::iterator i2 = c->p_attributes->begin(); i2 != c->p_attributes->end(); ++i2) {
          OksAttribute * a(*i2);
          OksAttribute * a1 = find_direct_attribute(a->get_name());
          OksAttribute * a2 = find_attribute(a->get_name());
          if( a1 == 0 && a2 == 0 ) {
	    p_all_attributes->push_back(a);
	  }
          else if( !p_kernel->p_silence ) {
            if(a1) {
              TLOG_DEBUG(1) << "in class \'" << get_name() << "\' direct attribute \'" << a1->get_name() <<
                           "\' overrides one coming from superclass \'" << a->p_class->get_name() << '\'';
              if(are_types_different(a1, a)) {
                Oks::warning_msg("OksClass::create_attributes()")
                  << "  found attribute \'" << a->get_name() << "\' types conflict in class \'" << get_name() << "\':\n"
                  << "  type of attribute in superclass \'" << a->p_class->get_name() << "\' is \'" << a->get_type() << "\'\n"
                  << "  type of attribute in class \'" << get_name() << "\' is \'" << a1->get_type() << "\'\n";
              }

              if(a1->get_is_multi_values() != a->get_is_multi_values()) {
                Oks::warning_msg("OksClass::create_attributes()")
                  << "  found attribute \'" << a->get_name() << "\' types conflict in class \'" << get_name() << "\':\n"
                  << "  attribute in superclass \'" << a->p_class->get_name() << "\' is " << bool2value_type(a->get_is_multi_values()) << "\n"
                  << "  attribute in class \'" << get_name() << "\' is \'" << bool2value_type(a1->get_is_multi_values()) << "\'\n";
              }
            }
            else if(a2) { 
              TLOG_DEBUG(1) << "in class \'" << get_name() << "\' attribute \'" << a2->get_name() <<
                           "\' from superclass \'" << a2->p_class->get_name() << "\' overrides one coming from superclass \'" << a->p_class->get_name() << '\'';
              if(are_types_different(a2, a)) {
                Oks::warning_msg("OksClass::create_attributes()")
                  << "  found attribute \'" << a->get_name() << "\' types conflict in class \'" << get_name() << "\':\n"
                  << "  type of attribute in superclass \'" << a->p_class->get_name() << "\' is \'" << a->get_type() << "\'\n"
                  << "  type of attribute in superclass \'" << a2->p_class->get_name() << "\' is \'" << a2->get_type() << "\'\n";
              }

              if(a2->get_is_multi_values() != a->get_is_multi_values()) {
                Oks::warning_msg("OksClass::create_attributes()")
                  << "  found attribute \'" << a->get_name() << "\' types conflict in class \'" << get_name() << "\':\n"
                  << "  attribute in superclass \'" << a->p_class->get_name() << "\' is " << bool2value_type(a->get_is_multi_values()) << "\n"
                  << "  attribute in superclass \'" << get_name() << "\' is \'" << bool2value_type(a2->get_is_multi_values()) << "\'\n";
              }
            }
          }
        }
      }
    }
  }

  if(p_attributes) {
    for(std::list<OksAttribute *>::iterator i = p_attributes->begin(); i != p_attributes->end(); ++i) {
      p_all_attributes->push_back(*i);
    }
  }
}



inline const char *
card2string(OksRelationship::CardinalityConstraint cc) 
{
  return (cc == OksRelationship::One ? "one object" : "many objects");
}

void
OksClass::create_relationships()
{
  if(!p_all_relationships ) p_all_relationships = new std::list<OksRelationship *>();
  else p_all_relationships->clear();

  if(p_all_super_classes) {
    for(FList::iterator i = p_all_super_classes->begin(); i != p_all_super_classes->end(); ++i) {
      OksClass * c(*i);
      if(c->p_relationships) {
        for(std::list<OksRelationship *>::iterator i2 = c->p_relationships->begin(); i2 != c->p_relationships->end(); ++i2) {
          OksRelationship * r = *i2;
          OksRelationship * r1 = find_direct_relationship(r->get_name());
          OksRelationship * r2 = find_relationship(r->get_name());

	  if(!r->p_class_type) { r->p_class_type = p_kernel->find_class(r->p_rclass); }

          if( r1 == 0 && r2 == 0) {
	    p_all_relationships->push_back(r);
	  }
          else {
            if(r1) {
              TLOG_DEBUG(1) << "in class \'" << get_name() << "\' direct relationship \'" << r1->get_name() <<
                           "\' overrides one coming from superclass \'" << r->p_class->get_name() << '\'';
              if(r1->get_high_cardinality_constraint() != r->get_high_cardinality_constraint()) {
                Oks::warning_msg("OksClass::create_relationships()")
                  << "  found relationship \'" << r->get_name() << "\' cardinality conflict in class \'" << get_name() << "\':\n"
                  << "  relationship in superclass \'" << r->p_class->get_name() << "\' allows " << card2string(r->get_high_cardinality_constraint()) << "\n"
                  << "  relationship in class \'" << get_name() << "\' allows " <<  card2string(r1->get_high_cardinality_constraint()) << std::endl;
              }
            }
            else if(r2) {
              TLOG_DEBUG(1) << "in class \'" << get_name() << "\' relationship \'" << r2->get_name() <<
                           "\' from superclass \'" << r2->p_class->get_name() << "\' overrides one coming from superclass \'" << r->p_class->get_name() << '\'';
              if(r2->get_high_cardinality_constraint() != r->get_high_cardinality_constraint()) {
                Oks::warning_msg("OksClass::create_relationships()")
                  << "  found relationship \'" << r->get_name() << "\' cardinality conflict in class \'" << get_name() << "\':\n"
                  << "  relationship in superclass \'" << r->p_class->get_name() << "\' allows " << card2string(r->get_high_cardinality_constraint()) << "\n"
                  << "  relationship in superclass \'" << r2->p_class->get_name() << "\' allows " <<  card2string(r2->get_high_cardinality_constraint()) << std::endl;
              }
            }
          }
        }
      }
    }
  }

  if(p_relationships) {
    for(std::list<OksRelationship *>::iterator i = p_relationships->begin(); i != p_relationships->end(); ++i) {
      OksRelationship * r = *i;

      if(!r->p_class_type) {
        r->p_class_type = p_kernel->find_class(r->p_rclass);
      }

      p_all_relationships->push_back(r);
    }
  }
}


void
OksClass::create_methods()
{
  if(!p_all_methods) p_all_methods = new std::list<OksMethod *>();
  else p_all_methods->clear();

  if(p_all_super_classes) {
    for(FList::iterator i = p_all_super_classes->begin(); i != p_all_super_classes->end(); ++i) {
      std::list<OksMethod *>::iterator i2;
      if((*i)->p_methods) {
        for(i2 = (*i)->p_methods->begin(); i2 != (*i)->p_methods->end(); ++i2) {
          if(
            find_direct_method((*i2)->get_name()) == 0 &&
            find_method((*i2)->get_name()) == 0
          ) {
	    p_all_methods->push_back(*i2);
	  }
        }
      }
    }
  }

  if(p_methods) {
    for(std::list<OksMethod *>::iterator i = p_methods->begin(); i != p_methods->end(); ++i) {
      p_all_methods->push_back(*i);
    }
  }
}


    // Updates the values of the instances of the class and all their subclasses
    // in case of a change of the attribute's type or multi values cardinality

void
OksClass::registrate_attribute_change(OksAttribute *a)
{
  OSK_PROFILING(OksProfiler::ClassRegistrateAttributeChange, p_kernel)

  FList::iterator i;
  if(p_all_sub_classes) i = p_all_sub_classes->begin();

  OksClass * c(this);

  do {
    if(c->p_objects && !c->p_objects->empty()) {
      for(OksObject::Map::const_iterator i2 = c->p_objects->begin(); i2 != c->p_objects->end(); ++i2) {
        OksObject *o(i2->second);
        OksData newData;
        OksData *oldData = &o->data[((*(c->p_data_info))[a->p_name])->offset];

        try {
          oldData->cvt(&newData, a);
          oldData->Clear();
          memcpy(static_cast<void *>(oldData), static_cast<void *>(&newData), sizeof(OksData));
          newData.Clear2(); // Do not free !
	}
	catch(oks::AttributeReadError & ex) {
	  std::ostringstream text;
	  text << "attribute \'" << a->get_name() << "\' change converting object " << o << ' ';
	  throw oks::CannotRegisterClass(*this, text.str(), ex);
	}
      }
    }
  } while(p_all_sub_classes && i != p_all_sub_classes->end() && (c = *(i++)));
}


    // Updates the values of the instances of the class and all their subclasses
    // in case of a change of the relationship's class type or a cardinality

void
OksClass::registrate_relationship_change(OksRelationship *r)
{
  OSK_PROFILING(OksProfiler::ClassRegistrateRelationshipChange, p_kernel)

  FList::iterator i;
  if(p_all_sub_classes) i = p_all_sub_classes->begin();

  OksClass * c(this);

  do {
    if(c->p_objects && !c->p_objects->empty()) {
      for(OksObject::Map::const_iterator i2 = c->p_objects->begin(); i2 != c->p_objects->end(); ++i2) {
        OksObject *o(i2->second);
        OksData newData;
        OksData *oldData = &o->data[((*(c->p_data_info))[r->p_name])->offset];

        oldData->ConvertTo(&newData, r);
        oldData->Clear();
        memcpy(static_cast<void *>(oldData), static_cast<void *>(&newData), sizeof(OksData));
        newData.Clear2(); // Do not free !
      }
    }
  } while(p_all_sub_classes && i != p_all_sub_classes->end() && (c = *(i++)));
}


void
OksClass::registrate_instances()
{
  OSK_PROFILING(OksProfiler::ClassRegistrateInstances, p_kernel)

  OksDataInfo::Map * dInfo = new OksDataInfo::Map();

  size_t dInfoLength = 0;
  bool thereAreChanges = false;
		
  if(!p_all_attributes->empty()) {
    for(std::list<OksAttribute *>::iterator i = p_all_attributes->begin(); i != p_all_attributes->end(); ++i) {
      OksAttribute *a = *i;
      if(!thereAreChanges) {
        OksDataInfo::Map::const_iterator x = p_data_info->find(a->get_name());
	if( x == p_data_info->end() || x->second->attribute == nullptr || x->second->offset != dInfoLength || !(*(x->second->attribute) == *a) ) {
	  thereAreChanges = true;
	}
      }

      (*dInfo)[a->get_name()] = new OksDataInfo(dInfoLength++, a);
    }
  }
      
  if(!p_all_relationships->empty()) {
    for(std::list<OksRelationship *>::iterator i = p_all_relationships->begin(); i != p_all_relationships->end(); ++i) {
      OksRelationship *r = *i;
      if(!thereAreChanges) {
        OksDataInfo::Map::const_iterator x = p_data_info->find(r->get_name());
	if( x == p_data_info->end() || x->second->relationship == nullptr || x->second->offset != dInfoLength || !(*(x->second->relationship) == *r) ) {
	  thereAreChanges = true;
	}
      }

      (*dInfo)[r->get_name()] = new OksDataInfo(dInfoLength++, r);
    }
  }

  if(thereAreChanges == true) {
    if(p_objects && !p_objects->empty()) {
      for(OksObject::Map::const_iterator i = p_objects->begin(); i != p_objects->end(); ++i) {
        OksObject	*o = (*i).second;
        OksData * data = new OksData[dInfoLength];
        size_t count = 0;

        if(!p_all_attributes->empty()) {
      	  for(std::list<OksAttribute *>::iterator i2 = p_all_attributes->begin(); i2 != p_all_attributes->end(); ++i2) {
            OksAttribute * a = *i2;
	  
	    OksDataInfo::Map::const_iterator x = p_data_info->find(a->get_name());

            if(x != p_data_info->end() && x->second->attribute) {
              OksData *oldData(&o->data[x->second->offset]);

              if(*(x->second->attribute) == *a) {
                memcpy(static_cast<void *>(&data[count++]), static_cast<void *>(oldData), sizeof(OksData));
                oldData->type = OksData::unknown_type;
              }
              else {
	        try {
                  oldData->cvt(&data[count++], a);
                }
	        catch(oks::AttributeReadError & ex) {
	          throw oks::AttributeConversionFailed(*a, o, ex);
	        }
              }
            }
      	  }
        }

        if(!p_all_relationships->empty()) {
      	  for(std::list<OksRelationship *>::iterator i2 = p_all_relationships->begin(); i2 != p_all_relationships->end(); ++i2) {
            OksRelationship *r = *i2;

            OksDataInfo::Map::const_iterator x = p_data_info->find(r->get_name());

            if(x != p_data_info->end() && x->second->relationship) {
              OksData *oldData = &o->data[x->second->offset];

              if(*(x->second->relationship) == *r) {
                memcpy(static_cast<void *>(&data[count++]), static_cast<void *>(oldData), sizeof(OksData));
                oldData->type = OksData::unknown_type;
              }
              else
                oldData->ConvertTo(&data[count++], r);
            }
      	  }
        }

        int n = p_instance_size;
        while(n--) o->data[n].Clear();
        delete o->data;

        o->data = data;
      }
    }
  }
		
  if(!p_data_info->empty()) {
    for(OksDataInfo::Map::iterator i = p_data_info->begin(); i != p_data_info->end(); ++i) {
      delete i->second;
    }
  }

  delete p_data_info;
		
  p_data_info = dInfo;
  p_instance_size = dInfoLength;
}


void
OksClass::registrate_class(bool skip_registered)
{
  OSK_PROFILING(OksProfiler::ClassRegistrateClass, p_kernel)

  {
    if(!p_data_info) p_data_info = new OksDataInfo::Map();
    if(!p_objects) {
      p_objects = new OksObject::Map( (p_abstract == false) ? 1024 : 1 );
    }
  }

  if(!p_data_info->empty() && skip_registered) {
    TLOG_DEBUG(4) << "skip already registered " << get_name();
    return;
  }

  try {
    create_super_classes();
    create_attributes();
    create_relationships();
    create_methods();

    registrate_instances();
  }
  catch(oks::exception& ex) {
    throw oks::CannotRegisterClass(*this, "", ex);
  }
}


void
OksClass::registrate_class_change(ChangeType changeType, const void *parameter, bool update_file)
{
  if(!p_kernel) return;

  OSK_PROFILING(OksProfiler::ClassRegistrateClassChange, p_kernel)

  try {
    if(update_file) p_file->set_updated();

    FList superclasses;

    if(changeType == ChangeSuperClassesList && !p_all_super_classes->empty())
      for(FList::iterator i2 = p_all_super_classes->begin(); i2 != p_all_super_classes->end(); ++i2) {
        superclasses.push_back(*i2);
      }

    FList::iterator i;
    if(p_all_sub_classes) i = p_all_sub_classes->begin();

    OksClass *c = this;

    do {
      switch(changeType) {
        case ChangeSuperClassesList:
          c->create_super_classes();			
          c->create_attributes();
          c->create_relationships();
          c->create_methods();
          break;
  
        case ChangeAttributesList:
          c->create_attributes();
          break;
  
        case ChangeRelationshipsList:
          c->create_relationships();
          break;
  
        case ChangeMethodsList:
          c->create_methods();
          break;
			
        default:
          continue;
      }
	
      if(changeType != ChangeMethodsList) c->registrate_instances();
    } while(p_all_sub_classes && i != p_all_sub_classes->end() && (c = *(i++)));


    if(changeType == ChangeSuperClassesList) {
      if(!p_all_super_classes->empty())
        for(FList::iterator i2 = p_all_super_classes->begin(); i2 != p_all_super_classes->end(); ++i2)
          if(find(superclasses.begin(), superclasses.end(), *i2) == superclasses.end()) {
	    superclasses.push_back(*i2);
	  }

      for(FList::const_iterator i2 = superclasses.begin(); i2 != superclasses.end(); ++i2) {
        c = *i2;

        if(p_kernel->is_dangling(c) || c->p_to_be_deleted) continue;

        c->create_sub_classes();

        if(OksClass::change_notify_fn)
          (*OksClass::change_notify_fn)(c, ChangeSubClassesList, 0);
      }
      superclasses.clear();
    }

    if(OksClass::change_notify_fn) {
      if(p_all_sub_classes) i = p_all_sub_classes->begin();
      c = this;
			
      do (*OksClass::change_notify_fn)(c, changeType, parameter);
      while(p_all_sub_classes && i != p_all_sub_classes->end() && (c = *(i++)));
    }
  }
  catch(oks::exception& ex) {
    throw oks::CannotRegisterClass(*this, "change ", ex);
  }
}

bool
OksClass::check_relationships(std::ostringstream & out, bool print_file_name) const noexcept
{
  bool found_problems = false;

  if(const std::list<OksRelationship *> * rels = direct_relationships())
    {
      for(auto & x : *rels)
        {
          if(x->get_class_type() == nullptr)
            {
              if(found_problems == false)
                {
                  found_problems = true;
                  out << " * there are problems with class \"" << get_name() << '\"';
                  if(print_file_name)
                    {
                      out << " from file \"" << get_file()->get_full_file_name() << '\"';
                    }
                  out << ":\n";
                }

              out << "   - class type \"" << x->get_type() << "\" of relationship \"" << x->get_name() << "\" is not loaded\n";
            }
        }
    }

  return found_problems;
}
