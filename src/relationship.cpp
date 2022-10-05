#define _OksBuildDll_

#include <oks/relationship.h>
#include <oks/xml.h>
#include <oks/class.h>
#include <oks/kernel.h>
#include <oks/cstring.h>

#include <sstream>


const char OksRelationship::relationship_xml_tag[]  = "relationship";
const char OksRelationship::name_xml_attr[]         = "name";
const char OksRelationship::description_xml_attr[]  = "description";
const char OksRelationship::class_type_xml_attr[]   = "class-type";
const char OksRelationship::low_cc_xml_attr[]       = "low-cc";
const char OksRelationship::high_cc_xml_attr[]      = "high-cc";
const char OksRelationship::is_composite_xml_attr[] = "is-composite";
const char OksRelationship::is_exclusive_xml_attr[] = "is-exclusive";
const char OksRelationship::is_dependent_xml_attr[] = "is-dependent";
const char OksRelationship::ordered_xml_attr[]      = "ordered";


OksRelationship::CardinalityConstraint
OksRelationship::str2card(const char * s) noexcept
{
  return (
    oks::cmp_str3(s, "one")  ? One :
    oks::cmp_str4(s, "zero") ? Zero :
    Many
  );
}

const char *
OksRelationship::card2str(CardinalityConstraint cc) noexcept
{
  return (
    cc == Zero ? "zero" :
    cc == One  ? "one" :
    "many"
  );
}

OksRelationship::OksRelationship(const std::string& nm, OksClass * p) :
  p_name	   (nm),
  p_low_cc	   (Zero),
  p_high_cc 	   (Many),
  p_composite	   (false),
  p_exclusive	   (false),
  p_dependent	   (false),
  p_class	   (p),
  p_class_type	   (nullptr),
  p_ordered        (false)
{
  oks::validate_not_empty(p_name, "relationship name");
}

OksRelationship::OksRelationship(const std::string& nm, const std::string& rc,
                                 CardinalityConstraint l_cc, CardinalityConstraint h_cc,
                                 bool cm, bool excl, bool dp, const std::string& desc, OksClass * p) :
  p_name           (nm),
  p_rclass         (rc),
  p_low_cc         (l_cc),
  p_high_cc        (h_cc),
  p_composite      (cm),
  p_exclusive      (excl),
  p_dependent      (dp),
  p_description	   (desc),
  p_class          (p),
  p_class_type     (nullptr),
  p_ordered        (false)
{
  oks::validate_not_empty(p_name, "relationship name");
  oks::validate_not_empty(p_rclass, "relationship class");
}

bool
OksRelationship::operator==(const class OksRelationship &r) const
{
  return (
    ( this == &r ) ||
    (
      ( p_name == r.p_name ) &&
      ( p_rclass == r.p_rclass ) &&
      ( p_low_cc == r.p_low_cc ) &&
      ( p_high_cc == r.p_high_cc ) &&
      ( p_composite == r.p_composite ) &&
      ( p_exclusive == r.p_exclusive ) &&
      ( p_dependent == r.p_dependent ) &&
      ( p_description == r.p_description ) &&
      ( p_ordered == r.p_ordered )
   )
  );
}

std::ostream&
operator<<(std::ostream& s, const OksRelationship& r)
{
  s << "Relationship name: \"" << r.p_name << "\"\n"
       " class type: \"" << r.p_rclass << "\"\n"
       " low cardinality constraint is " << OksRelationship::card2str(r.p_low_cc) << "\n"
       " high cardinality constraint is " << OksRelationship::card2str(r.p_high_cc) << "\n"
       " is" << (r.p_composite == true ? "" : " not") << " composite reference\n"
       " is" << (r.p_exclusive == true ? "" : " not") << " exclusive reference\n"
       " is " << (r.p_dependent == true ? "dependent" : "shared") << " reference\n"
       " has description: \"" << r.p_description << "\"\n"
       " is " << (r.p_ordered == true ? "ordered" : "unordered") << std::endl;

  return s;
}


void
OksRelationship::save(OksXmlOutputStream& s) const
{
  s.put("  ");

  s.put_start_tag(relationship_xml_tag, sizeof(relationship_xml_tag) - 1);

  s.put_attribute(name_xml_attr, sizeof(name_xml_attr) - 1, p_name.c_str());

  if (!p_description.empty())
    s.put_attribute(description_xml_attr, sizeof(description_xml_attr) - 1, p_description.c_str());

  s.put_attribute(class_type_xml_attr, sizeof(class_type_xml_attr) - 1, p_rclass.c_str());
  s.put_attribute(low_cc_xml_attr, sizeof(low_cc_xml_attr) - 1, card2str(p_low_cc));
  s.put_attribute(high_cc_xml_attr, sizeof(high_cc_xml_attr) - 1, card2str(p_high_cc));
  s.put_attribute(is_composite_xml_attr, sizeof(is_composite_xml_attr) - 1, oks::xml::bool2str(p_composite));
  s.put_attribute(is_exclusive_xml_attr, sizeof(is_exclusive_xml_attr) - 1, oks::xml::bool2str(p_exclusive));
  s.put_attribute(is_dependent_xml_attr, sizeof(is_dependent_xml_attr) - 1, oks::xml::bool2str(p_dependent));

  if (p_ordered)
    s.put_attribute(ordered_xml_attr, sizeof(ordered_xml_attr) - 1, oks::xml::bool2str(p_ordered));

  s.put_end_tag();
}


OksRelationship::OksRelationship(OksXmlInputStream& s, OksClass *parent) :
  p_low_cc	   (Zero),
  p_high_cc 	   (Many),
  p_composite	   (false),
  p_exclusive	   (false),
  p_dependent	   (false),
  p_class          (parent),
  p_class_type     (nullptr),
  p_ordered        (false)
{
  try {
    while(true) {
      OksXmlAttribute attr(s);

        // check for close of tag
      
      if(oks::cmp_str1(attr.name(), "/")) { break; }


        // check for known oks-relationship' attributes

      else if(oks::cmp_str4(attr.name(), name_xml_attr)) p_name.assign(attr.value(), attr.value_len());
      else if(oks::cmp_str11(attr.name(), description_xml_attr)) p_description.assign(attr.value(), attr.value_len());
      else if(oks::cmp_str10(attr.name(), class_type_xml_attr)) p_rclass.assign(attr.value(), attr.value_len());
      else if(oks::cmp_str6(attr.name(), low_cc_xml_attr)) p_low_cc = str2card(attr.value());
      else if(oks::cmp_str7(attr.name(), high_cc_xml_attr)) p_high_cc = str2card(attr.value());
      else if(oks::cmp_str12(attr.name(), is_composite_xml_attr)) p_composite = oks::xml::str2bool(attr.value());
      else if(oks::cmp_str12(attr.name(), is_exclusive_xml_attr)) p_exclusive = oks::xml::str2bool(attr.value());
      else if(oks::cmp_str12(attr.name(), is_dependent_xml_attr)) p_dependent = oks::xml::str2bool(attr.value());
      else if(oks::cmp_str7(attr.name(), ordered_xml_attr)) p_ordered = oks::xml::str2bool(attr.value());
      else if(!strcmp(attr.name(), "multi-value-implementation")) {
        s.error_msg("OksRelationship::OksRelationship(OksXmlInputStream&)")
             << "Obsolete oks-relationship\'s attribute \'" << attr.name() << "\'\n";
      }
      else {
        s.throw_unexpected_attribute(attr.name());
      }
    }
  }
  catch(oks::exception & e) {
    throw oks::FailedRead("xml attribute", e);
  }
  catch (std::exception & e) {
    throw oks::FailedRead("xml attribute", e.what());
  }


    // check validity of read values

  try {
    oks::validate_not_empty(p_name, "relationship name");
    oks::validate_not_empty(p_rclass, "relationship class");
  }
  catch(std::exception& ex) {
    throw oks::FailedRead("oks relationship", oks::BadFileData(ex.what(), s.get_line_no(), s.get_line_pos()));
  }
}


void
OksRelationship::set_name(const std::string& new_name)
{
    // ignore when name is the same

  if(p_name == new_name) return;


    // additional checks are required,
    // if the relationship already belongs to some class

  if(p_class) {

      // check maximum allowed length for relationship name

    try {
      oks::validate_not_empty(new_name, "name");
    }
    catch(std::exception& ex) {
      throw oks::SetOperationFailed("OksRelationship::set_name", ex.what());
    }


      // having a direct relationship with the same name is an error

    if(p_class->find_direct_relationship(new_name) != 0) {
      std::ostringstream text;
      text << "Class \"" << p_class->get_name() << "\" already has direct relationship \"" << new_name << '\"';
      throw oks::SetOperationFailed("OksRelationship::set_name", text.str());
    }


      // check possibility to lock the file

    p_class->lock_file("OksRelationship::set_name");


      // probably a non-direct relationship already exists

    OksRelationship * r = p_class->find_relationship(new_name);


      // change the name

    p_name = new_name;


      // registrate the change

    p_class->registrate_class_change(OksClass::ChangeRelationshipsList, (const void *)r);
  }
  else {
    p_name = new_name;
  }
}

void
OksRelationship::set_type(const std::string& cn)
{
  if(p_rclass == cn) return;

  if(!p_class || !p_class->p_kernel || (p_class_type = p_class->p_kernel->find_class(cn)) != 0) {
    if(p_class) p_class->lock_file("OksRelationship::set_type");

    p_rclass = cn;

    if(p_class) {
      p_class->registrate_class_change(OksClass::ChangeRelationshipClassType, (const void *)this);
      p_class->registrate_relationship_change(this);
    }
  }
  else {
    std::ostringstream text;
    text << "cannot find class \"" << cn << '\"';
    throw oks::SetOperationFailed("OksRelationship::set_type", text.str());
  }
}
	
void
OksRelationship::set_description(const std::string& desc)
{
  if(p_description != desc) {
    if(p_class) p_class->lock_file("OksRelationship::set_description");

    p_description = desc;

    if(p_class) p_class->registrate_class_change(OksClass::ChangeRelationshipDescription, (const void *)this);
  }
}
	
void
OksRelationship::set_low_cardinality_constraint(CardinalityConstraint l_cc)
{
  if(p_low_cc != l_cc) {
    if(p_class) p_class->lock_file("OksRelationship::set_low_cardinality_constraint");

    CardinalityConstraint old_cc = p_low_cc;

    p_low_cc = l_cc;

    if(p_class) {
      p_class->registrate_class_change(OksClass::ChangeRelationshipLowCC, (const void *)this);
		
      if(p_low_cc == Many || old_cc == Many) p_class->registrate_relationship_change(this);
    }
  }
}
	
void
OksRelationship::set_high_cardinality_constraint(CardinalityConstraint h_cc)
{
  if(p_high_cc != h_cc) {
    if(p_class) p_class->lock_file("OksRelationship::set_high_cardinality_constraint");

    CardinalityConstraint old_cc = p_high_cc;

    p_high_cc = h_cc;
	
    if(p_class) {
      p_class->registrate_class_change(OksClass::ChangeRelationshipHighCC, (const void *)this);

      if(p_high_cc == Many || old_cc == Many) p_class->registrate_relationship_change(this);
    }
  }
}
	
void
OksRelationship::set_is_composite(bool cm)
{
  if(p_composite != cm) {
    if(p_class) p_class->lock_file("OksRelationship::set_is_composite");

    p_composite = cm;

    if(p_class) p_class->registrate_class_change(OksClass::ChangeRelationshipComposite, (const void *)this);
  }
}
	
void
OksRelationship::set_is_exclusive(bool ex)
{
  if(p_exclusive != ex) {
    if(p_class) p_class->lock_file("OksRelationship::set_is_exclusive");

    p_exclusive = ex;

    if(p_class) p_class->registrate_class_change(OksClass::ChangeRelationshipExclusive, (const void *)this);
  }
}
	
void
OksRelationship::set_is_dependent(bool dp)
{
  if(p_dependent != dp) {
    if(p_class) p_class->lock_file("OksRelationship::set_is_dependent");

    p_dependent = dp;

    if(p_class) p_class->registrate_class_change(OksClass::ChangeRelationshipDependent, (const void *)this);
  }
}
