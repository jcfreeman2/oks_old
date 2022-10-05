#define _OksBuildDll_

#include <oks/query.h>
#include <oks/attribute.h>
#include <oks/relationship.h>
#include <oks/class.h>
#include <oks/object.h>
#include <oks/kernel.h>
#include <oks/index.h>
#include <oks/profiler.h>

#include <stdexcept>
#include <sstream>

const char * OksQuery::OR = "or";
const char * OksQuery::AND = "and";
const char * OksQuery::NOT = "not";
const char * OksQuery::SOME = "some";
const char * OksQuery::THIS_CLASS = "this";
const char * OksQuery::ALL_SUBCLASSES = "all";
const char * OksQuery::OID = "object-id";
const char * OksQuery::EQ = "=";
const char * OksQuery::NE = "!=";
const char * OksQuery::RE = "~=";
const char * OksQuery::LE = "<=";
const char * OksQuery::GE = ">=";
const char * OksQuery::LS = "<";
const char * OksQuery::GT = ">";
const char * OksQuery::PATH_TO = "path-to";
const char * OksQuery::DIRECT = "direct";
const char * OksQuery::NESTED = "nested";

namespace oks {

  std::string
  QueryFailed::fill(const OksQueryExpression& query, const OksClass& c, const std::string& reason) noexcept
  {
    std::ostringstream text;
    text << "query \"" << query << "\" in class \"" << c.get_name() << "\" failed:\n" << reason;
    return text.str();
  }

  std::string
  BadReqExp::fill(const std::string& what, const std::string& reason) noexcept
  {
    return std::string("failed to create reqular expression \"") + what + "\": " + reason;
  }

}

bool OksQuery::equal_cmp(const OksData *d1, const OksData *d2) {return (*d1 == *d2);}
bool OksQuery::not_equal_cmp(const OksData *d1, const OksData *d2) {return (*d1 != *d2);}
bool OksQuery::less_or_equal_cmp(const OksData *d1, const OksData *d2) {return (*d1 <= *d2);}
bool OksQuery::greater_or_equal_cmp(const OksData *d1, const OksData *d2) {return (*d1 >= *d2);}
bool OksQuery::less_cmp(const OksData *d1, const OksData *d2) {return (*d1 < *d2);}
bool OksQuery::greater_cmp(const OksData *d1, const OksData *d2) {return (*d1 > *d2);}
bool OksQuery::reg_exp_cmp(const OksData *d, const OksData * re) {
  return boost::regex_match(d->str(), *reinterpret_cast<const boost::regex *>(re));
}


void OksComparator::SetValue(OksData *v)
{
  delete value;
  value = v;
  
  clean_reg_exp();
}

void OksComparator::clean_reg_exp()
{
  if(m_reg_exp) {
    delete m_reg_exp;
    m_reg_exp = 0;
  }
}


inline void erase_empty_chars(std::string& s)
{
  while(s[0] == ' ' || s[0] == '\n' || s[0] == '\t') s.erase(0, 1);
}


OksQuery::OksQuery(const OksClass *c, const std::string & str) : p_expression (0), p_status (1)
{
  OSK_PROFILING(OksProfiler::fStringToQuery, c->get_kernel())

  const char * fname = "OksQuery::OksQuery(OksClass *, const char *)";
  const char * error_str = "Can't create query ";

  char delimiter = '\0';

  if(!c) {
    Oks::error_msg(fname) << error_str << "without specified class\n";
    return;
  }

  if(str.empty()) {
    Oks::error_msg(fname) << error_str << "from empty string\n";
    return;
  }

  std::string s(str);
  
  erase_empty_chars(s);

  if(s.empty()) {
    Oks::error_msg(fname) << error_str << "from string which consists of space symbols\n";
    return;
  }

  if(s[0] == '(') {
    s.erase(0, 1);
    delimiter = ')';
  }
  		
  std::string::size_type p = s.find(' ');
	
  if(p == std::string::npos) {
    Oks::error_msg(fname)
      << "Can't parse query expression \"" << str << "\"\n"
         "it must consists of as minimum two tokens separated by space\n";
    return;
  }
  
  if(s.substr(0, p) == OksQuery::ALL_SUBCLASSES)
    p_sub_classes = true;
  else if(s.substr(0, p) == OksQuery::THIS_CLASS)
    p_sub_classes = false;
  else {
    Oks::error_msg(fname)
      << "Can't parse query expression \"" << str << "\"\n"
         "the first token must be \'"<< OksQuery::ALL_SUBCLASSES
      << "\' or \'"<< OksQuery::THIS_CLASS << "\'\n";
    return;
  }

  s.erase(0, p + 1);

  if(delimiter == ')') {
    p = s.rfind(delimiter);
    if(p == std::string::npos) {
      Oks::error_msg(fname)
        << "Can't parse query expression \"" << str << "\"\n"
           "it must contain closing bracket \')\' if it has opening bracket \'(\'\n";
      return;
    }

    s.erase(p);
  }

  
  erase_empty_chars(s);


  if(s[0] == '(') {
    p = s.rfind(')');

    if(p == std::string::npos) {
      Oks::error_msg(fname)
        << "Can't parse query expression \"" << s << "\"\n"
           "it must contain closing bracket \')\' if it has opening bracket \'(\'\n";
      return;
    }

    s.erase(p);
    s.erase(0, 1);

    p_expression = create_expression(c, s);
 	
    if(p_expression) p_status = 0;
  }
  else
    Oks::error_msg(fname)
      << "Can't parse subquery expression \"" << s << "\"\n"
         "it must be enclosed by brackets\n";
}


OksQueryExpression *
OksQuery::create_expression(const OksClass *c, const std::string & str)
{
  const char * fname = "OksQuery::create_expression()";

  OSK_PROFILING(OksProfiler::fStringToQueryExpression, c->get_kernel())

  OksQueryExpression *qe = 0;

  if(!c) {
    Oks::error_msg(fname) << "Can't create query without specified class\n";
    return qe;
  }

  if(str.empty()) {
    Oks::error_msg(fname) << "Can't create query from empty string\n";
    return qe;
  }

  std::string s(str);

  std::list<std::string> slist;

  while(s.length()) {
    erase_empty_chars(s);

    if(!s.length()) break;	

    if(
     s[0] == '\"' ||
     s[0] == '\'' ||
     s[0] == '`'
    ) {
      char delimiter = s[0];
      s.erase(0, 1);

      std::string::size_type p = s.find(delimiter);

      if(p == std::string::npos) {
        Oks::error_msg(fname)
          << "Can't parse query expression \"" << str << "\"\n"
             "the delimiter is \' "<< delimiter << " \'\n"
             "the rest of the expression is \"" << s << "\"\n";
        return qe;
      }

      s.erase(p, 1);
      slist.push_back(std::string(s, 0, p));

      s.erase(0, p + 1);
    }
    else if(s[0] == '(') {
      std::string::size_type p = 1;
      size_t strLength = s.length();
      size_t r = 1;
		
      while(p < strLength) {
        if(s[p] == '(') r++;
        if(s[p] == ')') {
          r--;
          if(!r) break;
        }
        p++;
      }

      if(r) {
        Oks::error_msg(fname)
          << "Can't parse query expression \"" << str << "\"\n"
          << "There is no closing \')\' for " << '\"' << s << "\"\n";
        return qe;
      }

      s.erase(p, 1);
      s.erase(0, 1);

      slist.push_back(std::string(s, 0, p - 1));

      s.erase(0, p - 1);
    }
    else {
      std::string::size_type p = 0;
      size_t strLength = s.length();
		
      while(p < strLength && s[p] != ' ') p++;
		
      slist.push_back(std::string(s, 0, p));
		
      s.erase(0, p);
    }
  }
  
  if(slist.empty()) {
    Oks::error_msg(fname)
      << "Can't create query from empty string \"" << str << "\"\n";
    return qe;
  }

  const std::string first = slist.front();
  slist.pop_front();

  if(
   first == OksQuery::AND ||
   first == OksQuery::OR
  ) {
    if(slist.size() < 2) {
      Oks::error_msg(fname) << "\'" << first << "\' must have two or more arguments: (" << str << ")'\n";
      return qe;
    }

    qe = (
      (first == OksQuery::AND)
        ? (OksQueryExpression *)new OksAndExpression()
        : (OksQueryExpression *)new OksOrExpression()
    );

    while(!slist.empty()) {
      const std::string item2 = slist.front();
      slist.pop_front();

      OksQueryExpression *qe2 = create_expression(c, item2);

      if(qe2) {
        if(first == OksQuery::AND) 
          ((OksAndExpression *)qe)->add(qe2);
        else
          ((OksOrExpression *)qe)->add(qe2);
      }
    }

    return qe;	/* SUCCESS */
  }
  else if(first == OksQuery::NOT) {
    if(slist.size() != 1) {
      Oks::error_msg(fname) << "\'" << first << "\' must have exactly one argument: (" << str << ")\n";
      return qe;
    }

    qe = (OksQueryExpression *)new OksNotExpression();

    const std::string item2 = slist.front();
    slist.pop_front();

    OksQueryExpression *qe2 = create_expression(c, item2);

    if(qe2) ((OksNotExpression *)qe)->set(qe2);

    return qe;	/* SUCCESS */
  }
  else if(slist.size() != 2) {
    Oks::error_msg(fname) << "Can't parse query expression \"" << str << "\"\n";
    return qe;
  }
  else {
    const std::string second = slist.front();
    slist.pop_front();

    const std::string third = slist.front();
    slist.pop_front();
	
    if(second == OksQuery::SOME || second == OksQuery::ALL_SUBCLASSES) {
      OksRelationship *r = c->find_relationship(first);
		
      if(!r) {
        Oks::error_msg(fname)
          << "For expression \"" << str << "\"\n"
             "can't find relationship \"" << first << "\" in class \"" << c->get_name() << "\"\n";

        return qe;
      }

      bool b;

      if(second == OksQuery::SOME) b = false;
      else if(second == OksQuery::ALL_SUBCLASSES) b = true;
      else {
        Oks::error_msg(fname)
          << "For relationship expression \"" << str << "\"\n"
              "second parameter \'" << second << "\' must be \'" << *OksQuery::SOME
	  << "\' or \'" << *OksQuery::ALL_SUBCLASSES << "\'\n";
        return qe;
      }

      OksClass *relc = c->get_kernel()->find_class(r->get_type());

      if(!relc) {
        Oks::error_msg(fname)
          << "For expression \"" << str << "\"\n"
          << "can't find class \"" << r->get_type() << "\"\n";
        return qe;
      }

      OksQueryExpression *qe2 = create_expression(relc, third);

      if(qe2) qe = (OksQueryExpression *)new OksRelationshipExpression(r, qe2, b);

      return qe;	/* SUCCESS */
    }
    else {
      OksAttribute *a = ((first != OksQuery::OID) ? c->find_attribute(first) : 0);
		
      if(first != OksQuery::OID && !a) {
        Oks::error_msg(fname)
          << "For expression \"" << str << "\"\n"
          << "can't find attribute \"" << first << "\" in class \""
	  << c->get_name() << "\"\n";
        return qe;
      }

      OksData * d = new OksData();

      OksQuery::Comparator f = (
        (third == OksQuery::EQ) ? OksQuery::equal_cmp :
        (third == OksQuery::NE) ? OksQuery::not_equal_cmp :
        (third == OksQuery::RE) ? OksQuery::reg_exp_cmp :
        (third == OksQuery::LE) ? OksQuery::less_or_equal_cmp :
        (third == OksQuery::GE) ? OksQuery::greater_or_equal_cmp :
        (third == OksQuery::LS) ? OksQuery::less_cmp :
        (third == OksQuery::GT) ? OksQuery::greater_cmp :
        0
      );

      if(a) {
        if(f == OksQuery::reg_exp_cmp) {
	  d->type = OksData::string_type;
	  d->data.STRING = new OksString(second);
        }
        else {
          d->type = OksData::unknown_type;
          d->SetValues(second.c_str(), a);
        }
      }
      else {
        d->Set(second);
      }

      if(!f)
        Oks::error_msg(fname)
          << "For expression \"" << str << "\"\n"
          << "can't find comparator function \"" << third << "\"\n";
      else
        qe = (OksQueryExpression *)new OksComparator(a, d, f);

      return qe;	/* (UN)SUCCESS */
    }
  }
}



OksObject::List *
OksClass::execute_query(OksQuery *qe) const
{
  const char * fname = "OksClass::execute_query()";

  OSK_PROFILING(OksProfiler::Classexecute_query, p_kernel)

  
  OksObject::List * olist = 0;
  OksQueryExpression *sqe = qe->get();
  
  if(sqe->CheckSyntax() == false) {
    Oks::error_msg(fname) << "Can't execute query \"" << *sqe << "\"\n";
    return 0;
  }

  if(p_objects && !p_objects->empty()) {
    bool indexedSearch = false;
  	
    if(p_indices) {
      if(sqe->type() == OksQuery::comparator_type) {
        OksComparator *cq = (OksComparator *)sqe;
	OksIndex::Map::iterator j = p_indices->find(cq->GetAttribute());
	
        if(j != p_indices->end()) {
          indexedSearch = true;
          olist = (*j).second->find_all(cq->GetValue(), cq->GetFunction());
        }
      }
      else if(
       (sqe->type() == OksQuery::and_type) ||
       (sqe->type() == OksQuery::or_type)
      ) {
        std::list<OksQueryExpression *> * qlist = &((OksListBaseQueryExpression *)sqe)->p_expressions;
        OksQueryExpression *q1, *q2;
        OksComparator *cq1 = 0, *cq2 = 0;
			
        if(
         (qlist->size() == 2) &&
         ((q1 = qlist->front())->type() == OksQuery::comparator_type) &&
         ((q2 = qlist->back())->type() == OksQuery::comparator_type) &&
         ((cq1 = (OksComparator *)q1) != 0) &&
         ((cq2 = (OksComparator *)q2) != 0) &&
         (cq1->GetAttribute() == cq2->GetAttribute())
        ) {
	  OksIndex::Map::iterator j = p_indices->find(cq1->GetAttribute());
				
          if(j != p_indices->end()) {
            indexedSearch = true;

            olist = (*j).second->find_all(
              ((sqe->type() == OksQuery::and_type) ? true : false),
              cq1->GetValue(),
              cq1->GetFunction(),
              cq2->GetValue(),
              cq2->GetFunction()
            );
          }
        }
      }
    }
	
    if(indexedSearch == false) {
      for(OksObject::Map::iterator i = p_objects->begin(); i != p_objects->end(); ++i) {
        OksObject *o = (*i).second;

        try {
          if(o->SatisfiesQueryExpression(sqe) == true) {
            if(!olist) olist = new OksObject::List();
            olist->push_back(o);
          }
        }
        catch(oks::exception& ex) {
          throw oks::QueryFailed(*sqe, *this, ex);
        }
        catch(std::exception& ex) {
          throw oks::QueryFailed(*sqe, *this, ex.what());
        }
      }
    }
  }


  if(qe->search_in_subclasses() == true && p_all_sub_classes && !p_all_sub_classes->empty()) {
    for(OksClass::FList::iterator i = p_all_sub_classes->begin(); i != p_all_sub_classes->end(); ++i) {
      OksClass *c = *i;

      if(c->p_objects && !c->p_objects->empty()) {
        for(OksObject::Map::iterator i2 = c->p_objects->begin(); i2 != c->p_objects->end(); ++i2) {
          OksObject *o = (*i2).second;

          try {
            if(o->SatisfiesQueryExpression(sqe) == true) {
              if(!olist) olist = new OksObject::List();
              olist->push_back(o);
            }
          }
          catch(oks::exception& ex) {
            throw oks::QueryFailed(*sqe, *this, ex);
          }
          catch(std::exception& ex) {
            throw oks::QueryFailed(*sqe, *this, ex.what());
          }
        }
      }
    }
  }

  return olist;
}


bool
OksQueryExpression::CheckSyntax() const
{
  const char * fname = "OksQueryExpression::CheckSyntax()";

  switch(p_type) {
    case OksQuery::comparator_type:
      if(!((OksComparator *)this)->attribute && !((OksComparator *)this)->value) {
      	Oks::error_msg(fname)
          << "OksComparator: Can't execute query for nil attribute or nil object-id\n";
      	return false;
      }
      else if(!((OksComparator *)this)->m_comp_f) {
      	Oks::error_msg(fname)
          << "OksComparator: Can't execute query for nil compare function\n";
      	return false;
      }

      return true;

    case OksQuery::relationship_type:
      if(!((OksRelationshipExpression *)this)->relationship) {
      	Oks::error_msg(fname)
          << "OksRelationshipExpression: Can't execute query for nil relationship\n";
      	return false;
      }
      else if(!((OksRelationshipExpression *)this)->p_expression) {
      	Oks::error_msg(fname)
          << "OksRelationshipExpression: Can't execute query for nil query expression\n";
      	return false;
      }
      else
      	return (((OksRelationshipExpression *)this)->p_expression)->CheckSyntax();

    case OksQuery::not_type:
      if(!((OksNotExpression *)this)->p_expression) {
      	Oks::error_msg(fname)
          << "OksNotExpression: Can't execute \'not\' for nil query expression\n";
      	return false;
      }
      
      return (((OksNotExpression *)this)->p_expression)->CheckSyntax();

    case OksQuery::and_type:
      if(((OksAndExpression *)this)->p_expressions.size() < 2) {
      	Oks::error_msg(fname)
          << "OksAndExpression: Can't execute \'and\' for "
          << ((OksAndExpression *)this)->p_expressions.size() << " argument\n"
             "Two or more arguments are required\n";
      	return false;
      }
      else {
        std::list<OksQueryExpression *> & elist = ((OksAndExpression *)this)->p_expressions;
      	
      	for(std::list<OksQueryExpression *>::iterator i = elist.begin(); i != elist.end(); ++i)
          if((*i)->CheckSyntax() == false) return false;
      	
      	return true;
      }

    case OksQuery::or_type:
      if(((OksOrExpression *)this)->p_expressions.size() < 2) {
      	Oks::error_msg(fname)
          << "OksOrExpression: Can't execute \'or\' for "
          << ((OksOrExpression *)this)->p_expressions.size() << " argument\n"
             "Two or more arguments are required\n";
	
      	return false;
      }
      else {
        std::list<OksQueryExpression *> & elist = ((OksOrExpression *)this)->p_expressions;

      	for(std::list<OksQueryExpression *>::iterator i = elist.begin(); i != elist.end(); ++i)
          if((*i)->CheckSyntax() == false) return false;
      	
      	return true;
      }
	
    default:
      Oks::error_msg(fname)
        << "Unexpected query type " << (int)p_type << std::endl;

      return false;
  }
}


bool
OksObject::SatisfiesQueryExpression(OksQueryExpression *qe) const
{
  OSK_PROFILING(OksProfiler::ObjectSatisfiesQueryExpression, uid.class_id->p_kernel)

  if(!qe) {
    throw std::runtime_error("cannot execute nil query");
  }

  switch(qe->type()) {
    case OksQuery::comparator_type: {
      OksComparator *cmp = (OksComparator *)qe;
      const OksAttribute *a = cmp->attribute;
      OksQuery::Comparator f = cmp->m_comp_f;

      if(!a && !cmp->value) {
        throw std::runtime_error("cannot execute query for nil attribute");
      }
      else if(!f) {
        throw std::runtime_error("cannot execute query for nil compare function");
      }

      const OksData * cmp_value(cmp->value);

      if(f == OksQuery::reg_exp_cmp) { 
        if(!cmp->m_reg_exp) {
          try {
            std::string s(cmp->value->str());
            cmp->m_reg_exp = new boost::regex(s.c_str());
          }
          catch(std::exception& ex) {
            throw oks::BadReqExp(cmp->value->str(), ex.what());
          }
	}
	cmp_value = reinterpret_cast<const OksData *>(cmp->m_reg_exp);
      }

      if(!a) {
        OksData d(GetId());
	return (*f)(&d, cmp_value);
      }

      return (*f)(
        &(data[(*(uid.class_id->p_data_info->find(a->get_name()))).second->offset]),
        cmp_value
      );
    }

    case OksQuery::relationship_type:
      if(!((OksRelationshipExpression *)qe)->relationship) {
        throw std::runtime_error("cannot execute query for nil relationship");
      }
      else {
        OksData *d = &data[((*(uid.class_id->p_data_info->find(((OksRelationshipExpression *)qe)->relationship->get_name()))).second)->offset];

        if(((OksRelationshipExpression *)qe)->relationship->get_high_cardinality_constraint() == OksRelationship::Many) {
          if(!d->data.LIST || d->data.LIST->empty()) return false;

          for(OksData::List::iterator i = d->data.LIST->begin(); i != d->data.LIST->end(); ++i) {
            OksData *d2 = (*i);

            if(d2->type == OksData::uid2_type) {
              std::ostringstream text;
              text << "cannot process relationship expression: object \"" << *d2->data.UID2.object_id << '@' << *d2->data.UID2.class_id
                   << "\" referenced through multi values relationship \"" << ((OksRelationshipExpression *)qe)->relationship->get_name()
                   << "\" is not loaded in memory";
              throw std::runtime_error(text.str().c_str());
            }

            if(((OksRelationshipExpression *)qe)->checkAllObjects == true) {
              if(
               !d2->data.OBJECT ||
               d2->data.OBJECT->SatisfiesQueryExpression(((OksRelationshipExpression *)qe)->p_expression) == false
              ) return false;
            }
            else {
              if(
               d2->data.OBJECT &&
               d2->data.OBJECT->SatisfiesQueryExpression(((OksRelationshipExpression *)qe)->p_expression) == true
              ) return true;
            }
          }
			
          return (((OksRelationshipExpression *)qe)->checkAllObjects == true) ? true : false;
        }
        else {
          if(d->type != OksData::object_type) {
            std::ostringstream text;
            text << "cannot process relationship expression: object \"" << *d << "\" referenced through single value relationship \""
                 << ((OksRelationshipExpression *)qe)->relationship->get_name() << "\" is not loaded in memory";
            throw std::runtime_error(text.str().c_str());
          }

          return (
            d->data.OBJECT
	      ? d->data.OBJECT->SatisfiesQueryExpression(((OksRelationshipExpression *)qe)->p_expression)
	      : false
          );
        }
      }

    case OksQuery::not_type:
      if(!((OksNotExpression *)qe)->p_expression) {
        throw std::runtime_error("cannot process \'not\' expression: referenced query expression is nil");
      }

      return (SatisfiesQueryExpression(((OksNotExpression *)qe)->p_expression) ? false : true);

    case OksQuery::and_type:
      if(((OksAndExpression *)qe)->p_expressions.size() < 2) {
        std::ostringstream text;
        text << "cannot process \'and\' expression for " << ((OksAndExpression *)qe)->p_expressions.size()
             << " argument (two or more arguments are required)";
        throw std::runtime_error(text.str().c_str());
      }
      else {
        std::list<OksQueryExpression *> & elist = ((OksAndExpression *)qe)->p_expressions;

        for(std::list<OksQueryExpression *>::iterator i = elist.begin(); i != elist.end();++i)
          if(SatisfiesQueryExpression(*i) == false) return false;

        return true;
      }

    case OksQuery::or_type:
      if(((OksOrExpression *)qe)->p_expressions.size() < 2) {
        std::ostringstream text;
        text << "cannot process \'or\' expression for " << ((OksAndExpression *)qe)->p_expressions.size()
             << " argument (two or more arguments are required)";
        throw std::runtime_error(text.str().c_str());
      }
      else {
        std::list<OksQueryExpression *> & elist = ((OksOrExpression *)qe)->p_expressions;

        for(std::list<OksQueryExpression *>::iterator i = elist.begin(); i != elist.end();++i)
          if(SatisfiesQueryExpression(*i) == true) return true;

        return false;
      }

    default: {
      std::ostringstream text;
      text << "unexpected query type " << (int)(qe->type());
      throw std::runtime_error(text.str().c_str());
    }
  }
}


std::ostream&
operator<<(std::ostream& s, const OksQueryExpression& qe)
{
  s << '(';
  
  switch(qe.type()) {
    case OksQuery::comparator_type: {
      OksComparator *cmpr = (OksComparator *)&qe;
      const OksAttribute *a = cmpr->GetAttribute();
      OksData *v = cmpr->GetValue();
      OksQuery::Comparator f = cmpr->GetFunction();

      if(a) {
        s << '\"' << a->get_name() << "\" ";
      }
      else if(v) {
        s << OksQuery::OID << ' ';
      }
      else {
        s << "(null) ";
      }

      if(v) {
        s << *v << ' ';
      }
      else {
        s << "(null) ";
      }

      if(f) {
        if(f == OksQuery::equal_cmp) s << OksQuery::EQ;
        else if(f == OksQuery::not_equal_cmp) s << OksQuery::NE;
        else if(f == OksQuery::reg_exp_cmp) s << OksQuery::RE;
        else if(f == OksQuery::less_or_equal_cmp) s << OksQuery::LE;
        else if(f == OksQuery::greater_or_equal_cmp) s << OksQuery::GE;
        else if(f == OksQuery::less_cmp) s << OksQuery::LS;
        else if(f == OksQuery::greater_cmp) s << OksQuery::GT;
      }
      else
        s << "(null)";

      break; }

    case OksQuery::relationship_type: {
      OksRelationshipExpression *re = (OksRelationshipExpression *)&qe;
      const OksRelationship *r = re->GetRelationship();
      bool b = re->IsCheckAllObjects();
      OksQueryExpression *rqe = re->get();

      if(r)
        s << '\"' << r->get_name() << "\" ";
      else
        s << "(null) ";

      s << (b == true ? OksQuery::ALL_SUBCLASSES : OksQuery::SOME) << ' ';

      if(rqe) s << *rqe;
      else s << "(null)";

      break; }

    case OksQuery::not_type:
      s << OksQuery::NOT << ' ' << *(((OksNotExpression *)&qe)->get());

      break;

    case OksQuery::and_type: {
      s << OksQuery::AND << ' ';

      const std::list<OksQueryExpression *> & elist = ((OksAndExpression *)&qe)->expressions();

      if(!elist.empty()) {
        const OksQueryExpression * last = elist.back();

        for(std::list<OksQueryExpression *>::const_iterator i = elist.begin(); i != elist.end(); ++i) {
          s << *(*i);
          if(*i != last) s << ' ';
        }
      }

      break;
    }

    case OksQuery::or_type: {
      s << OksQuery::OR << ' ';

      const std::list<OksQueryExpression *> & elist = ((OksOrExpression *)&qe)->expressions();

      if(!elist.empty()) {
        const OksQueryExpression * last = elist.back();

        for(std::list<OksQueryExpression *>::const_iterator i = elist.begin(); i != elist.end(); ++i) {
          s << *(*i);
          if(*i != last) s << ' ';
        }
      }

      break;
    }

    case OksQuery::unknown_type: {
      s << "(unknown)";

      break;
    }
  }

  s << ')';

  return s;
}


std::ostream&
operator<<(std::ostream& s, const OksQuery& gqe)
{
  s << '('
    << (gqe.p_sub_classes ? OksQuery::ALL_SUBCLASSES : OksQuery::THIS_CLASS)
    << ' ';

  if(gqe.p_expression)
    s << *gqe.p_expression;
  else
    s << "(null)";

  s << ')';

  return s;
}


std::ostream&
operator<<(std::ostream& s, const oks::QueryPath& query)
{
  s << '(' << OksQuery::PATH_TO << ' ' << query.get_goal_object() << ' ' << *query.get_start_expression() << ')';
  return s;
}


std::ostream&
operator<<(std::ostream& s, const oks::QueryPathExpression& e)
{
  s << '(' << (e.get_use_nested_lookup() ? OksQuery::NESTED : OksQuery::DIRECT) << ' ';

  for(std::list<std::string>::const_iterator i = e.get_rel_names().begin(); i != e.get_rel_names().end(); ++i) {
    if(i != e.get_rel_names().begin()) s << ' ';
    s << '\"' << *i << '\"';
  }

  if(e.get_next()) s << ' ' << *(e.get_next());

  s << ')';

  return s;
}


OksObject::List *
OksObject::find_path(const oks::QueryPath& query) const
{
  OksObject::List * path = new OksObject::List();
  
  if(satisfies(query.get_goal_object(), *query.get_start_expression(), *path) == false) {
    delete path;
    path = 0;
  }

  return path;
}


bool
OksObject::satisfies(const OksObject * goal, const oks::QueryPathExpression& expression, OksObject::List& path) const
{
    // check the object is not in the path

  {
    for(std::list<OksObject *>::const_iterator i = path.begin(); i != path.end(); ++i) {
      if(*i == this) return false;
    }
  }

  path.push_back(const_cast<OksObject *>(this));


  for(std::list<std::string>::const_iterator i = expression.get_rel_names().begin(); i != expression.get_rel_names().end(); ++i) {
    OksData * d = 0;

    if(!(*i).empty() && (*i)[0] == '?') {
      std::string nm = (*i).substr(1);
      OksDataInfo::Map::iterator i = uid.class_id->p_data_info->find(nm);

      if(i != uid.class_id->p_data_info->end()) {
        d = GetRelationshipValue((*i).second);
      }
      else {
        continue;
      }
    }
    else {
      try {
        d = GetRelationshipValue(*i);
      }
      catch(oks::exception& ex) {
        Oks::error_msg("OksObject::satisfies") << ex.what() << std::endl;
        continue;
      }
    }

      // check if given relationship points to destination object

    if(d->type == OksData::object_type && d->data.OBJECT == goal) return true;
    else if(d->type == OksData::list_type && d->data.LIST) {
      for(OksData::List::iterator i2 = d->data.LIST->begin(); i2 != d->data.LIST->end(); ++i2) {
        OksData * d2 = (*i2);
        if(d2->type == OksData::object_type && d2->data.OBJECT == goal) return true;
      }
    }


      // go to next path, if there are no more expressions

    if(!expression.get_next()) {
      continue;
    }


      // check, if there is need for nested path lookup

    else if(expression.get_use_nested_lookup()) {

        // go directly

      path.pop_back();
      if(satisfies(goal, *expression.get_next(), path) == true) return true;
      path.push_back(const_cast<OksObject *>(this));

        // go nested

      if(d->type == OksData::object_type && d->data.OBJECT) {
        if(d->data.OBJECT->satisfies(goal, expression, path) == true) return true;
      }
      else if(d->type == OksData::list_type && d->data.LIST) {
        for(OksData::List::iterator i2 = d->data.LIST->begin(); i2 != d->data.LIST->end(); ++i2) {
          OksData * d2 = (*i2);
          if(d2->type == OksData::object_type && d2->data.OBJECT) {
            if(d2->data.OBJECT->satisfies(goal, expression, path) == true) return true;
	  }
        }
      }
    }

    else {
      if(d->type == OksData::object_type && d->data.OBJECT) {
        if(d->data.OBJECT->satisfies(goal, *expression.get_next(), path) == true) return true;
      }
      else if(d->type == OksData::list_type && d->data.LIST) {
        for(OksData::List::iterator i2 = d->data.LIST->begin(); i2 != d->data.LIST->end(); ++i2) {
          OksData * d2 = (*i2);
          if(d2->type == OksData::object_type && d2->data.OBJECT) {
            if(d2->data.OBJECT->satisfies(goal, *expression.get_next(), path) == true) return true;
	  }
        }
      }
    }
  }

  path.pop_back();
  return false;
}

oks::QueryPath::QueryPath(const std::string& str, const OksKernel& kernel) : p_start(0)
{
  std::string s(str);
  erase_empty_chars(s);

  if(s.empty()) {
    throw oks::bad_query_syntax( "Empty query" );
  }

  if(s[0] == '(') {
    std::string::size_type p = s.rfind(')');

    if(p == std::string::npos) {
      throw oks::bad_query_syntax(std::string("Query expression \'") + str + "\' must contain closing bracket");
    }

    s.erase(p);
    s.erase(0, 1);
  }
  else {
    throw oks::bad_query_syntax(std::string("Query expression \'") + str + "\' must be enclosed by brackets");
  }

  erase_empty_chars(s);

  Oks::Tokenizer t(s, " \t\n");
  std::string token;
  t.next(token);

  if(token != OksQuery::PATH_TO) {
    throw oks::bad_query_syntax(std::string("Expression \'") + s + "\' must start from " + OksQuery::DIRECT + " or " + OksQuery::NESTED + " keyword");
  }

  s.erase(0, token.size());
  erase_empty_chars(s);
  
  if( s[0] == '\"' ) {
    std::string::size_type p = s.find('\"', 1);

    if(p == std::string::npos) {
      throw oks::bad_query_syntax(std::string("No trailing delimiter of object name in query \'") + str + "\'");
    }

    std::string::size_type p2 = s.find('@');

    if(p2 == std::string::npos || p2 > p) {
      throw oks::bad_query_syntax(std::string("Bad format of object name ") + s.substr(0, p+1) + " in query \'" + str + "\'");
    }

    std::string object_id = std::string(s, 1, p2 - 1);
    std::string class_name = std::string(s, p2 + 1, p - p2 - 1);

    if(OksClass * c = kernel.find_class(class_name)) {
      if((p_goal = c->get_object(object_id)) == 0) {
        throw oks::bad_query_syntax(std::string("Cannot find object ") + s.substr(0, p+1) + " in query \'" + str + "\': no such object");
      }
    }
    else {
      throw oks::bad_query_syntax(std::string("Cannot find object ") + s.substr(0, p+1) + " in query \'" + str + "\': no such class");
    }

    s.erase(0, p + 1);
  }
  else {
    throw oks::bad_query_syntax(std::string("No name of object in \'") + str + "\'");
  }

  try {
    p_start = new QueryPathExpression(s);
  }
  catch ( oks::bad_query_syntax& e ) {
    throw oks::bad_query_syntax(std::string("Failed to parse expression \'") + str + "\' because \'" + e.what() + "\'");
  }
}

oks::QueryPathExpression::QueryPathExpression(const std::string& str) : p_next(0)
{
  std::string s(str);
  erase_empty_chars(s);

  if(s.empty()) {
    throw oks::bad_query_syntax( "Empty expression" );
  }

  if(s[0] == '(') {
    std::string::size_type p = s.rfind(')');

    if(p == std::string::npos) {
      throw oks::bad_query_syntax(std::string("Expression \'") + str + "\' must contain closing bracket");
    }

    s.erase(p);
    s.erase(0, 1);

      // build nested expression if any

    std::string::size_type p1 = s.find('(');

    if(p1 != std::string::npos) {
      std::string::size_type p2 = s.rfind(')');

      if(p2 == std::string::npos) {
        throw oks::bad_query_syntax(std::string("Nested expression of \'") + str + "\' must contain closing bracket");
      }

      p_next = new QueryPathExpression(s.substr(p1, p2));
      
      s.erase(p1, p2);
    }

    erase_empty_chars(s);

    Oks::Tokenizer t(s, " \t\n");
    std::string token;
    t.next(token);

    if(token == OksQuery::DIRECT) {
      p_use_nested_lookup = false;
    }
    else if(token == OksQuery::NESTED) {
      p_use_nested_lookup = true;
    }
    else {
      delete p_next; p_next = 0;
      throw oks::bad_query_syntax(std::string("Expression \'") + s + "\' must start from " + OksQuery::DIRECT + " or " + OksQuery::NESTED + " keyword");
    }

    s.erase(0, token.size());

    while(s.length()) {
      erase_empty_chars(s);

      if(!s.length()) break;	

      if( s[0] == '\"' || s[0] == '\'' || s[0] == '`' ) {
        char delimiter = s[0];

        p = s.find(delimiter, 1);

        if(p == std::string::npos) {
          delete p_next; p_next = 0;
          throw oks::bad_query_syntax(std::string("No trailing delimiter of \'") + s + "\' (expression \'" + str + "\')");
        }

        p_rel_names.push_back(std::string(s, 1, p-1));

        s.erase(0, p + 1);
      }
      else {
        delete p_next; p_next = 0;
        throw oks::bad_query_syntax(std::string("Name of relationship \'") + s + "\' must start from a delimiter (expression \'" + str + "\')");
      }
    }

    if(p_rel_names.empty()) {
      delete p_next; p_next = 0;
      throw oks::bad_query_syntax(std::string("An expression of \'") + str + "\' has no relationship names defined");
    }
  }
  else {
    throw oks::bad_query_syntax(std::string("Expression \'") + str + "\' must be enclosed by brackets");
  }
}
