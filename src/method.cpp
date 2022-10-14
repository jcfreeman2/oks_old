#define _OksBuildDll_

#include "oks/method.hpp"
#include "oks/xml.hpp"
#include "oks/class.hpp"
#include "oks/cstring.hpp"

#include <stdexcept>
#include <sstream>


const char OksMethodImplementation::method_impl_xml_tag[] = "method-implementation";
const char OksMethodImplementation::language_xml_attr[]   = "language";
const char OksMethodImplementation::prototype_xml_attr[]  = "prototype";
const char OksMethodImplementation::body_xml_attr[]       = "body";

const char OksMethod::method_xml_tag[]                    = "method";
const char OksMethod::name_xml_attr[]                     = "name";
const char OksMethod::description_xml_attr[]              = "description";


OksMethodImplementation::OksMethodImplementation(const std::string& language, const std::string& prototype, const std::string& body, OksMethod * p) :
 p_language  (language),
 p_prototype (prototype),
 p_body      (body),
 p_method    (p)
{
  oks::validate_not_empty(p_language, "method implementation language");
  oks::validate_not_empty(p_prototype, "method implementation prototype");
}

bool
OksMethodImplementation::operator==(const class OksMethodImplementation &i) const
{
  return (
    ( this == &i ) ||
    ( p_language == i.p_language && p_prototype == i.p_prototype && p_body == i.p_body )
  );
}

std::ostream&
operator<<(std::ostream& s, const OksMethodImplementation& i)
{
  s << "  Method implementation:\n"
       "    language:  \"" << i.get_language()  << "\"\n"
       "    prototype: \"" << i.get_prototype() << "\"\n"
       "    body:      \"" << i.get_body()      << "\"\n";

  return s;
}


void
OksMethodImplementation::save(OksXmlOutputStream& s) const
{
  s.put("   ");

  s.put_start_tag(method_impl_xml_tag, sizeof(method_impl_xml_tag)-1);

  s.put_attribute( language_xml_attr,  sizeof(language_xml_attr)-1,  get_language().c_str()  );
  s.put_attribute( prototype_xml_attr, sizeof(prototype_xml_attr)-1, get_prototype().c_str() );
  s.put_attribute( body_xml_attr,      sizeof(body_xml_attr)-1,      get_body().c_str()      );

  s.put_end_tag();
}


OksMethodImplementation::OksMethodImplementation(OksXmlInputStream& s, OksMethod * parent) :
 p_method (parent)
{
  try {
    while(true) {
      OksXmlAttribute attr(s);

        // check for close of tag

      if(oks::cmp_str1(attr.name(), "/")) { break; }

        // check for known oks-method-implementation' attributes

      else if( oks::cmp_str8(attr.name(), language_xml_attr)  ) p_language.assign(attr.value(), attr.value_len());
      else if( oks::cmp_str9(attr.name(), prototype_xml_attr) ) p_prototype.assign(attr.value(), attr.value_len());
      else if( oks::cmp_str4(attr.name(), body_xml_attr)      ) p_body.assign(attr.value(), attr.value_len());
      else {
        s.throw_unexpected_attribute(attr.name());
      }
    }
  }
  catch(oks::exception & e) {
    throw oks::FailedRead("xml method implementation", e);
  }
  catch (std::exception & e) {
    throw oks::FailedRead("xml method implementation", e.what());
  }

  try {
    oks::validate_not_empty(p_language, "method implementation language");
    oks::validate_not_empty(p_prototype, "method implementation prototype");
  }
  catch(std::exception& ex) {
    throw oks::FailedRead("oks method implementation", oks::BadFileData(ex.what(), s.get_line_no(), s.get_line_pos()));
  }
}


OksMethod::OksMethod(const std::string& nm, OksClass * p) :
  p_class		(p),
  p_name		(nm),
  p_implementations	(nullptr)
{
  try {
    oks::validate_not_empty(p_name, "method name");
  }
  catch(std::exception& ex) {
    Oks::error_msg("OksMethod::OksMethod()") << ex.what() << std::endl;
  }
}

OksMethod::OksMethod(const std::string& nm, const std::string& desc, OksClass * p) :
  p_class		(p),
  p_name		(nm),
  p_description		(desc),
  p_implementations	(nullptr)
{
  oks::validate_not_empty(p_name, "method name");
}

OksMethod::~OksMethod()
{
  if(p_implementations) {
    while(!p_implementations->empty()) {
      OksMethodImplementation * i = p_implementations->front();
      p_implementations->pop_front();
      delete i;
    }

    delete p_implementations;
  }
}


bool
OksMethod::operator==(const class OksMethod &m) const
{
    // check if compare with self

  if(this == &m) return true;


    // check if attributes are different

  if(p_name != m.p_name || p_description != m.p_description) return false;


    // check if methods have no implementations

  if(p_implementations == 0 && m.p_implementations == 0) return true;


    // check if only one method has implementation

  if(p_implementations == 0 || m.p_implementations == 0) return false;


    // check if numbers of implementations are different

  if(p_implementations->size() != m.p_implementations->size()) return false;


    // check implementations

  std::list<OksMethodImplementation *>::const_iterator i1 = p_implementations->begin();
  std::list<OksMethodImplementation *>::const_iterator i2 = m.p_implementations->begin();
  
  for(;i1 != p_implementations->end(); ++i1, ++i2) {
    if( !(*(*i1) == *(*i2)) ) return false;
  }
  
  return true;
}

std::ostream&
operator<<(std::ostream& s, const OksMethod& m)
{
  s << "Method name: \"" << m.p_name << "\"\n"
       " description: \"" << m.p_description << "\"\n";
	
  if(m.p_implementations) {
    s << " implementations:\n";

    for(std::list<OksMethodImplementation *>::const_iterator i = m.p_implementations->begin(); i != m.p_implementations->end(); ++i)
      s << *(*i);
  }
  else
    s << " there are no implementation(s)\n";

  return s;
}


void
OksMethod::save(OksXmlOutputStream& s) const
{
  s.put_raw(' ');
  s.put_raw(' ');

  s.put_start_tag(method_xml_tag, sizeof(method_xml_tag)-1);
  
  s.put_attribute(name_xml_attr, sizeof(name_xml_attr)-1, p_name.c_str());
  s.put_attribute(description_xml_attr, sizeof(description_xml_attr)-1, p_description.c_str());

  s.put_raw('>');
  s.put_raw('\n');

  if(p_implementations) {
    for(std::list<OksMethodImplementation *>::iterator i = p_implementations->begin(); i != p_implementations->end(); ++i)
      (*i)->save(s);
  }

  s.put_raw(' ');
  s.put_raw(' ');

  s.put_last_tag(method_xml_tag, sizeof(method_xml_tag)-1);
}


OksMethod::OksMethod(OksXmlInputStream& s, OksClass * parent) :
 p_class           (parent),
 p_implementations (0)
{
  try {
    while(true) {
      OksXmlAttribute attr(s);

        // check for close of tag

      if(oks::cmp_str1(attr.name(), ">")) { break; }

        // check for known oks-relationship' attributes

      else if(oks::cmp_str4(attr.name(), name_xml_attr)) p_name.assign(attr.value(), attr.value_len());
      else if(oks::cmp_str11(attr.name(), description_xml_attr)) p_description.assign(attr.value(), attr.value_len());
      else {
        s.throw_unexpected_attribute(attr.name());
      }
    }
  }
  catch(oks::exception & e) {
    throw oks::FailedRead("xml method", e);
  }
  catch (std::exception & e) {
    throw oks::FailedRead("xml method", e.what());
  }

    // check validity of read values

  try {
    oks::validate_not_empty(p_name, "method name");
  }
  catch(std::exception& ex) {
    throw oks::FailedRead("oks method", oks::BadFileData(ex.what(), s.get_line_no(), s.get_line_pos()));
  }


    // read 'body' and 'method-actions'

  {
    while(true) try {
      const char * tag_start = s.get_tag_start();

      if(oks::cmp_str7(tag_start, "/method")) { break; }

      else if(!strcmp(tag_start, "method-implementation")) {
	if(!p_implementations) p_implementations = new std::list<OksMethodImplementation *>();
	p_implementations->push_back(new OksMethodImplementation(s, this));
      }

      else {
        std::ostringstream text;
	text << "Unexpected tag \'" << tag_start << "\' inside method \'" << p_name << "\'\n";
        throw std::runtime_error( text.str().c_str() );
      }

    }
    catch (oks::exception & e) {
      throw oks::FailedRead("method tag", e);
    }
    catch (std::exception & e) {
      throw oks::FailedRead("method tag", e.what());
    }
  }
}


void
OksMethod::set_name(const std::string& new_name)
{
    // ignore when name is the same

  if(p_name == new_name) return;


    // additional checks are required,
    // if the method already belongs to some class

  if(p_class) {

      // check allowed length for attribute name

    try {
      oks::validate_not_empty(new_name, "name");
    }
    catch(std::exception& ex) {
      throw oks::SetOperationFailed("OksMethod::set_name", ex.what());
    }


      // having a direct method with the same name is an error

    if(p_class->find_direct_method(new_name) != 0) {
      std::ostringstream text;
      text << "Class \"" << p_class->get_name() << "\" already has direct method \"" << new_name << '\"';
      throw oks::SetOperationFailed("OksMethod::set_name", text.str());
    }


      // check that it is possible to lock the file

    p_class->lock_file("OksMethod::set_name");


      // probably a non-direct method already exists

    OksMethod * m = p_class->find_method(new_name);


      // change the name

    p_name = new_name;


      // registrate the change

    p_class->registrate_class_change(OksClass::ChangeMethodsList, (const void *)m);
  }
  else {
    p_name = new_name;
  }
}

OksMethodImplementation *
OksMethod::find_implementation(const std::string& language) const
{
  if(p_implementations) {
    for(std::list<OksMethodImplementation *>::iterator i = p_implementations->begin(); i != p_implementations->end(); ++i)
      if(language == (*i)->get_language()) return *i;
  }

  return 0;
}

void
OksMethod::add_implementation(const std::string& language, const std::string& prototype, const std::string& body)
{
  if(find_implementation(language)) {
    std::ostringstream text;
    text << "Cannot add implementation on language \"" << language << "\" since it already exists.";
    throw oks::SetOperationFailed("OksMethod::add_implementation", text.str());
  }

  if(p_class) p_class->lock_file("OksMethod::add_implementation");

  if(!p_implementations) {
    p_implementations = new std::list<OksMethodImplementation *>();
  }

  OksMethodImplementation * i = new OksMethodImplementation(language, prototype, body);

  p_implementations->push_back(i);
  i->p_method = this;

  if(p_class) p_class->registrate_class_change(OksClass::ChangeMethodImplementation, (const void *)this);
}

void
OksMethod::remove_implementation(const std::string& language)
{
  OksMethodImplementation * i = find_implementation(language);

  if(i == 0) {
    std::ostringstream text;
    text << "Cannot remove implementation on language \"" << language << "\" since it does not exist.";
    throw oks::SetOperationFailed("OksMethod::remove_implementation", text.str());
  }

  if(p_class) p_class->lock_file("OksMethod::remove_implementation");

  p_implementations->remove(i);

  if(p_implementations->empty()) {
    delete p_implementations;
    p_implementations = 0;
  }

  if(p_class) p_class->registrate_class_change(OksClass::ChangeMethodImplementation, (const void *)this);
}


void
OksMethod::set_description(const std::string& desc)
{
  if(p_description != desc) {
    if(p_class) p_class->lock_file("OksMethod::set_description");

    p_description = desc;

    if(p_class) p_class->registrate_class_change(OksClass::ChangeMethodDescription, (const void *)this);
  }
}

void
OksMethodImplementation::set_language(const std::string& s)
{
  if(p_language != s) {
    if(p_method) {
      if(p_method->find_implementation(s) != 0) {
        std::ostringstream text;
        text << "cannot rename \"" << p_method->p_name << "\" method implementation to language \"" << s << "\" "
             "since the method already has implementation with such language.";
        throw oks::SetOperationFailed("OksMethodImplementation::set_language", text.str());
      }

      try {
        oks::validate_not_empty(s, "language");
      }
      catch(std::exception& ex) {
        throw oks::SetOperationFailed("OksMethodImplementation::set_language", ex.what());
      }

      if(p_method->p_class) p_method->p_class->lock_file("OksMethodImplementation::set_language");
    }

    p_language = s;

    if(p_method && p_method->p_class) {
      p_method->p_class->registrate_class_change(OksClass::ChangeMethodImplementation, (const void *)this);
    }
  }
}

void
OksMethodImplementation::set_prototype(const std::string& s)
{
  if(p_prototype != s) {
    try {
      oks::validate_not_empty(s, "prototype");
    }
    catch(std::exception& ex) {
      throw oks::SetOperationFailed("OksMethodImplementation::set_prototype", ex.what());
    }

    if(p_method && p_method->p_class) p_method->p_class->lock_file("OksMethodImplementation::set_prototype");

    p_prototype = s;

    if(p_method && p_method->p_class) {
      p_method->p_class->registrate_class_change(OksClass::ChangeMethodImplementation, (const void *)this);
    }
  }
}

void
OksMethodImplementation::set_body(const std::string& s)
{
  if(p_body != s) {
    if(p_method && p_method->p_class) p_method->p_class->lock_file("OksMethodImplementation::set_body");

    p_body = s;

    if(p_method && p_method->p_class) {
      p_method->p_class->registrate_class_change(OksClass::ChangeMethodImplementation, (const void *)this);
    }
  }
}
