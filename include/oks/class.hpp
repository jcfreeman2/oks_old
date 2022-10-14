/**	
 *	\file oks/class.h
 *	
 *	This file is part of the OKS package.
 *      Author: Igor SOLOVIEV "https://phonebook.cern.ch/phonebook/#personDetails/?id=432778"
 *	
 *	This file contains the declarations for the OKS class.
 */

#ifndef OKS_CLASS_H
#define OKS_CLASS_H

#include "oks/index.hpp"
#include "oks/exceptions.hpp"

#include <list>
#include <map>
#include <mutex>
#include <shared_mutex>
#include <string>
#include <vector>

#include <boost/pool/pool_alloc.hpp>




  // Forward declarations

class OksAttribute;
class OksClass;
class OksFile;
class OksKernel;
class OksMethod;
class OksQuery;
class OksQueryExpression;
class OksRelationship;
class OksXmlInputStream;
class OksXmlOutputStream;



  /// @addtogroup oks

namespace oks {

    /** Cannot find super class */

  class CannotFindSuperClass : public exception {

    public:

      CannotFindSuperClass(const OksClass& c, const std::string& name) noexcept : exception (fill(c, name), 0) { }
      virtual ~CannotFindSuperClass() noexcept { }

    private:

      static std::string fill(const OksClass& c, const std::string& name) noexcept;

  };

    /** Cannot register class */

  class AttributeConversionFailed : public exception {

    public:

      AttributeConversionFailed(const OksAttribute& a, const OksObject * o, const exception& reason) noexcept : exception (fill(a, o, reason.what()), reason.level() + 1) { }
      virtual ~AttributeConversionFailed() noexcept { }

    private:

      static std::string fill(const OksAttribute& a, const OksObject * o, const std::string& reason) noexcept;

  };

    /** Cannot register class */

  class CannotRegisterClass : public exception {

    public:

      CannotRegisterClass(const OksClass& c, const std::string& what, const exception& reason) noexcept : exception (fill(c, what, reason.what()), reason.level() + 1) { }
      CannotRegisterClass(const OksClass& c, const std::string& what, const std::string& reason) noexcept : exception (fill(c, what, reason), 0) {}
      virtual ~CannotRegisterClass() noexcept { }

    private:

      static std::string fill(const OksClass& c, const std::string& what, const std::string& reason) noexcept;

  };
  

    /** Cannot destroy class */

  class CannotDestroyClass : public exception {

    public:

      CannotDestroyClass(const OksClass& c, const exception& reason) noexcept : exception (fill(c, reason.what()), reason.level() + 1) { }
      CannotDestroyClass(const OksClass& c, const std::string& reason) noexcept : exception (fill(c, reason), 0) {}
      virtual ~CannotDestroyClass() noexcept { }

    private:

      static std::string fill(const OksClass& c, const std::string& reason) noexcept;

  };
  

    /** Cannot add or remove object of class */

  class ObjectOperationFailed : public exception {

    public:

      ObjectOperationFailed(const OksClass& c, const std::string& oid, const char * op, const exception& reason) noexcept : exception (fill(c, oid, op, reason.what()), reason.level() + 1) { }
      ObjectOperationFailed(const OksClass& c, const std::string& oid, const char * op, const std::string& reason) noexcept : exception (fill(c, oid, op, reason), 0) {}
      virtual ~ObjectOperationFailed() noexcept { }

    private:

      static std::string fill(const OksClass& c, const std::string& oid, const char * op, const std::string& reason) noexcept;

  };


    /** Cannot set some class, attribute, relationship or method property */

  class SetOperationFailed : public exception {

    public:

      SetOperationFailed(const char * op, const exception& reason) noexcept : exception (fill(op, reason.what()), reason.level() + 1) { }
      SetOperationFailed(const char * op, const std::string& reason) noexcept : exception (fill(op, reason), 0) {}
      virtual ~SetOperationFailed() noexcept { }

    private:

      static std::string fill(const char * op, const std::string& reason) noexcept;

  };


    /** Query Failed  */

  class QueryFailed: public exception {

    public:

      QueryFailed(const OksQueryExpression& query, const OksClass& c, const exception& reason) noexcept : exception (fill(query, c, reason.what()), reason.level() + 1) { }
      QueryFailed(const OksQueryExpression& query, const OksClass& c, const std::string& reason) noexcept : exception (fill(query, c, reason), 0) {}
      virtual ~QueryFailed() noexcept { }

    private:

      static std::string fill(const OksQueryExpression& query, const OksClass& c, const std::string& reason) noexcept;

  };


    /** Bad Reqular Expression */

  class BadReqExp: public exception {

    public:

      BadReqExp(const std::string& what, const std::string& reason) noexcept : exception (fill(what, reason), 0) {}
      virtual ~BadReqExp() noexcept { }

    private:

      static std::string fill(const std::string& what, const std::string& reason) noexcept;

  };

}


  /**
   *    @ingroup oks
   *
   *	\brief The OKS class.
   *
   *    An OKS class has name, documentation, attributes (list of OksAttribute objects),
   *    relationships (list of OksRelationship objects) and methods (list of OksMethod objects).
   *
   *    A class may participate in inheritance (multiple): in such case the class inherits
   *    all attributes, relationships and methods of its base classes.
   *
   *    An OKS class can be abstract (i.e. it does not have any instances).
   *    Otherwise the objects of class can be accessed via:
   *    - objects() : get all objects of given class
   *    - create_list_of_all_objects() : get all objects of given and derived classes
   *    - get_object(const std::string&) : get object by unique identity 
   *    - execute_query() : get objects satisfying a query
   *    See OksObject class to get information how to access properties of object,
   *    to create new or to delete existing object.
   *
   *    An OKS class can be made persistent, i.e. stored on OKS schema file.
   */

class OksClass
{
  friend class	OksObject;
  friend class	OksKernel;
  friend class	OksAttribute;
  friend class	OksRelationship;
  friend class	OksMethod;
  friend class	OksMethodImplementation;
  friend class	OksIndex;
  friend class	OksSortedClass;

  public:

#ifndef ERS_NO_DEBUG

      // Declare set of pointers to OksClass sorted by name

    struct SortByName {
      bool operator() (OksClass * c1, OksClass * c2) const {
        return c1->get_name() < c2->get_name();
      }
    };

    typedef std::set<OksClass *, SortByName> Set;

#endif

    struct SortStr {
      bool operator() (const char * c1, const char * c2) const {
        return (strcmp(c1, c2) < 0);
      }
    };


    typedef std::map<const char * , OksClass *, SortStr> Map;

    typedef std::list<OksClass *, boost::fast_pool_allocator<OksClass *> > FList;


      /**
       *  \brief Create OKS class.
       *
       *  The parameters are:
       *  \param name          name of new class
       *  \param kernel        OKS kernel owning this class (if empty, class will not be registered)
       *  \param transient     if true, the class is transient (is used by OKS tools, do not change default value)
       */

    OksClass (const std::string& name, OksKernel * kernel, bool transient = false);


      /**
       *  \brief Create OKS class and set its basic properties.
       *
       *  The parameters are:
       *  \param name          name of new class
       *  \param description   description of new class
       *  \param is_abstract   if true, new class is abstract (i.e. may not have objects)
       *  \param kernel        OKS kernel owning this class (if empty, class will not be registered)
       *  \param transient     if true, the class is transient (is used by OKS tools, do not change default value)
       */

    OksClass (const std::string& name, const std::string& description, bool is_abstract, OksKernel * kernel, bool transient = false);


      /**
       *  \brief Create OKS class for internal usage (fast, no complete registration).
       *
       *  The parameters are:
       *  \param kernel        OKS kernel owning this class
       *  \param name          name of new class
       *  \param description   description of new class
       *  \param is_abstract   if true, new class is abstract (i.e. may not have objects)
       */

    OksClass (OksKernel * kernel, const std::string& name, const std::string& description, bool is_abstract);


      /**
       *  \brief Destroy OKS class.
       *
       *  This the the only way available to user to destroy OKS class since ~OksClass() is private.
       *
       *  The parameter is:
       *  \param c  the pointer to class
       *
       *  \throw oks::exception is thrown in case of problems
       */

    static void destroy(OksClass * c);


      /**
       *  \brief The equality operator.
       *
       *  The operator returns true, if two classes have equal names.
       */

    bool operator==(const OksClass &) const;

      /**
       *  \brief The not equal operator.
       *
       *  The operator returns false, if two classes have
       *  equal attributes, relationships and methods.
       */

    bool operator!=(const OksClass &) const;
    
      /**
       *  \brief Stream operator.
       *
       *  Send to stream complete description of OKS class:
       *  - print class name
       *  - print details of attributes, relationships and methods
       *  - print details of objects
       */

    friend  std::ostream& operator<<(std::ostream&, const OksClass&);

    friend  std::ostream& operator<<(std::ostream&, const OksObject&);


      /** Compare all properties of classes excluding methods **/

    bool compare_without_methods(const OksClass & c) const noexcept ;


      // methods to access class external information

  public:

      /** Get OKS kernel which registering given class. */

    OksKernel * get_kernel() const noexcept {return p_kernel;}


      /** Get file of object. */

    OksFile * get_file() const noexcept {return p_file;}


      /**
       *  \brief Move class to different file.
       *
       *  Move class to an existing file. Both, present file where object is stored
       *  and new destination file have to be writable by user.
       *
       *  The parameters are:
       *  \param file           destination file
       *  \param update_owner   mark original file as updated
       *
       *  \throw oks::exception is thrown in case of problems
       */

    void set_file(OksFile * f, bool update_owner = true);


      // methods to access class properties

  public:

      /** Return name of class. */

    const std::string& get_name() const noexcept {return p_name;}


      /** Return description of class. */

    const std::string& get_description() const noexcept {return p_description;}


      /**
       *  \brief Set class description.
       *
       *  \param description    new description of attribute (max 2000 bytes, see #s_max_description_length variable)
       *
       *  \throw oks::exception is thrown in case of problems
       */

    void set_description(const std::string& description);


      /** Return true if class is abstract (i.e. has no objects). */

    bool get_is_abstract() const noexcept {return p_abstract;}


      /**
       *  \brief Set class abstract property.
       *
       *  If the class is abstract, it may not have objects.
       *  However the derived classes may have,
       *  if they are not abstract themselves.
       *
       *  \param abstract    pass true to make class abstract
       *
       *  \throw oks::exception is thrown in case of problems
       */

    void set_is_abstract(bool abstract);


      // methods to access class hierarchy description

  public:

      /** Return list of all base classes of given class. */

    const FList * all_super_classes() const noexcept {return p_all_super_classes;}


      /** Return list of direct base classes of given class. */

    const std::list<std::string *> * direct_super_classes() const noexcept {return p_super_classes;}


      /** Search class by name among all base classes of given class. */

    OksClass * find_super_class(const std::string&) const noexcept;


      /** Return true, if class has direct base class with given name. */

    bool has_direct_super_class(const std::string&) const noexcept;


      /**
       *  Add direct base class with given name.
       *  \param name name of superclass
       *  \throw oks::exception is thrown in case of problems
       */

    void add_super_class(const std::string& name);


      /** Kernel method to add direct base class with given name. */

    void k_add_super_class(const std::string& name);


      /**
       * Remove direct base class with given name.
       *  \param name name of superclass
       *  \throw oks::exception is thrown in case of problems
       */

    void remove_super_class(const std::string& name);


       /**
       *  \brief Swap order of two superclasses.
       *
       *  Swap position of two superclasses.
       *  The superclasses must be direct superclasses of given class.
       *
       *  \param c1    name of first superclass
       *  \param c2    name of second superclass
       *
       *  \throw oks::exception is thrown in case of problems
       */

    void swap_super_classes(const std::string& c1, const std::string& c2);


      /** Return list of all derived classes. */

    const FList * all_sub_classes() const noexcept {return p_all_sub_classes;}


      // methods to access class attributes description

  public:

      /** Get list of all attributes (including defined in base classes) */

    const std::list<OksAttribute *> * all_attributes() const noexcept {return p_all_attributes;}


      /** Get list of direct attributes */

    const std::list<OksAttribute *> * direct_attributes() const noexcept {return p_attributes;}


      /**
       *  \brief Find attribute (search in this and base classes).
       *
       *  Get attribute by name.
       *
       *  \param name    attribute name
       *  \return        found attribute or 0, if this and base classes have no attribute with such name
       */

    OksAttribute * find_attribute(const std::string& name) const noexcept;


      /**
       *  \brief Find direct attribute.
       *
       *  Get attribute by name.
       *
       *  \param name    attribute name
       *  \return        found attribute or 0, if this class has no attribute with such name
       */

    OksAttribute * find_direct_attribute(const std::string& name) const noexcept;


      /**
       *  \brief Add attribute.
       *
       *  Add new attribute to class.
       *  The class must not have attributes with such name.
       *
       *  \param a    new attribute
       *
       *  \throw oks::exception is thrown in case of problems
       */

    void add(OksAttribute * a);


      /**
       *  \brief Kernel method to add attribute.
       *  \param a    new attribute
       */

    void k_add(OksAttribute * a);


      /**
       *  \brief Remove attribute.
       *
       *  Remove attribute from class.
       *  The attribute mush be direct attribute of the class.
       *
       *  \param a    attribute to be removed
       *
       *  \throw oks::exception is thrown in case of problems
       */

    void remove(const OksAttribute * a);


      /**
       *  \brief Swap order of two attributes.
       *
       *  Swap position of two attributes.
       *  The attributes must be direct attributes of given class.
       *
       *  \param a1    first attribute
       *  \param a2    second attribute
       *
       *  \throw oks::exception is thrown in case of problems
       */

    void swap(const OksAttribute * a1, const OksAttribute * a2);


      /** Get number of direct attributes */

    size_t number_of_direct_attributes() const noexcept;


      /** Get number of all attributes (including attributes from this and base classes) */

    size_t number_of_all_attributes() const noexcept;
    
    
      /**
       *  \brief Get class owning given attribute.
       *
       *  Return class where attribute is defined.
       *
       *  \param a    the attribute
       *  \return     pointer to class where attribute is defined or 0, if the attribute does not belong to a class
       */

    OksClass * source_class(const OksAttribute * a) const noexcept;


      // methods to access class relationships description


      /** Get list of all relationships (including defined in base classes) */

    const std::list<OksRelationship *> * all_relationships() const noexcept {return p_all_relationships;}


      /** Get list of direct relationships */

    const std::list<OksRelationship *> * direct_relationships() const noexcept {return p_relationships;}


      /**
       *  \brief Find relationship (search in this and base classes).
       *
       *  Get relationship by name.
       *
       *  \param name   relationship name
       *  \return       found relationship or 0, if this and base classes have no relationship with such name
       */

    OksRelationship * find_relationship(const std::string& name) const noexcept;


      /**
       *  \brief Find direct relationship.
       *
       *  Get relationship by name.
       *
       *  \param name    relationship name
       *  \return        found relationship or 0, if this class has no relationship with such name
       */

    OksRelationship * find_direct_relationship(const std::string& name) const noexcept;


      /**
       *  \brief Add relationship.
       *
       *  Add new relationship to class.
       *  The class must not have relationships with such name.
       *
       *  \param r    new relationship
       *
       *  \throw oks::exception is thrown in case of problems
       */

    void add(OksRelationship * r);


      /**
       *  \brief Kernel method to add relationship.
       *  \param r    new relationship
       */

    void k_add(OksRelationship * r);


      /**
       *  \brief Remove relationship.
       *
       *  Remove relationship from class.
       *  The relationship must be direct method of the class.
       *
       *  \param r            relationship to be removed
       *  \param call_delete  if true, delete removed relationship
       *
       *  \throw oks::exception is thrown in case of problems
       */

    void remove(const OksRelationship * r, bool call_delete = true);


      /**
       *  \brief Swap order of two relationships.
       *
       *  Swap position of two relationships.
       *  The relationships must be direct relationships of given class.
       *
       *  \param r1    first relationship
       *  \param r2    second relationship
       *
       *  \throw oks::exception is thrown in case of problems
       */

    void swap(const OksRelationship * r1, const OksRelationship * r2);


      /** Get number of direct relationships */

    size_t number_of_direct_relationships() const noexcept;


      /** Get number of all relationships (including relationships from this and base classes) */

    size_t number_of_all_relationships() const noexcept;


      /**
       *  \brief Get class owning given relationship.
       *
       *  Return class where relationship is defined.
       *
       *  \param r    the relationship
       *  \return     pointer to class where relationship is defined or 0, if the relationship does not belong to a class
       */

    OksClass * source_class(const OksRelationship * r) const noexcept;


      // methods to access class methods description



      /** Get list of all methods (including defined in base classes) */

    const std::list<OksMethod *> * all_methods() const noexcept {return p_all_methods;}


      /** Get list of direct methods */

    const std::list<OksMethod *> * direct_methods() const noexcept {return p_methods;}


      /**
       *  \brief Find method (search in this and base classes).
       *
       *  Get method by name.
       *
       *  \param name    method name
       *  \return        found method or 0, if this and base classes have no method with such name
       */

    OksMethod * find_method(const std::string& name) const noexcept;


      /**
       *  \brief Find direct method.
       *
       *  Get method by name.
       *
       *  \param name    method name
       *  \return        found method or 0, if this class has no method with such name
       */

    OksMethod * find_direct_method(const std::string& name) const noexcept;


      /**
       *  \brief Add method.
       *
       *  Add new method to class.
       *  The class must not have methods with such name.
       *
       *  \param m    new method
       *
       *  \throw oks::exception is thrown in case of problems
       */

    void add(OksMethod * m);


      /**
       *  \brief Remove method.
       *
       *  Remove method from class.
       *  The method mush be direct method of the class.
       *
       *  \param m   method to be removed
       *
       *  \throw oks::exception is thrown in case of problems
       */

    void remove(const OksMethod * m);


      /**
       *  \brief Swap order of two methods.
       *
       *  Swap position of two methods.
       *  The methods must be direct methods of given class.
       *
       *  \param m1    first method
       *  \param m2    second method
       *
       *  \throw oks::exception is thrown in case of problems
       */

    void swap(const OksMethod * m1, const OksMethod * m2);


      /** Get number of direct methods */

    size_t number_of_direct_methods() const noexcept;


      /** Get number of all methods (including methods from this and base classes) */

    size_t number_of_all_methods() const noexcept;


      /**
       *  \brief Get class owning given method.
       *
       *  Return class where method is defined.
       *
       *  \param m    the method
       *  \return     pointer to class where method is defined or 0, if the method does not belong to a class
       */

    OksClass * source_class(const OksMethod * m) const noexcept;


      // methods to access class instances


      /** Get number of objects of given class */

    size_t number_of_objects() const noexcept;


      /** Get objects of given class (the pointer can be 0) */

    const OksObject::Map * objects() const noexcept {return p_objects;}


      /**
       *  \brief Get all objects of the class (including derived).
       *
       *  Return all objects of given and derived classes.
       *  The list to be deleted by user.
       *
       *  \return     pointer to list of all objects, or 0 if there are no objects
       */

    std::list<OksObject *> * create_list_of_all_objects() const noexcept;


      /**
       *  \brief Get object by ID.
       *
       *  Return pointer to object of given class by identity.
       *
       *  \return     pointer to object, or 0 if there is no such object
       */

    OksObject * get_object(const std::string& id) const noexcept;


      /** See get_object(const std::string&) */

    OksObject * get_object(const std::string* id) const noexcept {return get_object(*id);}


      /**
       *  \brief Execute query.
       *
       *  Return list of objects satisfying given query.
       *  The list to be deleted by user.
       *
       *  \param query    OKS query
       *  \return         pointer to list of all objects, or 0 if there are no objects
       *
       *  \throw In case of problems the oks::exception is thrown.
       */

    OksObject::List * execute_query(OksQuery * query) const;


      /**
       *  \brief Get OKS data information for attribute or relationship.
       *
       *  Pass name of attribute or relationship defined for given class.
       *  If it is not defined, the application will crash (no extra checks by efficiency reasons).
       *
       *  \param s    name of existing OKS attribute or relationship
       *  \return     OKS data information pointer
       */

    OksDataInfo * data_info(const std::string &s) const noexcept {return p_data_info->find(s)->second;}


      /**
       *  \brief Get OKS data information for attribute or relationship.
       *
       *  Pass name of attribute or relationship defined for given class.
       *  Safe version of data_info()
       *
       *  \param s    name of existing OKS attribute or relationship
       *  \return     OKS data information pointer or nullptr if no such attribute or relationship
       */

    OksDataInfo *
    get_data_info(const std::string &s) const noexcept
    {
      auto it = p_data_info->find(s);
      return (it != p_data_info->end() ? it->second : nullptr);
    }


      /**
       *  \brief Information about class property changes.
       *
       *  It is used in the ChangeNotifyFN callback, that
       *  can be used by user to subscribe on schema changes.
       *
       */

    enum ChangeType {

      ChangeSuperClassesList,
      ChangeSubClassesList,
      ChangeDescription,
      ChangeIsAbstaract,

      ChangeAttributesList,
      ChangeAttributeType,
      ChangeAttributeRange,
      ChangeAttributeFormat,
      ChangeAttributeMultiValueCardinality, 
      ChangeAttributeInitValue,
      ChangeAttributeDescription,
      ChangeAttributeIsNoNull,

      ChangeRelationshipsList,
      ChangeRelationshipClassType,
      ChangeRelationshipDescription,
      ChangeRelationshipLowCC,
      ChangeRelationshipHighCC,
      ChangeRelationshipComposite,
      ChangeRelationshipExclusive,
      ChangeRelationshipDependent,

      ChangeMethodsList,
      ChangeMethodDescription,
      ChangeMethodImplementation

    };


      /** Callback function invoked on OKS class creation or deletion */

    typedef void (*NotifyFN)(OksClass *);
    
    
      /** Callback function invoked on OKS class modification */
    
    typedef void (*ChangeNotifyFN)(OksClass *, ChangeType, const void *);


  private:

    std::string				p_name;
    std::string				p_description;
    std::list<std::string *> *		p_super_classes;
    std::list<OksAttribute *> *		p_attributes;
    std::list<OksRelationship *> *	p_relationships;
    std::list<OksMethod *> *		p_methods;
    bool				p_abstract;
    bool				p_transient;
    bool				p_to_be_deleted;

    FList *                             p_all_super_classes;
    FList *                             p_all_sub_classes;
    std::list<OksAttribute *> *		p_all_attributes;
    std::list<OksRelationship *> *	p_all_relationships;
    std::list<OksMethod *> *		p_all_methods;

    std::vector<OksClass *> *           p_inheritance_hierarchy;
    unsigned int                        p_id;

    OksFile *				p_file;
    OksKernel *				p_kernel;

    size_t				p_instance_size;

    OksDataInfo::Map *			p_data_info;
    OksObject::Map *			p_objects;
    OksIndex::Map *			p_indices;

    mutable std::shared_mutex           p_mutex;
    mutable std::mutex                  p_unique_id_mutex;



    static NotifyFN			create_notify_fn;
    static ChangeNotifyFN		change_notify_fn;
    static NotifyFN			delete_notify_fn;



      // to be used by OksKernel only

    ~OksClass();
    OksClass				(OksXmlInputStream &, OksKernel *);

    template<class T> static void       destroy_map(T map);
    template<class T> static void       destroy_list(T list);

      // transient class for OksKernel::find_class()

    inline OksClass (const char * name, size_t len);

    void save(OksXmlOutputStream &) const;

    void add(OksObject *);
    void remove(OksObject *);
    void registrate_class(bool skip_registered);
    void registrate_class_change(ChangeType, const void *, bool = true);
    void registrate_attribute_change(OksAttribute *);
    void registrate_relationship_change(OksRelationship *);
    void registrate_instances();

    void add_super_classes(FList *) const;
    void create_super_classes();
    void create_sub_classes();
    void create_attributes();
    void create_relationships();
    void create_methods();

    void lock_file(const char *);

    bool check_relationships(std::ostringstream & out, bool print_file_name) const noexcept;


      // valid xml tags and attributes

    static const char class_xml_tag[];
    static const char name_xml_attr[];
    static const char description_xml_attr[];
    static const char is_abstract_xml_attr[];
    static const char superclass_xml_attr[];

};


	//
	// INLINE METHODS
	//

inline bool
OksClass::operator==(const OksClass &c) const
{
  return ((this == &c || p_name == c.p_name) ? true : false);
}


inline size_t
OksClass::number_of_direct_attributes() const noexcept
{
  return (p_attributes ? p_attributes->size() : 0);
}


inline size_t
OksClass::number_of_all_attributes() const noexcept
{
  return (p_all_attributes ? p_all_attributes->size() : 0);
}


inline size_t
OksClass::number_of_direct_relationships() const noexcept
{
  return (p_relationships ? p_relationships->size() : 0);
}


inline size_t
OksClass::number_of_all_relationships() const noexcept
{
  return (p_all_relationships ? p_all_relationships->size() : 0);
}


inline size_t
OksClass::number_of_direct_methods() const noexcept
{
  return (p_methods ? p_methods->size() : 0);
}


inline size_t
OksClass::number_of_all_methods() const noexcept
{
  return (p_all_methods ? p_all_methods->size() : 0);
}

inline
OksClass::OksClass (const char * name, size_t len) :
  p_name		  (name, len),
  p_super_classes	  (0),
  p_attributes		  (0),
  p_relationships	  (0),
  p_methods		  (0),
  p_abstract		  (false),
  p_transient		  (true),
  p_to_be_deleted         (false),
  p_all_super_classes	  (0),
  p_all_sub_classes	  (0),
  p_all_attributes	  (0),
  p_all_relationships	  (0),
  p_all_methods		  (0),
  p_inheritance_hierarchy (0),
  p_kernel		  (0),
  p_instance_size	  (0),
  p_data_info		  (0),
  p_objects		  (0),
  p_indices		  (0)
{ ; }

#endif
