#ifndef __OKS_INDEX
#define __OKS_INDEX

#include "oks/query.hpp"
#include "oks/attribute.hpp"
#include "oks/object.hpp"

#include <set>

class OksObjectSortBy {

  public:

    OksObjectSortBy	(size_t i = 0) : offset(i) {;}

    bool operator()	(const OksObject * o1, const OksObject * o2) const {
      return o1->data[offset] < o2->data[offset];
    }


  private:

    size_t offset;

};


 class OksIndex : public std::multiset<OksObject *, OksObjectSortBy> {
  friend class OksClass;
  friend class OksObject;


  public:

      // Declare map of poiters to OksIndex sorted by attribute name

    struct SortByName {
      bool operator()	(const OksAttribute * a1, const OksAttribute * a2) const {
        return a1->get_name() < a2->get_name();
      }
    };

    typedef std::map<const OksAttribute *, OksIndex *, SortByName> Map;

    typedef std::multiset<OksObject *, OksObjectSortBy>::iterator Position;
    typedef std::multiset<OksObject *, OksObjectSortBy>::const_iterator ConstPosition;


    OksIndex		(OksClass *, OksAttribute *);
    ~OksIndex		();
    
    OksObject *		FindFirst(OksData *d) const;

    OksObject::List *	FindEqual(OksData *d) const {return find_all(d, OksQuery::equal_cmp);}
    OksObject::List *	FindLessEqual(OksData *d) const {return find_all(d, OksQuery::less_or_equal_cmp);}
    OksObject::List *	FindGreatEqual(OksData *d) const {return find_all(d, OksQuery::greater_or_equal_cmp);}
    OksObject::List *	FindLess(OksData *d) const {return find_all(d, OksQuery::less_cmp);}
    OksObject::List *	FindGreat(OksData *d) const {return find_all(d, OksQuery::greater_cmp);}

    OksObject::List *	FindLessAndGreat(OksData *d1, OksData *d2) const {return find_all(true, d1, OksQuery::less_cmp, d2, OksQuery::greater_cmp);}
    OksObject::List *	FindLessAndGreatEqual(OksData *d1, OksData *d2) const {return find_all(true, d1, OksQuery::less_cmp, d2, OksQuery::greater_or_equal_cmp);}
    OksObject::List *	FindLessEqualAndGreat(OksData *d1, OksData *d2) const {return find_all(true, d1, OksQuery::less_or_equal_cmp, d2, OksQuery::greater_cmp);}
    OksObject::List *	FindLessEqualAndGreatEqual(OksData *d1, OksData *d2) const {return find_all(true, d1, OksQuery::less_or_equal_cmp, d2, OksQuery::greater_or_equal_cmp);}
    OksObject::List *	FindEqualAndEqual(OksData *d1, OksData *d2) const {return find_all(true, d1, OksQuery::equal_cmp, d2, OksQuery::equal_cmp);}
    OksObject::List *	FindEqualAndLess(OksData *d1, OksData *d2) const {return find_all(true, d1, OksQuery::equal_cmp, d2, OksQuery::less_cmp);}
    OksObject::List *	FindEqualAndLessEqual(OksData *d1, OksData *d2) const {return find_all(true, d1, OksQuery::equal_cmp, d2, OksQuery::less_or_equal_cmp);}
    OksObject::List *	FindEqualAndGreat(OksData *d1, OksData *d2) const {return find_all(true, d1, OksQuery::equal_cmp, d2, OksQuery::greater_cmp);}
    OksObject::List *	FindEqualAndGreatEqual(OksData *d1, OksData *d2) const {return find_all(true, d1, OksQuery::equal_cmp, d2, OksQuery::greater_or_equal_cmp);}
    OksObject::List *	FindLessOrGreat(OksData *d1, OksData *d2) const {return find_all(false, d1, OksQuery::less_cmp, d2, OksQuery::greater_cmp);}
    OksObject::List *	FindLessOrGreatEqual(OksData *d1, OksData *d2) const {return find_all(false, d1, OksQuery::less_cmp, d2, OksQuery::greater_or_equal_cmp);}
    OksObject::List *	FindLessEqualOrGreat(OksData *d1, OksData *d2) const {return find_all(false, d1, OksQuery::less_or_equal_cmp, d2, OksQuery::greater_cmp);}
    OksObject::List *	FindLessEqualOrGreatEqual(OksData *d1, OksData *d2) const {return find_all(false, d1, OksQuery::less_or_equal_cmp, d2, OksQuery::greater_or_equal_cmp);}
    OksObject::List *	FindEqualOrEqual(OksData *d1, OksData *d2) const {return find_all(false, d1, OksQuery::equal_cmp, d2, OksQuery::equal_cmp);}
    OksObject::List *	FindEqualOrLess(OksData *d1, OksData *d2) const {return find_all(false, d1, OksQuery::equal_cmp, d2, OksQuery::less_cmp);}
    OksObject::List *	FindEqualOrLessEqual(OksData *d1, OksData *d2) const {return find_all(false, d1, OksQuery::equal_cmp, d2, OksQuery::less_or_equal_cmp);}
    OksObject::List *	FindEqualOrGreat(OksData *d1, OksData *d2) const {return find_all(false, d1, OksQuery::equal_cmp, d2, OksQuery::greater_cmp);}
    OksObject::List *	FindEqualOrGreatEqual(OksData *d1, OksData *d2) const {return find_all(false, d1, OksQuery::equal_cmp, d2, OksQuery::greater_or_equal_cmp);}

  private:

    OksClass *		c;
    OksAttribute *	a;
    size_t		offset;

    OksObject *		remove_obj(OksObject *);

    OksObject::List *	find_all(OksData *, OksQuery::Comparator) const;
    OksObject::List *	find_all(bool, OksData *, OksQuery::Comparator, OksData *, OksQuery::Comparator) const;

    void		find_interval(OksData *, OksQuery::Comparator, ConstPosition&, ConstPosition&) const;

    static size_t       get_offset(OksClass *, OksAttribute *);

};

#endif
