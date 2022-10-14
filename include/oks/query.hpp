/**	
 *	\file oks/query.h
 *	
 *	This file is part of the OKS package.
 *      Author: Igor SOLOVIEV "https://phonebook.cern.ch/phonebook/#personDetails/?id=432778"
 *	
 *	This file contains the declarations for the OKS query.
 */

#ifndef __OKS_QUERY
#define __OKS_QUERY

#include "oks/defs.hpp"
#include "oks/object.hpp"

#include <list>
#include <exception>

#include <boost/regex.hpp>


class OksQueryExpression;


  ///	OKS query class.
  /**
   *  	The class implements OKS query.
   *  	A query can be executed over some class (and optionally subclasses)
   *	with given query expression. A query expression can be constructed
   *	dynamically for given query expression or can be read from string.
   */

class OksQuery
{
  public:

    OksQuery(bool b, OksQueryExpression *q = 0) : p_sub_classes (b), p_expression (q), p_status (0) {};
    OksQuery(const OksClass *, const std::string &);
    virtual ~OksQuery();

    friend std::ostream& operator<<(std::ostream&, const OksQuery&);

    bool search_in_subclasses() const {return p_sub_classes;}
    void search_in_subclasses(bool b) {p_sub_classes = b;}

    OksQueryExpression * get() const {return p_expression;}
    void set(OksQueryExpression* q) {p_expression = q;}
    
    bool good() const {return (p_status == 0);}

    enum QueryType {
      unknown_type,
      comparator_type,
      relationship_type,
      not_type,
      and_type,
      or_type
    };

    static const char *	 OR;
    static const char *	 AND;
    static const char *	 NOT;
    static const char *	 SOME;
    static const char *	 THIS_CLASS;
    static const char *	 ALL_SUBCLASSES;
    static const char *	 OID;
    static const char *	 EQ;
    static const char *	 NE;
    static const char *	 RE;
    static const char *	 LE;
    static const char *	 GE;
    static const char *	 LS;
    static const char *	 GT;
    static const char *	 PATH_TO;
    static const char *	 DIRECT;
    static const char *	 NESTED;

    static bool equal_cmp(const OksData *, const OksData *);
    static bool not_equal_cmp(const OksData *, const OksData *);
    static bool less_or_equal_cmp(const OksData *, const OksData *);
    static bool greater_or_equal_cmp(const OksData *, const OksData *);
    static bool less_cmp(const OksData *, const OksData *);
    static bool greater_cmp(const OksData *, const OksData *);
    static bool reg_exp_cmp(const OksData *, const OksData * regexp);

    typedef bool (*Comparator)(const OksData *, const OksData *);


  private:  

    bool p_sub_classes;
    OksQueryExpression * p_expression;
    int p_status;
    
    static OksQueryExpression *	create_expression(const OksClass *, const std::string &);
};


  ///	OKS query expression class.
  /**
   *  	The abstract class provides interface to OKS query expression.
   *	A query expression has type and method to check its correctness.
   */

class OksQueryExpression {
  friend std::ostream& operator<<(std::ostream&, const OksQueryExpression&);

  public:

    virtual ~OksQueryExpression() {;}

    OksQuery::QueryType type() const {return p_type;}
    bool CheckSyntax() const;
    bool operator==(const class OksQueryExpression& e) const {return (this == &e);}


  protected:

    OksQueryExpression(OksQuery::QueryType qet = OksQuery::unknown_type) : p_type (qet) {};


  private:

    const OksQuery::QueryType p_type;
};


  ///	OKS query expression comparator class.
  /**
   *  	The query comparator class is a basis of any query.
   *	It returns result of logical comparison between OKS value
   *	(defined by the OksData) and values of tested objects
   *	attributes (e.g. found all objects with attr-x >= 128)
   */

class OksComparator : public OksQueryExpression
{
  friend class OksObject;
  friend class OksQueryExpression;

  public:

    OksComparator(const OksAttribute *a, OksData *v, OksQuery::Comparator f) :
	OksQueryExpression	(OksQuery::comparator_type),
  	attribute		(a),
  	value			(v),
  	m_comp_f		(f),
	m_reg_exp               (0)
  	{};

    virtual ~OksComparator() {
      delete value;
      if(m_reg_exp) delete m_reg_exp;
    }

    const OksAttribute * GetAttribute() const {return attribute;}
    void SetAttribute(const OksAttribute* a) {attribute = a;}

    OksData * GetValue() {return value;}
    void SetValue(OksData *v);

    void clean_reg_exp();

    OksQuery::Comparator GetFunction() const {return m_comp_f;}
    void SetFunction(OksQuery::Comparator f) {m_comp_f = f;}

  private:

    const OksAttribute *  attribute;
    OksData *             value;
    OksQuery::Comparator  m_comp_f;
    boost::regex *        m_reg_exp;

};


  ///	OKS query relationship expression class.
  /**
   *  	The query relationship expression class is used to define
   *	queries via values of attributes for referenced objects, (e.g.
   *	find all persons living in a house with number 13.
   */

class OksRelationshipExpression : public OksQueryExpression
{
  friend class			OksObject;
  friend class			OksQueryExpression;

  public:

    OksRelationshipExpression(const OksRelationship *r, OksQueryExpression *q, bool b = false) :
	OksQueryExpression	(OksQuery::relationship_type),
  	relationship		(r),
  	checkAllObjects		(b),
  	p_expression		(q)
  	{};

    virtual ~OksRelationshipExpression() {delete p_expression;}
  
    const OksRelationship * GetRelationship() const {return relationship;}
    void SetRelationship(const OksRelationship* r) {relationship = r;}
  
    OksQueryExpression * get() const {return p_expression;}
    void set(OksQueryExpression* q) {p_expression = q;}

    bool IsCheckAllObjects() const {return checkAllObjects;}
    void SetIsCheckAllObjects(const bool b) {checkAllObjects = b;}


  private:

    const OksRelationship*	relationship;
    bool			checkAllObjects;
    OksQueryExpression*		p_expression;

};


  ///	OKS query logical NOT expression class.
  /**
   *  	The query not expression is used to change result of expression
   *	to opposite.
   */

class OksNotExpression : public OksQueryExpression
{

  friend class OksObject;
  friend class OksQueryExpression;

  public:

    OksNotExpression(OksQueryExpression *q = 0) : OksQueryExpression(OksQuery::not_type), p_expression (q) {};

    virtual ~OksNotExpression() {delete p_expression;}

    OksQueryExpression * get() const {return p_expression;}
    void set(OksQueryExpression* q) {p_expression = q;}


  private:

    OksQueryExpression		*p_expression;

};


  ///	Abstract class describing list of OKS query expressions.

class OksListBaseQueryExpression
{

  friend class OksObject;
  friend class OksClass;
  friend class OksQueryExpression;

  public:

    virtual ~OksListBaseQueryExpression() {while(!p_expressions.empty()) {OksQueryExpression * qe = p_expressions.front(); p_expressions.pop_front(); delete qe;}}

    const std::list<OksQueryExpression *> & expressions() const {return p_expressions;}
    void add(OksQueryExpression *q) {p_expressions.push_back(q);}


  protected:

    OksListBaseQueryExpression () {};


  private:

    std::list<OksQueryExpression *> p_expressions;

};


  ///	OKS query logical AND expression class.

class OksAndExpression : public OksQueryExpression, public OksListBaseQueryExpression
{
  public:

    OksAndExpression() : OksQueryExpression(OksQuery::and_type) {};

    virtual ~OksAndExpression() {;}
};


  ///	OKS query logical OR expression class.

class OksOrExpression : public OksQueryExpression, public OksListBaseQueryExpression
{
  public:

    OksOrExpression() : OksQueryExpression(OksQuery::or_type) {};

    virtual ~OksOrExpression() {;}

};

inline OksQuery::~OksQuery() {delete p_expression;}


namespace oks {

    /**
     *  The exception is thrown when parsing of query object
     *  (QueryPath object) from string is failed.
     */

  class bad_query_syntax : public std::exception
  {

    std::string p_what;

  public:

    bad_query_syntax(const std::string& what_arg) noexcept : p_what (what_arg)
      {}
    virtual ~bad_query_syntax() noexcept
      {}

    virtual const char * what () const noexcept
      { return p_what.c_str ();}
  };


    /**
     *  The class is used by the QueryPath class to describe relationships
     *  used for path search for given class.
     */

  class QueryPathExpression
  {

    friend class QueryPath;
    friend class OksObject;

    public:

      bool get_use_nested_lookup() const { return p_use_nested_lookup; }
      const std::list<std::string>& get_rel_names() const { return p_rel_names; }
      const QueryPathExpression * get_next() const { return p_next; }

    protected:

      QueryPathExpression(bool v) : p_use_nested_lookup(v) { }
      QueryPathExpression(const std::string& expression);

      ~QueryPathExpression() {delete p_next;}

      bool p_use_nested_lookup;
      std::list<std::string> p_rel_names;
      QueryPathExpression * p_next;

  };



    /**
     *  Class QueryPath describes special type of query to calculate path (i.e list of objects)
     *  between two given objects. The use case is to get a path in several trees using the same
     *  leave objects. Note for composite objects and exclusive relationships, the usage of 
     *  reverse composite relationships is more effective. In such case there is the only tree
     *  built on top of given leaves.
     *
     *  \par Example
     *
     *  The example of query is shown below:
     *    "(path-to "my-id@my-class" (direct "A" "B" (nested "N" (direct "X" "Y" "Z"))))"
     *
     *  The destination object is "my-id@my-class". The search can be started from any object of any class.
     *  In our example the start object has to have two relationships named "A" and "B".
     *  An object referenced via "A" and "B" should have relationship "N". In our example
     *  it is possible to lookup for path via nested objects linked via relationship "N".
     *  Finally all objects referenced via "N" should have relationships "X", "Y" and "Z".
     *  If the destination object is referenced by them, the path is found. The result of path
     *  query execution is list of objects between the start and the destination object.
     */


  class QueryPath
  {

    public:

      QueryPath(const OksObject * o, QueryPathExpression * qpe) : p_goal(o), p_start(qpe) { }
      QueryPath(const std::string& query, const OksKernel&);
      ~QueryPath() {delete p_start;}

      const QueryPathExpression * get_start_expression() const { return p_start; }
      const OksObject * get_goal_object() const { return p_goal; }

    private:

      const OksObject * p_goal;
      QueryPathExpression * p_start;

  };


}

std::ostream& operator<<(std::ostream&, const oks::QueryPathExpression&);
std::ostream& operator<<(std::ostream&, const oks::QueryPath&);

#endif
