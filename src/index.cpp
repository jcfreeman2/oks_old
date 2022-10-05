#define _OksBuildDll_

#include <oks/index.h>
#include <oks/attribute.h>
#include <oks/class.h>


size_t
OksIndex::get_offset(OksClass *cl, OksAttribute *a)
{
  OksDataInfo::Map::const_iterator x = cl->p_data_info->find(a->p_name);
  return (x == cl->p_data_info->end()) ? 0 : x->second->offset;
}


OksIndex::OksIndex(OksClass *cl, OksAttribute *attr) :
  std::multiset<OksObject *, OksObjectSortBy> (get_offset(cl, attr)),
  c (cl),
  a (attr)
{
  const char * fname = "OksIndex::OksIndex(OksClass *, OksAttribute *";

  if(!c) {
    Oks::error_msg(fname) << "Can't build index for NIL class\n";
    return;
  }
  
  if(c->get_is_abstract()) {
    Oks::error_msg(fname)
      << "Can't build index for ABSTRACT class \"" << c->get_name() << "\"\n";
    return;
  }

  if(!a) {
    Oks::error_msg(fname) << "Can't build index for NIL attribute\n";
    return;
  }

  if(c->find_attribute(a->get_name()) == 0) {
    Oks::error_msg(fname)
      << "Can't find attribute \"" << a->p_name << "\" in class \""
      << c->get_name() << "\" to build index.\n";
    return;
  }

  if(c->p_indices && c->p_indices->find(a) != c->p_indices->end()) {
    Oks::error_msg(fname)
      << "Class \"" << c->get_name() << "\" already has index for attribute \""
      << a->p_name << "\".\n";
    return;
  }

  offset = ((*c->p_data_info)[a->p_name])->offset;

  if(!c->p_indices)
    c->p_indices = new OksIndex::Map();

  (*c->p_indices)[a] = this;

  if(c->p_objects && !c->p_objects->empty()) {
    for(OksObject::Map::iterator i = c->p_objects->begin(); i != c->p_objects->end(); ++i)
      insert((*i).second);
  }
  
  std::cout << "Build index for attribute \'" << a->p_name << "\' in class \'" << c->get_name()
  	    << "\' for " << size() << " instances\n";

#ifdef DEBUG_INDICES
  for(int j=0; j<entries(); j++) {
	OksObject *obj = at(j);

	std::cout << j << ".\tobject id \'" << obj->GetId() << "\'   "
		  << "\tvalue: \'" << obj->data[offset] << "\'\n";
  }
#endif
}

OksIndex::~OksIndex()
{
  if(c && a) {
    c->p_indices->erase(a);

    if(c->p_indices->empty()) {
      delete c->p_indices;
      c->p_indices = 0;
    }
  }
}


OksObject*
OksIndex::remove_obj(OksObject *o)
{
  std::pair<Position, Position> positions = equal_range(o);
  Position i = positions.first;
  Position i2 = positions.second;

  for(; i != i2; ++i) {
    if(o == *i) {
      erase(i);
      return o;
    }
  }

  return 0;
}


OksObject *
OksIndex::FindFirst(OksData *d) const
{
  OksObject test_o(offset, d);

  ConstPosition pos = lower_bound(&test_o);

  if(pos != end()) return *pos;

  return 0;
}


void
OksIndex::find_interval(OksData *d, OksQuery::Comparator f, ConstPosition& i1, ConstPosition& i2) const
{
  i1 = i2 = end();

  if(empty()) return;

  OksObject test_o(offset, d);

  if(f == OksQuery::equal_cmp) {
    i1 = lower_bound(&test_o);

    if(i1 != end()) {
      if((*i1)->data[offset] == *d) {
        i2 = upper_bound(&test_o);
      }
      else {
        i2 = i1;
      }
    }
  }
  else if(f == OksQuery::less_or_equal_cmp || f == OksQuery::less_cmp) {
    i1 = begin();

    if(f == OksQuery::less_cmp) {
      i2 = lower_bound(&test_o);
    }
    else {
      i2 = upper_bound(&test_o);
    }
  }
  else if(f == OksQuery::greater_or_equal_cmp || f == OksQuery::greater_cmp) {
    i2 = end();
    
    if(f == OksQuery::greater_cmp) {
      i1 = upper_bound(&test_o);
    }
    else {
      i1 = lower_bound(&test_o);
    }
  }
}


OksObject::List *
OksIndex::find_all(OksData *d, OksQuery::Comparator f) const
{
  OksObject::List * olist = 0;

  ConstPosition pos1, pos2;

  find_interval(d, f, pos1, pos2);

  if(pos1 != pos2) {
    olist = new OksObject::List();
    for(;pos1 != pos2; ++pos1) olist->push_back(*pos1);
  }

  return olist;
}


OksObject::List *
OksIndex::find_all(bool andOperation, OksData *d1, OksQuery::Comparator f1, OksData *d2, OksQuery::Comparator f2) const
{
  OksObject::List * olist = 0;
  
  ConstPosition a1, b1;
  
  find_interval(d1, f1, a1, b1);
  
  if((andOperation == true) && (a1 == b1)) return 0;
  
  ConstPosition a2, b2;

  find_interval(d2, f2, a2, b2);

  if(andOperation == true) { 
    if(a2 == b2) return 0;


      //
      // find intersection ([pos1,pos2] or NIL) of [a1,b1] and [a2,b2]
      // where begin() <= {a1,b1,a2,b2} <= end())
      //
      // (note that a1 and a2 are not equal to end() but b1 nad b2 have to be tested)
      //

    ConstPosition pos1, pos2;

    if((*a2)->data[offset] <= (*a1)->data[offset]) {
      if((b2 != end()) && ((*b2)->data[offset] < (*a1)->data[offset])) return 0;
      pos1 = a1;
    }
    else {
      if((b1 != end()) && (*a2)->data[offset] > (*b1)->data[offset]) return 0;
      pos1 = a2;
    }
    
    if(b2 == end() || b1 == end()) {
      pos2 = (b2 != end()) ? b2 : b1;
    }
    else
      pos2 = ((*b2)->data[offset] <= (*b1)->data[offset]) ? b2 : b1;

    olist = new OksObject::List();
    for(;pos1 != pos2; ++pos1) olist->push_back(*pos1);
  }
  else {
    if(a1 != b1 && a2 == b2) {
      olist = new OksObject::List();
      for(;a1 != b1; ++a1) olist->push_back(*a1);
    }
    else if(a2 != b2 && a1 == b1) {
      olist = new OksObject::List();
      for(;a2 != b2; ++a2) olist->push_back(*a2);
    }
    else if(a1 != b1 && a2 != b2) {
      olist = new OksObject::List();


        //
        // find union of [a1,b1] and [a2,b2] (i.e. [pos1,pos2] or [a1,b1],[a2,b2])
	// where begin() <= {a1,b1,a2,b2} <= end())
        //
        // (note that a1 and a2 are not equal to end() but b1 nad b2 have to be tested)
        //

      ConstPosition pos1, pos2;

      if((*a2)->data[offset] <= (*a1)->data[offset]) {
        if(b2 != end() && (*b2)->data[offset] < (*a1)->data[offset]) {
          for(;a2 != b2; ++a2) olist->push_back(*a2);
          for(;a1 != b1; ++a1) olist->push_back(*a1);

          return olist;
        }

        pos1 = a2;
      }
      else {
        if(b1 != end() && (*a2)->data[offset] > (*b1)->data[offset]) {
          for(;a1 != b1; ++a1) olist->push_back(*a1);
          for(;a2 != b2; ++a2) olist->push_back(*a2);

          return olist;
        }
		
        pos1 = a1;
      }

      if(b2 == end() || b1 == end()) {
	pos2 = (b2 == end()) ? b2 : b1;
      }
      else
        pos2 = ((*b2)->data[offset] >= (*b1)->data[offset]) ? b2 : b1;

      for(;pos1 != pos2; ++pos1) olist->push_back(*pos1);
    }
  }

  return olist;
}
