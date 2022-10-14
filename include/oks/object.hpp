/**	
 *  \file oks/object.h
 *
 *  This file is part of the OKS package.
 *  Author: Igor SOLOVIEV "https://phonebook.cern.ch/phonebook/#personDetails/?id=432778"
 *
 *  This file contains the declarations for the OKS object.
 */

#ifndef OKS_OBJECT_H
#define OKS_OBJECT_H

#include "oks/defs.hpp"
#include "oks/file.hpp"
#include "oks/exceptions.hpp"

#include <stdint.h>

#include <string>
#include <list>
#include <set>
#include <map>
#include <functional>

#include "oks/config/map.hpp"

#include <boost/date_time/gregorian/greg_date.hpp>
#include <boost/date_time/posix_time/ptime.hpp>
#include <boost/pool/pool_alloc.hpp>

#include <unordered_map>
#include <unordered_set>



  /// Forward declarations

class   OksKernel;
class   OksFile;
struct  OksData;
class	OksClass;
class	OksObject;
class	OksAttribute;
class	OksRelationship;
class	OksMethod;
class	OksQueryExpression;
struct	OksAliasTable;
class   OksXmlOutputStream;
class   OksXmlInputStream;
struct  OksXmlToken;
struct  OksXmlValue;
struct  OksXmlRelValue;

  /// @addtogroup oks

namespace oks {

  class QueryPathExpression;
  class QueryPath;


    /** Failed to create an object. **/

  class FailedCreateObject : public exception {

    public:

        /** Get reason from nested oks exception. **/
      FailedCreateObject(const OksObject * o, const exception& reason) noexcept : exception (fill(o, reason.what()), reason.level() + 1) { }

        /** Get reason from nested non-oks exception. **/
      FailedCreateObject(const OksObject * o, const std::string& reason) noexcept : exception (fill(o, reason), 0) { }

      virtual ~FailedCreateObject() noexcept { }


    private:

      static std::string fill(const OksObject * o, const std::string& reason);
  };


    /** Failed to read an object. **/

  class FailedReadObject : public exception {

    public:

        /** Get reason from nested oks exception. **/
      FailedReadObject(const OksObject * o, const std::string& what, const exception& reason) noexcept : exception (fill(o, what, reason.what()), reason.level() + 1) { }

        /** Get reason from nested non-oks exception. **/
      FailedReadObject(const OksObject * o, const std::string& what, const std::string& reason) noexcept : exception (fill(o, what, reason), 0) { }

      virtual ~FailedReadObject() noexcept { }


    private:

      static std::string fill(const OksObject * o, const std::string& what, const std::string& reason);
  };


    /** Failed to save an object. **/

  class FailedSaveObject : public exception {

    public:

        /** Get reason from nested oks exception. **/
      FailedSaveObject(const OksObject * o, const exception& reason) noexcept : exception (fill(o, reason.what()), reason.level() + 1) { }

        /** Get reason from nested non-oks exception. **/
      FailedSaveObject(const OksObject * o, const std::string& reason) noexcept : exception (fill(o, reason), 0) { }

      virtual ~FailedSaveObject() noexcept { }


    private:

      static std::string fill(const OksObject * o, const std::string& reason);

  };


    /** Failed to rename an object. **/

  class FailedRenameObject : public exception {

    public:

        /** Get reason from nested oks exception. **/
      FailedRenameObject(const OksObject * o, const exception& reason) noexcept : exception (fill(o, reason.what()), reason.level() + 1) { }

        /** Get reason from nested non-oks exception. **/
      FailedRenameObject(const OksObject * o, const std::string& reason) noexcept : exception (fill(o, reason), 0) { }

      virtual ~FailedRenameObject() noexcept { }


    private:

      static std::string fill(const OksObject * o, const std::string& reason);

  };


    /** Failed to rename an object. **/

  class FailedDestoyObject : public exception {

    public:

        /** Get reason from nested oks exception. **/
      FailedDestoyObject(const OksObject * o, const exception& reason) noexcept : exception (fill(o, reason.what()), reason.level() + 1) { }

        /** Get reason from nested non-oks exception. **/
      FailedDestoyObject(const OksObject * o, const std::string& reason) noexcept : exception (fill(o, reason), 0) { }

      virtual ~FailedDestoyObject() noexcept { }


    private:

      static std::string fill(const OksObject * o, const std::string& reason);

  };


    /** Failed to read a value since it is out of range. **/

  class AttributeRangeError : public exception {

    public:

        /** Get reason from nested non-oks exception. **/
      AttributeRangeError(const OksData * d, const std::string& range) noexcept : exception (fill(d, range), 0) { }

      virtual ~AttributeRangeError() noexcept { }


    private:

      static std::string fill(const OksData * d, const std::string& range);

  };


    /** Failed to read a value since it does not match to expected type. **/

  class AttributeReadError : public exception {

    public:

        /** Get reason from nested oks exception. **/
      AttributeReadError(const char * value, const char * type, const exception& reason) noexcept : exception (fill(value, type, reason.what()), reason.level() + 1) { }

        /** Get reason from nested non-oks exception. **/
      AttributeReadError(const char * value, const char * type, const std::string& reason) noexcept : exception (fill(value, type, reason), 0) { }

        /** Get reason from nested oks exception. **/
      AttributeReadError(const char * type, const exception& reason) noexcept : exception (fill(type, reason.what()), reason.level() + 1) { }

        /** Get reason from nested non-oks exception. **/
      AttributeReadError(const char * type, const std::string& reason) noexcept : exception (fill(type, reason), 0) { }

      virtual ~AttributeReadError() noexcept { }


    private:

      static std::string fill(const char * value, const char * type, const std::string& reason);
      static std::string fill(const char * type, const std::string& reason);

  };


    /** Failed to set attribute or relationship value. **/

  class ObjectSetError : public exception {

    public:

        /** Get reason from nested oks exception. **/
      ObjectSetError(const OksObject * obj, bool is_rel, const std::string& name, const exception& reason) noexcept : exception (fill(obj, is_rel, name, reason.what()), reason.level() + 1) { }

        /** Get reason from nested non-oks exception. **/
      ObjectSetError(const OksObject * obj, bool is_rel, const std::string& name, const std::string& reason) noexcept : exception (fill(obj, is_rel, name, reason), 0) { }

      virtual ~ObjectSetError() noexcept { }


    private:

      static std::string fill(const OksObject * obj, bool is_rel, const std::string& name, const std::string& reason);

  };


    /** Failed to get attribute or relationship value. **/

  class ObjectGetError : public exception {

    public:

        /** Get reason from nested oks exception. **/
      ObjectGetError(const OksObject * obj, bool is_rel, const std::string& name, const exception& reason) noexcept : exception (fill(obj, is_rel, name, reason.what()), reason.level() + 1) { }

        /** Get reason from nested non-oks exception. **/
      ObjectGetError(const OksObject * obj, bool is_rel, const std::string& name, const std::string& reason) noexcept : exception (fill(obj, is_rel, name, reason), 0) { }

      virtual ~ObjectGetError() noexcept { }


    private:

      static std::string fill(const OksObject * obj, bool is_rel, const std::string& name, const std::string& reason);

  };


    /** Failed to init object. **/

  class ObjectInitError : public exception {

    public:

        /** Get reason from nested oks exception. **/
      ObjectInitError(const OksObject * obj, const std::string& why, const exception& reason) noexcept : exception (fill(obj, why, reason.what()), reason.level() + 1) { }

        /** Get reason from nested non-oks exception. **/
      ObjectInitError(const OksObject * obj, const std::string& why, const std::string& reason) noexcept : exception (fill(obj, why, reason), 0) { }

      virtual ~ObjectInitError() noexcept { }


    private:

      static std::string fill(const OksObject * obj, const std::string& why, const std::string& reason);

  };



    /** Failed to init object. **/

  class ObjectBindError : public exception {

    public:

        /** Get reason from nested oks exception. **/
      ObjectBindError(const OksObject * obj, const OksData * data, const OksRelationship * rel, bool is_error, const std::string& why, const exception& reason) noexcept :
        exception (fill(obj, rel, why, reason.what()), reason.level() + 1),
	p_data(data), p_is_error(is_error) { }

        /** Get reason from nested non-oks exception. **/
      ObjectBindError(const OksObject * obj, const OksData * data, const OksRelationship * rel, bool is_error, const std::string& why, const std::string& reason) noexcept :
        exception (fill(obj, rel, why, reason), 0),
	p_data(data), p_is_error(is_error) { }

      virtual ~ObjectBindError() noexcept { }

      const OksData * p_data;
      const OksRelationship * p_rel;
      bool p_is_error;

    private:

      std::string fill(const OksObject * obj, const OksRelationship * rel, const std::string& why, const std::string& reason);

  };



    /** Failed to add RCR value. **/

  class AddRcrError : public exception {

    public:

        /** Get reason from nested non-oks exception. **/
      AddRcrError(const OksObject * obj, const std::string& name, const OksObject * p1, const OksObject * p2) noexcept : exception (fill(obj, name, p1, p2), 0) { }

      virtual ~AddRcrError() noexcept { }


    private:

      static std::string fill(const OksObject * obj, const std::string& name, const OksObject * p1, const OksObject * p2);

  };

}

    /// Struct OKS data information.
    /**
     *  @ingroup oks
     *
     *  The struct OksDataInfo stores information about offset of attribute
     *  or relationship value (i.e. OksData) inside DATA array
     */

struct OksDataInfo {

    /// Declare map of pointers to OksDataInfo (unsorted by name)

  typedef config::map<OksDataInfo *> Map;


    /// Constructors

  OksDataInfo(size_t o, const OksAttribute * a) : offset (o), attribute (a), relationship(nullptr)  { ; }
  OksDataInfo(size_t o, const OksRelationship * r) : offset (o), attribute (nullptr), relationship(r)  { ; }

  size_t                   offset;			// offset of OKS data in the object
  const OksAttribute *     attribute;
  const OksRelationship *  relationship;
};


    /// Class OKS string.
    /**
     *  @ingroup oks
     *
     *  The class OksString is inherited from C++ Standard Library
     *  string class but uses private memory allocator for instances
     *  (is used for performance optimisation)
     */

class OksString : public std::string {
  friend class OksKernel; /// to deallocate memory when destroyed

  public:

    OksString	() {;}
    OksString	(const std::string& s) : std::string(s) {;}
    OksString	(const char * s) : std::string(s) {;}
    OksString	(const char * s, size_t n) : std::string(s,n) {;}
    OksString	(const std::string&s, std::string::size_type n) : std::string(s,0,n) {;}

    void *      operator new(size_t) {return boost::fast_pool_allocator<OksString>::allocate();}
    void        operator delete(void *ptr) {boost::fast_pool_allocator<OksString>::deallocate(reinterpret_cast<OksString*>(ptr));}
};


  /// Implementation of OksString logical equality operator

inline bool
operator==(const OksString& s1, const OksString& s2)
{
  return ( *(static_cast<const std::string *>(&s1)) == *(static_cast<const std::string *>(&s2)) );
}


  /// Implementation of OksString logical equality operator

inline bool
operator==(const OksString& s1, const std::string& s2)
{
  return ( *(static_cast<const std::string *>(&s1)) == s2 );
}


  /// Implementation of OksString logical equality operator

inline bool
operator==(const OksString& s1, const char *s2)
{
  return (*(static_cast<const std::string *>(&s1)) == s2);
}


  /// Implementation of OksString logical less operator

inline bool
operator<(const OksString& s1, const OksString& s2)
{
  return ( *(static_cast<const std::string *>(&s1)) < *(static_cast<const std::string *>(&s2)) );
}


  /// Implementation of OksString out stream operator

inline std::ostream&
operator<<(std::ostream& s, const OksString& str)
{
  s << *(static_cast<const std::string *>(&str));
  
  return s;
}


  // forward declaration for private OKS structures declared in non-installing src/oks_utils.h

namespace oks {
  struct ReloadObjects;    ///< the structure for efficient search of objects to be re-read or to be deleted during reload
  struct ReadFileParams;   ///< the structure to pass common parameters to various read() methods of OksData and OksObject class
}


    /// Struct OKS data.
    /**
     *  @ingroup oks
     *
     *  The struct OksData is used to present OKS data types
     *  (the type is unknown before run-time)
     *  Members:
     *    enumeration 'type' is used to define data type in run-time
     *    union 'data' is used to represent such type
     */

struct OksData {
  friend class OksObject;
  friend class OksKernel;

  public:

    typedef std::list<OksData *, boost::fast_pool_allocator<OksData *> > List;

    enum Type {
      unknown_type	= 0,
      s8_int_type	= 1,
      u8_int_type	= 2,
      s16_int_type	= 3,
      u16_int_type	= 4,
      s32_int_type	= 5,
      u32_int_type	= 6,
      s64_int_type	= 7,
      u64_int_type	= 8,
      float_type	= 9,
      double_type	= 10,
      bool_type		= 11,
      class_type        = 12,
      object_type	= 13,
      date_type		= 14,
      time_type		= 15,
      string_type	= 16,
      list_type		= 17,
      uid_type		= 18,
      uid2_type		= 19,
      enum_type		= 20
    } type;

    union Data {
      int8_t		       S8_INT;
      uint8_t	               U8_INT;
      int16_t 		       S16_INT;
      uint16_t	               U16_INT;
      int32_t 		       S32_INT;
      uint32_t	               U32_INT;
      int64_t                  S64_INT;
      uint64_t                 U64_INT;
      float                    FLOAT;
      double                   DOUBLE;
      bool                     BOOL;
      const OksClass *         CLASS;
      OksObject *              OBJECT;
      uint32_t                 DATE; // boost::gregorian::date    => ugly, but member with constructor not allowed in union
      uint64_t                 TIME; // boost::posix_time::ptime  => ......................................................
      OksString *	       STRING;
      List *                   LIST;
      struct {
        const OksClass *         class_id;
        OksString *              object_id;
      }                        UID;
      struct {
        OksString *              class_id;
        OksString *              object_id;
      }                        UID2;
      const std::string *      ENUMERATION;
    } data;

    OksData()                                           {Clear2();}
    OksData(int8_t c)                                   {Clear2(); Set(c);}
    OksData(uint8_t c)                                  {Clear2(); Set(c);}
    OksData(int16_t i)                                  {Clear2(); Set(i);}
    OksData(uint16_t i)                                 {Clear2(); Set(i);}
    OksData(int32_t i)                                  {Clear2(); Set(i);}
    OksData(uint32_t i)                                 {Clear2(); Set(i);}
    OksData(int64_t i)                                  {Clear2(); Set(i);}
    OksData(uint64_t i)                                 {Clear2(); Set(i);}
    OksData(const float& f)                             {Clear2(); Set(f);}
    OksData(const double& d)                            {Clear2(); Set(d);}
    OksData(bool b)                                     {Clear2(); Set(b);}
    OksData(boost::gregorian::date d)                   {Clear2(); Set(d);}
    OksData(boost::posix_time::ptime t)                 {Clear2(); Set(t);}
    OksData(OksString *s)                               {Clear2(); Set(s);}
    OksData(const char *s)                              {Clear2(); Set(s);}
    OksData(const std::string &s)                       {Clear2(); Set(s);}
    OksData(const OksString &s)                         {Clear2(); Set(s);}
    OksData(const char *s, size_t len, const OksAttribute *a) {Clear2(); SetE(s,len,a);}
    OksData(const std::string &s, const OksAttribute *a){Clear2(); SetE(s,a);}
    OksData(const OksString &s, const OksAttribute *a)  {Clear2(); SetE(s,a);}
    OksData(const OksClass * c)                         {Clear2(); Set(c);}
    OksData(List *l)                                    {Clear2(); Set(l);}
    OksData(OksObject *o)                               {Clear2(); Set(o);}
    OksData(const OksClass *c, const char * o)          {Clear(); Set(c, o);}
    OksData(const OksClass *c, const OksString& o)      {Clear(); Set(c, o);}
    OksData(const std::string &c, const std::string &o) {Clear2(); Set(c, o);}

    ~OksData()                                          {Clear();}

    void Clear();
    void Clear2()                                       {type = unknown_type;}

    void Set(int8_t c)			                {Clear(); type = s8_int_type; data.S8_INT = c;}
    void Set(uint8_t c)			                {Clear(); type = u8_int_type; data.U8_INT = c;}
    void Set(int16_t i)			                {Clear(); type = s16_int_type; data.S16_INT = i;}
    void Set(uint16_t i)		                {Clear(); type = u16_int_type; data.U16_INT = i;}
    void Set(int32_t i)			                {Clear(); type = s32_int_type; data.S32_INT = i;}
    void Set(uint32_t i)		                {Clear(); type = u32_int_type; data.U32_INT = i;}
    void Set(int64_t i)			                {Clear(); type = s64_int_type; data.S64_INT = i;}
    void Set(uint64_t i)		                {Clear(); type = u64_int_type; data.U64_INT = i;}
    void Set(const float& f)		                {Clear(); type = float_type; data.FLOAT = f;}
    void Set(const double& d)		                {Clear(); type = double_type; data.DOUBLE = d;}
    void Set(bool b)			                {Clear(); type = bool_type; data.BOOL = b;}
    void SetFast(boost::gregorian::date d);
    void SetFast(boost::posix_time::ptime t);
    void Set(boost::gregorian::date d)                  {Clear(); type = date_type; SetFast(d);}
    void Set(boost::posix_time::ptime t)                {Clear(); type = time_type; SetFast(t);}
    void Set(OksString *s)		                {Clear(); type = string_type; data.STRING = s;}
    void Set(const char *s)		                {Clear(); type = string_type; data.STRING = new OksString(s);}
    void Set(const std::string &s)	                {Clear(); type = string_type; data.STRING = new OksString(s);}
    void Set(const OksString &s)	                {Clear(); type = string_type; data.STRING = new OksString(s);}
    void Set(const OksClass * c)                        {Clear(); type = class_type; data.CLASS = c;}
    inline void SetE(OksString *s, const OksAttribute *a);
    inline void SetE(const char *s, size_t len, const OksAttribute *a);
    inline void SetE(const std::string &s, const OksAttribute *a);
    inline void SetE(const OksAttribute *a);
    inline void SetE(const OksString &s, const OksAttribute *a);
    void Set(List * l)			                {Clear(); type = list_type; data.LIST = l;}
    void Set(OksObject *o)		                {Clear(); type = object_type; data.OBJECT = o;}
    void Set(const OksClass *c, const OksString& s)     {Clear(); type = uid_type; data.UID.class_id = c; data.UID.object_id = new OksString(s);}
    void Set(const OksClass *c, OksString* s)           {Clear(); type = uid_type; data.UID.class_id = c; data.UID.object_id = s;}
    void Set(const OksString &c, const OksString &s)    {Clear(); type = uid2_type; data.UID2.class_id = new OksString(c); data.UID2.object_id = new OksString(s);}
    void Set(OksString *c, OksString *s)                {Clear(); type = uid2_type; data.UID2.class_id = c; data.UID2.object_id = s;}

    /**
     *  Read from string using non multi-value type
     *  \throw oks::AttributeReadError is thrown in case of problems.
     */
    inline void ReadFrom(const char *, Type, const OksAttribute *);

    /**
     *  Read from string using non multi-value type
     *  \throw oks::AttributeReadError is thrown in case of problems.
     */
    inline void ReadFrom(const std::string&, const OksAttribute *);

    /**
     *  Read from null string (single-value)
     *  \throw oks::AttributeReadError is thrown in case of problems.
     */
    void SetNullValue(const OksAttribute * a);

    /**
     *  Read from string (single-value)
     *  \throw oks::AttributeReadError is thrown in case of problems.
     */
    void SetValue(const char * s, const OksAttribute * a);

    /**
     *  Read from string (any type)
     *  \throw oks::AttributeReadError is thrown in case of problems.
     */
    void SetValues(const char *, const OksAttribute * a);

    /**
     *  Create emtpy list or empty uid2
     */
    void ReadFrom(const OksRelationship *) noexcept;

    void WriteTo(OksXmlOutputStream&, bool) const;

    void WriteTo(OksXmlOutputStream&) const;

    void ConvertTo(OksData *, const OksRelationship *) const;

    boost::gregorian::date date() const noexcept;
    boost::posix_time::ptime time() const noexcept;


      /**
       *  \brief Convert data to new type.
       *
       *  The method converts this data to the new data using type defined by attribute parameter.
       *  A single-value can be converted to multi-value conta4ining this single item.
       *  A multi-value can be converted to single-value using first item from the multi-value.
       *
       *  If data cannot be converted, then the oks::AttributeReadError exception is thrown.
       *
       *  \param to    out parameter (new data containing converted value)
       *  \param attr  pointer to OKS kernel (required to read class_type values)
       *
       *  \throw oks::AttributeReadError is thrown in case of problems.
       */

    void cvt(OksData * to, const OksAttribute * attr) const;

    std::string str(int base = 0) const;
    std::string str(const OksKernel *) const;


      /**
       *  \brief Check range of data.
       *  
       *  The method checks that the data matches to the range.
       *  If the data is out of range, the oks::AttributeRangeError exception is thrown.
       *  If definition of range is bad, then the oks::AttributeReadError exception is thrown.
       *
       *  \param a   attribute defining the range (UML syntax, e.g. "A,B,C" or "1..10" or "*..99")
       *
       *  \throw oks::AttributeRangeError or oks::AttributeReadError is thrown in case of problems.
       */

    void check_range(const OksAttribute * a) const;


      /**
       *  \brief Set value defined by initial value of attribute.
       *  
       *  The method checks that the data matches to the range.
       *  If the data is out of range, the oks::AttributeRangeError exception is thrown.
       *  If definition of range is bad, then the oks::AttributeReadError exception is thrown.
       *
       *  \param attr       the attribute
       *  \param kernel     pointer to OKS kernel (required to read class_type values)
       *  \param skip_init  only required by OksObject::init2() method for optimization
       *
       *  \throw oks::AttributeReadError is thrown in case of problems.
       */

    void set_init_value(const OksAttribute * attr, bool skip_init);

    OksData& operator=(const OksData& d)
    {
      if(&d != this) {
        Clear();
        copy(d);
      }

      return *this;
    }

    OksData(const OksData& d)
    {
      if(&d != this) {
        copy(d);
      }
    }

    bool operator==(const OksData &) const;
    bool operator!=(const OksData &) const;
    bool operator<=(const OksData &) const; //report incompatible data types
    bool operator>=(const OksData &) const; //report incompatible data types
    bool operator<(const OksData &) const;  //report incompatible data types
    bool operator>(const OksData &) const;  //report incompatible data types
    friend std::ostream& operator<<(std::ostream&, const OksData&);
    void* operator new(size_t) {return boost::fast_pool_allocator<OksData>::allocate();}
    void operator delete(void *ptr) {boost::fast_pool_allocator<OksData>::deallocate(reinterpret_cast<OksData*>(ptr));}

    void sort(bool ascending = true);

    static bool is_object(Type t)
    {
      return (t == object_type || t == uid_type || t == uid2_type);
    }

  private:

    void copy(const OksData &);

    bool is_le(const OksData &) const noexcept; // fast version of operator<=(const OksData &)
    bool is_ge(const OksData &) const noexcept; // fast version of operator>=(const OksData &)
    bool is_l(const OksData &) const noexcept;  // fast version of operator<(const OksData &)
    bool is_g(const OksData &) const noexcept;  // fast version of operator>(const OksData &)

      /// private methods which can be used by OksObject class only

    void read(const oks::ReadFileParams&, const OksAttribute *, int32_t);     // read mv attribute
    void read(const oks::ReadFileParams&, const OksAttribute *);              // read sv attribute
    void read(const OksAttribute *, const OksXmlValue&);                      // read sv attribute for "data" format
    void read(const OksAttribute *, const oks::ReadFileParams&);              // read mv attribute for "data" format

    void read(const oks::ReadFileParams&, const OksRelationship*, int32_t);   // read mv relationship
    void read(const oks::ReadFileParams&, const OksRelationship*);            // read sv relationship
    void read(const OksRelationship *, const OksXmlRelValue&);                // read sv relationship for "data" format
    void read(const OksRelationship *, const oks::ReadFileParams&);           // read mv relationship for "data" format

    OksData(const oks::ReadFileParams& params, const OksAttribute * a, int32_t n) {Clear2(); read(params, a, n);}
    OksData(const oks::ReadFileParams& params, const OksAttribute * a) {Clear2(); read(params, a);}
    OksData(const OksAttribute * a, const OksXmlValue& value) {Clear2(); read(a, value);}
    OksData(const OksAttribute * a, const oks::ReadFileParams& params) {Clear2(); read(a, params);}

    OksData(const oks::ReadFileParams& params, const OksRelationship *r, int32_t n) {Clear2(); read(params, r, n);}
    OksData(const oks::ReadFileParams& params, const OksRelationship *r) {Clear2(); read(params, r);}
    OksData(const OksRelationship * r, const OksXmlRelValue& value) {Clear2(); read(r, value);}
    OksData(const OksRelationship * r, const oks::ReadFileParams& params) {Clear2(); read(r, params);}

    void WriteTo(OksXmlOutputStream&, OksKernel *, OksAliasTable *, bool, bool) const;

    bool IsConsistent(const OksRelationship *r, const OksObject *o, const char *msg);

    const std::string& __oid() const;
    const std::string& __cn() const;
};


inline void
OksData::ReadFrom(const char * s, Type t, const OksAttribute * a)
{
  Clear();
  type = t;
  SetValue(s, a);
}


  /**
   *  @ingroup oks
   *
   *  \brief The struct OksRCR describes Reverse Composite Relationship
   *  (i.e. back reference from child to composite parent)
   */

struct OksRCR {
  friend class OksKernel;
  
  public:
  
    OksRCR		    (OksObject *o, const OksRelationship *r) :
                              obj (o), relationship (r) {;}

    void *                  operator new(size_t) {return boost::fast_pool_allocator<OksRCR>::allocate();}
    void                    operator delete(void *ptr) {boost::fast_pool_allocator<OksRCR>::deallocate(reinterpret_cast<OksRCR*>(ptr));}

    OksObject *		    obj;
    const OksRelationship * relationship;

};

namespace oks {
  struct hash_str
  {
    inline size_t operator() ( const std::string * x ) const {
      return std::hash<std::string>()(*x);
    }
  };

  struct equal_str
  {
    inline bool operator()(const std::string * __x, const std::string * __y) const {
      return ((__x == __y) || (*__x == *__y));
    }
  };

  struct hash_obj_ptr
  {
    inline size_t operator() ( const OksObject * x ) const {
      return reinterpret_cast<size_t>(x);
    }
  };

  struct equal_obj_ptr
  {
    inline bool operator()(const OksObject * __x, const OksObject * __y) const {
      return (__x == __y);
    }
  };

  struct hash_class_ptr
  {
    inline size_t operator() ( const OksClass * x ) const {
      return reinterpret_cast<size_t>(x);
    }
  };

  struct equal_class_ptr
  {
    inline bool operator()(const OksClass * __x, const OksClass * __y) const {
      return (__x == __y);
    }
  };
  
  typedef std::unordered_set<const OksClass *, oks::hash_class_ptr, oks::equal_class_ptr> ClassSet;
}


  /**
   *  @ingroup oks
   *
   *  \brief OksObject describes instance of OksClass
   *
   *  This class implements OKS object that is an instance of OKS class.
   *  Each object has unique ID string in a scope of class and its derived sub-classes.
   *  The properties of object are described by values of its attributes (i.e. values of
   *  primitive types like strings, integers, floats, see also OksAttribute)
   *  and relationships (i.e. references on others OKS objects, see also OksRelationship).
   */

class OksObject
{
  friend class	OksClass;
  friend struct	OksData;
  friend class	OksKernel;
  friend class	OksIndex;
  friend class	OksObjectSortBy;
  friend struct OksLoadObjectsJob;
  friend struct oks::ReadFileParams;
  friend struct oks::ReloadObjects;

  public:

    typedef void (*notify_obj)(OksObject *, void *);

      /** Functor to sort map of OksObject pointers by object Id */

    struct SortById {
      bool operator() (const std::string * s1, const std::string * s2) const {
        return *s1 < *s2;
      }
    };

      /** Map of pointers to OksObject sorted by object Id */

    typedef std::map<const std::string *, OksObject *, SortById> SMap;


      /** Map of pointers to OksObject (unsorted) */

    typedef std::unordered_map<const std::string *, OksObject *, oks::hash_str, oks::equal_str> Map;
    typedef std::unordered_set<OksObject *, oks::hash_obj_ptr, oks::equal_obj_ptr> FSet;


      /** Set of pointers to OksObject */

    typedef std::set<OksObject *, std::less<OksObject *> > Set;

      /** List of pointers to OksObject */

    typedef std::list<OksObject *> List;

    typedef std::list<OksObject *, boost::fast_pool_allocator<OksObject *> > FList;


      /**
       * \brief OKS object constructor.
       *
       *  Create new OKS object providing class and object ID.
       *  If the object ID is not set, the OKS creates new object with random unique ID.
       *  To create a new object the OKS kernel active data file has to be set.
       *
       *  In case of problems the error message is printed and the uid.class_id is set to 0 to flag the error.
       *
       *  The parameters are:
       *  \param oks_class    pointer to OKS class
       *  \param object_id    optional object unique ID
       *  \param skip_init    if true, skip initialization of OKS data
       */

    OksObject (const OksClass * oks_class, const char * object_id = 0, bool skip_init = false);


      /**
       * \brief OKS object copy constructor.
       *
       *  Create new OKS object as copy of parent.
       *  If the object ID is not set, the OKS creates new object with random unique ID.
       *  To create a new object the OKS kernel active data file has to be set.
       *
       *  In case of problems the error message is printed and the uid.class_id is set to 0 to flag the error.
       *
       *  The parameters are:
       *  \param parent_object   pointer to OKS class
       *  \param object_id       optional object unique ID
       */

    OksObject (const OksObject & parent_object, const char * object_id = 0);


      /**
       * \brief Destroy OKS object.
       *
       *  This the the only way available to user to destroy OKS object since ~OksObject() is private.
       *
       *  When an object is destroyed, it shall not be referenced by other objects.
       *
       *  In case of problems the oks::exception is thrown.
       *
       *  The parameters are:
       *  \param obj       the object to be destroyed
       *  \param fast      if true, skip extra protection checks
       */

    static void destroy(OksObject * obj, bool fast = false);


      /**
       * \brief The equality operator.
       *
       *  The operator returns true, if comparing objects belong to the same class
       *  or classes with equal names (e.g. belonging to different OksKernels)
       *  and their attributes and relationships are equal.
       */

    bool operator==(const OksObject&) const;


      /**
       * \brief The fast equal method.
       *
       *  This method replaces operator!= used by OKS library in the past,
       *  since the operator causes problems starting from gcc62 assuming &reference != nullptr
       *
       *  The method returns false, if comparing objects belong to classes with different names or their IDs are not equal.
       */

    static bool
    are_equal_fast(const OksObject * o1, const OksObject * o2);

    bool operator!=(const OksObject&) const = delete;


      /** Fast new operator to reduce resources consumption. */

    void * operator new(size_t) {return boost::fast_pool_allocator<OksObject>::allocate();}


      /** Fast delete operator. */

    void operator delete(void *ptr) {boost::fast_pool_allocator<OksObject>::deallocate(reinterpret_cast<OksObject*>(ptr));}


      /**
       * \brief Detailed stream operator.
       *
       *  Send to stream complete description of OKS object:
       *  - print class name and ID
       *  - print value of all attributes and relationships
       */

    friend std::ostream& operator<<(std::ostream& out, const OksObject & obj);


      /**
       * \brief Short stream operator.
       *
       *  Send to stream OKS object reference in format "foo@bar" (i.e. object "foo" from class "bar")
       */

    friend std::ostream& operator<<(std::ostream&, const OksObject *);


      /** Get class of object. */

    const OksClass * GetClass() const {return uid.class_id;}


     /** Get object ID. */

    const std::string& GetId() const {return uid.object_id;}


      /**
       * \brief Set object ID.
       *
       *  The ID has to be unique in scope of object's class
       *  and all classes participating in inheritance hierarchy of this class.
       *
       *  \param id    new identity of the object
       *  In case of problems the oks::exception is thrown.
       */

    void set_id(const std::string & id);


     /** Get file of object. */

    OksFile * get_file() const {return file;}


      /**
       * \brief Move object to different file.
       *
       *  Move object to an existing file. Both, present file where object is stored
       *  and new destination file have to be writable by user.
       *
       *  The parameters are:
       *  \param file           destination file
       *  \param update_owner   mark original file as updated
       *
       *  \throw oks::exception is thrown in case of problems.
       */

    void set_file(OksFile * file, bool update_owner = true);


      /**
       * \brief Get value of attribute by name.
       *
       *  The method returns pointer on OksData value for given attribute.
       *
       *  In case of problems (e.g. no attribute with such name) the oks::exception is thrown.
       *
       *  The parameter is:
       *  \param name  name of attribute
       *
       *  \return      the OKS data value for given attribute
       *
       *  \throw oks::exception is thrown in case of problems.
       */

     OksData * GetAttributeValue(const std::string& name) const;


      /**
       * \brief Get value of attribute by offset.
       *
       *  The method returns pointer on OksData value for given attribute offset.
       *  The method is optimised for performance and does not check validity of offset.
       *
       *  The parameter is:
       *  \param data_info  describes offset of attribute's value for given OKS class
       *
       *  \return           the OKS data value for given attribute
       */

    OksData * GetAttributeValue(const OksDataInfo *i) const noexcept { return &(data[i->offset]); }


      /**
       * \brief Get value of relationship by name.
       *
       *  The method returns pointer on OksData value for given relationship.
       *
       *  In case of problems (e.g. no relationship with such name) the oks::exception is thrown.
       *
       *  The parameter is:
       *  \param name  name of relationship
       *
       *  \return the OKS data value for given relationship
       *
       *  \throw oks::exception is thrown in case of problems.
       */

    OksData * GetRelationshipValue(const std::string&) const;


      /**
       * \brief Get value of relationship by offset.
       *
       *  The method returns pointer on OksData value for given relationship offset.
       *  The method is optimised for performance and does not check validity of offset.
       *
       *  The parameter is:
       *  \param data_info  describes offset of relationship's value for given OKS class
       *
       *  \return           the OKS data value for given relationship
       */

    OksData * GetRelationshipValue(const OksDataInfo *i) const noexcept { return &(data[i->offset]); }


      /**
       * \brief Set value of attribute by name.
       *
       *  It the type of data do not match to the defined by the attribute of object's class,
       *  then the method tries to convert them and to print out warning message.
       *
       *  In case of problems (e.g. no attribute with such name, data are out of range) the oks::exception is thrown.
       *
       *  The parameters are:
       *  \param name    name of attribute
       *  \param data    new attribute value
       *
       *  \throw oks::exception is thrown in case of problems.
       */

    void SetAttributeValue(const std::string& name, OksData * data);


      /**
       * \brief Set value of attribute by offset.
       *
       *  It the type of data do not match to the defined by the attribute of object's class,
       *  then the method tries to convert them and to print out warning message.
       *
       *  In case of problems (e.g. data are out of range) the oks::exception is thrown.
       *
       *  The parameters are:
       *  \param data_info    describes offset of attribute's value for given OKS class
       *  \param data         new attribute value
       *
       *  \throw oks::exception is thrown in case of problems.
       */

    void SetAttributeValue(const OksDataInfo * data_info, OksData * data);


      /**
       * \brief Set value of relationship by name.
       *
       *  In case of problems (e.g. no relationship with such name, wrong class type or cardinality constraint) the oks::exception is thrown.
       *
       *  The parameters are:
       *  \param name                  name of relationship
       *  \param data                  new relationship value
       *  \param skip_non_null_check   if true, ignore low cardinality constraint
       *
       *  \throw oks::exception is thrown in case of problems.
       */

    void SetRelationshipValue(const std::string& name, OksData * data, bool skip_non_null_check = false);


      /**
       * \brief Set value of relationship by offset.
       *
       *  In case of problems (e.g. wrong class type or cardinality constraint) the oks::exception is thrown.
       *
       *  The parameters are:
       *  \param data_info             describes offset of relationship's value for given OKS class
       *  \param data                  new relationship value
       *  \param skip_non_null_check   if true, ignore low cardinality constraint
       *
       *  \throw oks::exception is thrown in case of problems.
       */

    void SetRelationshipValue(const OksDataInfo * data_info, OksData * data, bool skip_non_null_check = false);


      /**
       * \brief Set value of single-value relationship by name.
       *
       *  In case of problems (e.g. no relationship with such name, wrong class type or relationship is multi-value) the oks::exception is thrown.
       *
       *  The parameters are:
       *  \param name    name of relationship
       *  \param object  new relationship value
       *
       *  \throw oks::exception is thrown in case of problems.
       */

    void SetRelationshipValue(const std::string& name, OksObject * object);


      /**
       * \brief Set value of single-value relationship by offset.
       *
       *  In case of problems (e.g. wrong class type or relationship is multi-value) the oks::exception is thrown.
       *
       *  The parameters are:
       *  \param data_info    describes offset of relationship's value for given OKS class
       *  \param object       new relationship value
       *
       *  \throw oks::exception is thrown in case of problems.
       */

    void SetRelationshipValue(const OksDataInfo * data_info, OksObject * object);


      /**
       * \brief Add object value to multi-value relationship by name.
       *
       *  In case of problems (e.g. no relationship with such name, wrong class type, relationship is single-value or
       *  RCR cannot be set) the oks::exception is thrown.
       *
       *  The parameters are:
       *  \param name    name of relationship
       *  \param object  the object to be added
       *
       *  \throw oks::exception is thrown in case of problems.
       */

    void AddRelationshipValue(const std::string& name, OksObject * object);


      /**
       * \brief Add object value to multi-value relationship by offset.
       *
       *  In case of problems (e.g. wrong class type, relationship is single-value or RCR cannot be set) the oks::exception is thrown.
       *
       *  The parameters are:
       *  \param data_info    describes offset of relationship's value for given OKS class
       *  \param object       the object to be added
       *
       *  \throw oks::exception is thrown in case of problems.
       */

    void AddRelationshipValue(const OksDataInfo * data_info, OksObject * object);


      /**
       * \brief Remove object value from multi-value relationship by name.
       *
       *  In case of problems (e.g. no relationship with such name, relationship is single-value or there is no such object) the oks::exception is thrown.
       *
       *  The parameters are:
       *  \param name    name of relationship
       *  \param object  the object to be removed from relationship
       *
       *  \throw oks::exception is thrown in case of problems.
       */

    void RemoveRelationshipValue(const std::string& name, OksObject * object);


      /**
       * \brief Remove object value from multi-value relationship by offset.
       *
       *  In case of problems (e.g. relationship is single-value or there is no such object) the oks::exception is thrown.
       *
       *  The parameters are:
       *  \param data_info    describes offset of relationship's value for given OKS class
       *  \param object       the object to be removed from relationship
       *
       *  \throw oks::exception is thrown in case of problems.
       */

    void RemoveRelationshipValue(const OksDataInfo * data_info, OksObject * object);


      /**
       * \brief Set class-name and object-id value of single-value relationship by name.
       *
       *  In case of problems (e.g. no relationship with such name, wrong class type or relationship is multi-value) the oks::exception is thrown.
       *
       *  The parameters are:
       *  \param rel_name    name of relationship
       *  \param class_name  class name of the referenced object
       *  \param object_id   ID of the referenced object
       *
       *  \throw oks::exception is thrown in case of problems.
       */

    void SetRelationshipValue(const std::string& rel_name, const std::string& class_name, const std::string& object_id);


      /**
       * \brief Add class-name and object-id value to multi-value relationship by name.
       *
       *  In case of problems (e.g. no relationship with such name, unknown class) the oks::exception is thrown.
       *
       *  The parameters are:
       *  \param rel_name    name of relationship
       *  \param class_name  class name of the referenced object
       *  \param object_id   ID of the referenced object
       *
       *  \throw oks::exception is thrown in case of problems.
       */

    void AddRelationshipValue(const std::string& rel_name, const std::string& class_name, const std::string& object_i);


      /**
       * \brief Remove class-name and object-id value from multi-value relationship by name.
       *
       *  In case of problems (e.g. no relationship with such name, no such object) the oks::exception is thrown.
       *
       *  The parameters are:
       *  \param rel_name    name of relationship
       *  \param class_name  class name of the referenced object
       *  \param object_id   ID of the referenced object
       *
       *  \throw oks::exception is thrown in case of problems.
       */

    void RemoveRelationshipValue(const std::string& rel_name, const std::string& class_name, const std::string& object_i);


      /**
       * \brief Return information about composite parents.
       *  The method returns list of the OKS object's reverse composite relationships.
       */

    const std::list<OksRCR *> *	reverse_composite_rels() const {return p_rcr;}


      /**
       * \brief Return objects referencing this one via relationship with given name.
       *
       *  The method returns list of objects which have a reference on given one.
       *  If the relationship name is set to "*", then the method takes into account  all relationships of all objects.
       *  The method performs full scan of all OKS objects and it is not recommended at large scale to build complete graph of relations between all database object;
       *  if only composite parents are needed, them the reverse_composite_rels() method has to be used.
       *
       *  The parameters are:
       *  \param  name    the name of relationship used to reference given object (by default ANY relationship)
       *
       *  \return the list of objects referencing this one (can be null, if there are no such objects); the user is responsible to destroy the returned list (but not the objects it contains)
       */

    FList * get_all_rels(const std::string& name = "*") const;


      /**
       * \brief Set transient user data associated with given object.
       *  Such data are not stored on file and only exist while OksKernel object is not destroyed.
       */

    void SetTransientData(void *d) const {p_user_data = d;}


      /**
       * \brief Get transient user data associated with given object.
       *  Return data set using SetTransientData() method.
       */

    void * GetTransientData() const {return p_user_data;}


      /**
       * \brief Set object id is used to assign an object unique integer number.
       *  Such data are not stored on file and only exist while OksKernel object is not destroyed.
       */

    void set_int32_id(int32_t object_id) {p_int32_id = object_id;}
    
      /**
       * \brief Get object id associated with given object.
       *  Return integer number set using set_int32_id() method.
       */

    int32_t get_int32_id() const  {return p_int32_id;}

    

      /**
       *  \brief Check if object satisfies query expression.
       *
       *  The method parses query expression structures and applies them to the object's values.
       *  In case of incorrect query expression parameter the method throws exception.
       *
       *  \param query_exp    the query expression
       *  \return true, if object satisfies given query expression.
       *
       *  \throw std::exception is thrown in case of problems.
       */

    bool SatisfiesQueryExpression(OksQueryExpression * query_exp) const;

    bool satisfies(const OksObject * goal, const oks::QueryPathExpression& expresssion, OksObject::List& path) const;
    OksObject::List * find_path(const oks::QueryPath& query) const;

      /** The method checks the schema constraints for given object and the file's includes. Return false, if object is inconsistent. **/
    bool is_consistent(const std::set<OksFile *>&, const char * msg) const;

     
      /** The method returns string containing dangling references for given object. **/
    std::string report_dangling_references() const;

      /** The method puts to set objects recursively referenced by given object. **/
    void references(OksObject::FSet& refs, unsigned long recursion_depth, bool add_self = false, oks::ClassSet * classes = 0) const;


    bool is_duplicated() const { return (p_duplicated_object_id_idx != -1); }


  private:

      // the struct OksUid is used to describe object unique identity
      // in form 'class_name@object_identity'

    struct OksUid {
      const OksClass * class_id;
      std::string object_id;

      OksUid() : class_id(0) {;}
    } uid;

    OksData * data;
    std::list<OksRCR *> * p_rcr;
    mutable void * p_user_data;
    int32_t p_int32_id;
    int32_t p_duplicated_object_id_idx;
    OksFile * file;


      // to be used by OksKernel only

    ~OksObject();


      /**
       *  Read OKS object from input stream.
       *
       *  \return The method returns a pointer to OksObject, or NULL object to indicate end-of-stream.
       *  \throw oks::exception is thrown in case of errors, e.g. bad input stream, duplicated object, abstract class of object.
       **/

    static OksObject * read(const oks::ReadFileParams&);
    void __report_type_cvt_warning(const oks::ReadFileParams&, const OksAttribute&, const OksData&, const OksData::Type, int32_t) const;
    void __report_cardinality_cvt_warning(const oks::ReadFileParams&, const OksRelationship&) const;
    int32_t __get_num(const oks::ReadFileParams& params);



      // these two are used by read() method

    void read_body(const oks::ReadFileParams&, bool);

    /**
     *  Construct OKS object from input stream.
     *  \throw oks::exception is thrown in case of errors.
     **/
    OksObject (const oks::ReadFileParams&, OksClass *, const std::string&);



      // to be used by OksIndex only

    OksObject (size_t, const OksData *);

      // to be used by OksKernel only

    OksObject (OksClass * c, const std::string& id, void * user_data, int32_t object_id, int32_t duplicated_object_id_idx, OksFile * f);

      // is not implemented and must not be used

    OksObject&	operator= (const OksObject &);


      // these 3 methods are used by constructors

    void init1(const OksFile * = 0);
    void init2(bool skip_init = false);
    void init3();
    void init3(OksClass *c);

    void check_ids();


  public:

    void add_RCR(OksObject *, const OksRelationship *);
    void remove_RCR(OksObject *, const OksRelationship *) noexcept;


  private:

    inline void notify() {
      file->set_updated();
      change_notify();
    }

    void check_class_type(const OksRelationship *, const OksClass *);
    void check_class_type(const OksRelationship *r, const OksObject *o) { if(o) { check_class_type(r, o->GetClass()); }}
    void check_non_null(const OksRelationship *, const OksObject *);
    void check_file_lock(const OksAttribute *, const OksRelationship *);


    void set_unique_id();
    void put(OksXmlOutputStream&, bool force_defaults) const;
    static void put_object_attributes(OksXmlOutputStream&, const OksData&);

      // bind data and methods

    struct BindInfo {
      OksKernel *       k;
      OksObject *       o;
      OksRelationship * r;
    };

      /** Binds objects and returns true on success **/

    void			bind_objects();
    static void			bind(OksData *, const BindInfo&);

    void			unbind_file(const OksFile *);


      // check and report to standard out missing file inclusion paths

    bool check_links_and_report(const OksObject *, const std::set<OksFile *>&, const std::string&, const char *) const;


      // valid names of xml tags and attributes

    static const char		obj_xml_tag[];
    static const char		obj2_xml_tag[];
    static const char	 	attribute_xml_tag[];
    static const char 		relationship_xml_tag[];
    static const char 		class_xml_attribute[];
    static const char 		class2_xml_attribute[];
    static const char 		id_xml_attribute[];
    static const char 		id2_xml_attribute[];
    static const char 		name_xml_attribute[];
    static const char 		type_xml_attribute[];
    static const char 		num_xml_attribute[];

    static const char           value_xml_attribute[];
    static const char           data_xml_tag[];
    static const char           ref_xml_tag[];

    notify_obj			get_change_notify() const;

    void			create_notify();
    void			change_notify();
    void			delete_notify();

};


#endif
