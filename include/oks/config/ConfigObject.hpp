  /**
   *  \file ConfigObject.h This file contains ConfigObject class,
   *  that is used to represent config objects.
   *  \author Igor Soloviev
   *  \brief config object class
   */

#ifndef CONFIG_CONFIGOBJECT_H_
#define CONFIG_CONFIGOBJECT_H_

#include <string>
#include <vector>
#include <iostream>

#include <mutex>

#include "config/ConfigObjectImpl.hpp"
#include "config/Errors.hpp"

class Configuration;

  /**
   *  \brief Represents database objects.
   *
   *  The class has default constructor, so user can declare instances
   *  locally or as a part of an array as he likes. However, the only way
   *  to retrieve a valid instance of the class is to call either
   *  Configuration::get(const std::string&, const std::string&, ConfigObject&) or
   *  Configuration::get(const std::string&, std::vector<ConfigObject>&) method.
   *
   *  The class has a copy constructor and an assignment operator, i.e.
   *  instances can be copied, passed by value, etc. All necessary
   *  bookkeeping is done transparent to user.
   *
   *  The class has methods to report object's class and identity:
   *  - class_name()  return actual class name of the object
   *  - UID()         return object's unique identity inside given class
   *
   *  Apart from this there is only one template method get() to retrieve
   *  values of object's attributes and relationships.
   */

class ConfigObject {

  friend class Configuration;
  friend class ConfigurationImpl;
  friend class DalObject;

  public:

     /**
      *  \brief Default constructor.
      *
      *  The object is invalid until it is initialized by the
      *  Configuration::get(const std::string&, const std::string&, ConfigObject&) or
      *  Configuration::get(const std::string&, std::vector<ConfigObject>&)
      *  method.
      */

    ConfigObject() noexcept;


     /**
      *  \brief Copy constructor.
      *
      *  All necessary bookkeeping relevant to implementation
      *  is done transparent to user.
      */

    ConfigObject(const ConfigObject& other) noexcept;


     /**
      *  \brief Construct object from implementation object.
      */

    ConfigObject(ConfigObjectImpl *impl) noexcept;


     /**
      *  \brief Destructor.
      */

    ~ConfigObject() noexcept;


     /**
      *  \brief Assignment operator.
      *
      *  All necessary bookkeeping relevant to implementation
      *  is done transparent to user.
      */

    ConfigObject& operator=(const ConfigObject& other) noexcept;


     /**
      *  \brief Assignment from implementation object.
      */

    ConfigObject& operator=(ConfigObjectImpl *impl) noexcept;


     /**
      *  \brief Compare two objects
      */

    bool operator==(const ConfigObject& other) const noexcept;


    /**
     *  \brief Check if object's implementation points to null.
     *
     *  Returns false, if the object points to a valid implementation object.
     *  The object can point to null implementation object, if it was
     *  initialized via Configuration::get(relationship, object)
     *  and the relationship's value is not set.
     */

    bool
    is_null() const noexcept
    {
      return (m_impl == nullptr);
    }


    /**
     *  \brief Check if object was deleted.
     */

    bool
    is_deleted() const
    {
      return (m_impl->is_deleted());
    }


  private:

    void action_on_object_update(Configuration * db, const std::string& name);



  public:


     /**
      *  \brief Return object identity.
      *
      *  The identity corresponds to the unique identity of object
      *  within it's class. It is taken from database implementation.
      *  For named objects it can be a meaningful string.
      *
      *  The pair UID and class name (see class_name() method)
      *  allows to identify object inside database.
      */

    const std::string& UID() const noexcept { return m_impl->m_id; }


     /**
      *  \brief Return object's class name.
      *
      *  It corresponds to the actual name of the class from the
      *  database implementation.
      *
      *  The pair UID (see UID() method) and class name
      *  allows to identify object inside database.
      */

    const std::string& class_name() const noexcept { return *m_impl->m_class_name; }


     /**
      *  \brief Return full object name.
      *
      *  The method combines object identity and it's class name.
      *  Such full name is unique for any database object.
      */

    const std::string full_name() const noexcept { return UID() + '@' + class_name(); }


     /**
      *  \brief Return the name of the database file this object belongs to.
      *  \throw daq::config::Generic in case of an error
      */

    const std::string contained_in() const
    {
      return m_impl->contained_in();
    }


     /**
      *  \brief Get value of object's attribute or relationship.
      *
      *  Template parameter T can be:
      *  - bool
      *  - int8_t and uint8_t
      *  - int16_t and uint16_t
      *  - int32_t and uint32_t
      *  - int64_t and uint64_t
      *  - float
      *  - double
      *  - std::string
      *  - ConfigObject&
      *  - std::vector<T> of above types
      *
      *  All operations are simply forwarded to the implementation.
      *
      *  \param name   name of attribute or relationship
      *  \param value  type of attribute or relationship
      *
      *  \throw daq::config::Exception in case of an error
      */

    template<class T> void get(const std::string& name, T& value) {
      m_impl->get(name, value);
      m_impl->convert(value, *this, name);
    }


     /**
      *  \brief Get value of object's relationship.
      *
      *  Forward to implementation and convert value.
      *
      *  \param name   name of relationship
      *  \param value  returned value of relationship
      *  \return true if there is such relationship and false otherwise
      *
      *  \throw daq::config::Exception in case of an error
      */

    bool
    rel(const std::string& name, std::vector<ConfigObject>& value)
    {
      if (m_impl->rel(name, value))
        {
          m_impl->convert(value, *this, name);
          return true;
        }

      return false;
    }


     /**
      *  \brief Get objects which have references to given object.
      *
      *  The method returns objects, which have references on given object via explicitly provided relationship name.
      *  If the relationship name is set to "*", then the method takes into account  all relationships of all objects.
      *  The method is efficient only for composite relationships (i.e. when a parent has composite reference on this object).
      *  For generic relationships the method performs full scan of all database objects, so at large scale this method
      *  should not be applied to every object.
      *
      *  \param value                 returned objects
      *  \param relationship_name     name of relationship (if "*", then return objects referencing via ANY relationship)
      *  \param check_composite_only  only returned composite parent objects
      *  \param rlevel                optional references level to optimize performance (defines how many objects referenced by given object have also to be read to the implementation cache)
      *  \param rclasses              optional array of class names to optimize performance (defines which referenced objects have to be read to the implementation cache)
      *
      *  \throw daq::config::Generic in case of an error
      */

    void referenced_by(std::vector<ConfigObject>& value,
		       const std::string& relationship_name = "*",
		       bool check_composite_only = true,
		       unsigned long rlevel = 0,
		       const std::vector<std::string> * rclasses = nullptr ) const {
      m_impl->referenced_by(value, relationship_name, check_composite_only, rlevel, rclasses);
    }

    /// Get pointer to configuration object
    Configuration * get_configuration() const;


     /**
      *  \brief Set relationship single-value.
      *
      *  \param name                  name of relationship
      *  \param o                     pointer to object
      *  \param skip_non_null_check   if true, ignore low cardinality constraint
      *
      *  \throw daq::config::Generic in case of an error
      */

    void set_obj(const std::string& name, const ::ConfigObject * o, bool skip_non_null_check = false) {
      m_impl->set(name, o, skip_non_null_check);
      action_on_object_update(get_configuration(), name);
    }


     /**
      *  \brief Set relationship multi-value.
      *
      *  \param name                  name of relationship
      *  \param o                     vector of pointers on config object
      *  \param skip_non_null_check   if true, ignore low cardinality constraint
      *
      *  \throw daq::config::Generic in case of an error
      */

    void set_objs(const std::string& name, const std::vector<const ::ConfigObject*> & o, bool skip_non_null_check = false) {
      m_impl->set(name, o, skip_non_null_check);
      action_on_object_update(get_configuration(), name);
    }


     /**
      *  \brief Set attribute value.
      *
      *  Template parameter T can be:
      *  - bool
      *  - int8_t and uint8_t (old char)
      *  - int16_t and uint16_t (old short)
      *  - int32_t and uint32_t (old long)
      *  - int64_t and uint64_t
      *  - float
      *  - double
      *
      *  All operations are simply forwarded to the implementation.
      *
      *  \param name   name of attribute
      *  \param value  type of attribute
      *
      *  \throw daq::config::Generic in case of an error
      */

    template<class T> void set_by_val(const std::string& name, T value) {
      m_impl->set(name, value);
      action_on_object_update(get_configuration(), name);
    }


     /**
      *  \brief Set attribute value.
      *
      *  Template parameter T can be:
      *  - std::string
      *  - std::vector<T> of bool, [un]signed char, [un]signed short, [un]signed long, float, double, std::string
      *
      *  All operations are simply forwarded to the implementation.
      *
      *  \param name   name of attribute
      *  \param value  type of attribute
      *
      *  \throw daq::config::Generic in case of an error
      */

    template<class T> void set_by_ref(const std::string& name, T& value) {
      m_impl->set(name, value);
      action_on_object_update(get_configuration(), name);
    }


     /**
      *  \brief Set attribute enumeration value.
      *
      *  Forward operation to the implementation.
      *
      *  \param name   name of attribute
      *  \param value  value of attribute
      *
      *  \throw daq::config::Generic in case of an error
      */

    void set_enum(const std::string& name, const std::string& value) {
      m_impl->set_enum(name, value);
      action_on_object_update(get_configuration(), name);
    }


     /**
      *  \brief Set attribute class value.
      *
      *  Forward operation to the implementation.
      *
      *  \param name   name of attribute
      *  \param value  value of attribute
      *
      *  \throw daq::config::Generic in case of an error
      */

    void set_class(const std::string& name, const std::string& value) {
      m_impl->set_class(name, value);
      action_on_object_update(get_configuration(), name);
    }


     /**
      *  \brief Set attribute date value.
      *
      *  Forward operation to the implementation.
      *
      *  \param name   name of attribute
      *  \param value  value of attribute
      *
      *  \throw daq::config::Generic in case of an error
      */

    void set_date(const std::string& name, const std::string& value) {
      m_impl->set_date(name, value);
      action_on_object_update(get_configuration(), name);
    }


     /**
      *  \brief Set attribute time value.
      *
      *  Forward operation to the implementation.
      *
      *  \param name   name of attribute
      *  \param value  value of attribute
      *
      *  \throw daq::config::Generic in case of an error
      */

    void set_time(const std::string& name, const std::string& value) {
      m_impl->set_time(name, value);
      action_on_object_update(get_configuration(), name);
    }


     /**
      *  \brief Set attribute vector-of-enumerations value.
      *
      *  Forward operation to the implementation.
      *
      *  \param name   name of attribute
      *  \param value  value of attribute
      *
      *  \throw daq::config::Generic in case of an error
      */

    void set_enum(const std::string& name, const std::vector<std::string>& value) {
      m_impl->set_enum(name, value);
      action_on_object_update(get_configuration(), name);
    }


     /**
      *  \brief Set attribute vector-of-class value.
      *
      *  Forward operation to the implementation.
      *
      *  \param name   name of attribute
      *  \param value  value of attribute
      *
      *  \throw daq::config::Generic in case of an error
      */

    void set_class(const std::string& name, const std::vector<std::string>& value) {
      m_impl->set_class(name, value);
      action_on_object_update(get_configuration(), name);
    }


     /**
      *  \brief Set attribute vector-of-dates value.
      *
      *  Forward operation to the implementation.
      *
      *  \param name   name of attribute
      *  \param value  value of attribute
      *
      *  \throw daq::config::Generic in case of an error
      */

    void set_date(const std::string& name, const std::vector<std::string>& value) {
      m_impl->set_date(name, value);
      action_on_object_update(get_configuration(), name);
    }


     /**
      *  \brief Set attribute vector-of-times value.
      *
      *  Forward operation to the implementation.
      *
      *  \param name   name of attribute
      *  \param value  value of attribute
      *
      *  \throw daq::config::Generic in case of an error
      */

    void set_time(const std::string& name, const std::vector<std::string>& value) {
      m_impl->set_time(name, value);
      action_on_object_update(get_configuration(), name);
    }


    /**
     *  \brief Move object to a different database.
     *
     *  Forward operation to the implementation.
     *
     *  \param at name of database file
     *
     *  \throw daq::config::Generic in case of an error
     */

    void move(const std::string& at) {
      m_impl->move(at);
    }


    /**
     *  \brief Rename object.
     *
     *  Forward operation to the implementation.
     *
     *  \param new_id new object ID
     *
     *  \throw daq::config::Generic in case of an error
     */

    void
    rename(const std::string& new_id);


     /**
      *  \brief Print object's pointer in format 'obj-id\@class-name'.
      */

    void print_ptr(std::ostream& s) const noexcept;


     /**
      *  \brief Print details of object's attributes and relationships.
      */

    void print_ref(
      std::ostream& s,                 /*!< the output stream */
      ::Configuration & conf,          /*!< the configuration object (required to read schema description) */
      const std::string& prefix = "",  /*!< optional shift output using prefix */
      bool show_contained_in = false   /*!< optional print out info about object database file */
    ) const noexcept;


     /**
      *  \brief Returns pointer on implementation.
      *
      *  This method exists only by the performance reasons.
      */

    const ConfigObjectImpl * implementation() const noexcept {return m_impl;}


  private:

    void
    _clear() noexcept
    {
      if (m_impl)
        {
          m_impl->clear();
        }
    }


  private:

    ConfigObjectImpl * m_impl;

};


  /** Operator to print pointer on config object in 'obj-id\@class-name' format **/

std::ostream& operator<<(std::ostream&, const ConfigObject *);


  /** Operator to print reference on config object in 'obj-id\@class-name' format **/

std::ostream& operator<<(std::ostream&, const ConfigObject &);

#endif // CONFIG_CONFIGOBJECT_H_
