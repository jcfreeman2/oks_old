  /**
   *  \file ConfigObjectImpl.h This file contains abstract ConfigObjectImpl class,
   *  that is used to implement config objects.
   *  \author Igor Soloviev
   *  \brief abstract ConfigObject implementation
   */

#ifndef CONFIG_CONFIGOBJECTIMPL_H_
#define CONFIG_CONFIGOBJECTIMPL_H_

#include <string>
#include <vector>
#include <iostream>
#include <stdint.h>

#include "config/Errors.hpp"

class ConfigObject;
class Configuration;
class ConfigurationImpl;
class DalObject;

namespace daq
{
  namespace config
  {

    /** Possible states of configuration objects. */
    enum ObjectState
    {
      Valid,           /*!< the object is valid */
      Deleted,         /*!< the object was deleted and may not be accessed */
      Unknown          /*!< the state of object i unknown (e.g. after abort) */
    };
  }
}

  /**
   *  \brief Implements database objects.
   *
   *  The class defines several pure virtual methods used to get, set
   *  and convert values of object's attributes and relationships.
   *
   *  To create implementation for an arbitrary DBMS technology
   *  it is necessary to derive new class from ConfigObjectImpl
   *  and to implement all virtual methods.
   *
   *  The methods may throw daq::config::Generic exception in case of an error
   *  unless \b noexcept is explicitly used in their specification.
   */

class ConfigObjectImpl {

  friend class ConfigObject;
  friend class Configuration;
  friend class ConfigurationImpl;
  friend class DalObject;


  public:

      /// The constructor stores configuration implementation pointer
    ConfigObjectImpl(ConfigurationImpl * impl, const std::string& id, daq::config::ObjectState state = daq::config::Valid) noexcept;

      /// The virtual destructor
    virtual ~ConfigObjectImpl() noexcept;


  private:

    ConfigObjectImpl() = delete;  // do not allow default constructor


  public:

      /// Returns default implementation
    static ConfigObjectImpl * default_impl() noexcept;


  public:

      /// Method to get database object's unique ID
    const std::string&
    UID() const noexcept
    {
      std::lock_guard<std::mutex> scoped_lock(m_mutex); // be sure no one renames the object
      return m_id;
    }

      /// Virtual method to get object's class name
    const std::string&
    class_name() const noexcept
    {
      return *m_class_name;
    }

      /// Virtual method to get object's database file name
    virtual const std::string contained_in() const = 0;

  public:

      /// Virtual method to read boolean attribute value
    virtual void get(const std::string& attribute, bool& value) = 0;

      /// Virtual method to read unsigned char attribute value
    virtual void get(const std::string& attribute, uint8_t& value) = 0;

      /// Virtual method to read signed char attribute value
    virtual void get(const std::string& attribute, int8_t& value) = 0;

      /// Virtual method to read unsigned short attribute value
    virtual void get(const std::string& attribute, uint16_t& value) = 0;

      /// Virtual method to read signed short attribute value
    virtual void get(const std::string& attribute, int16_t& value) = 0;

      /// Virtual method to read unsigned long attribute value
    virtual void get(const std::string& attribute, uint32_t& value) = 0;

      /// Virtual method to read signed long attribute value
    virtual void get(const std::string& attribute, int32_t& value) = 0;

      /// Virtual method to read unsigned 64 bits integer attribute value
    virtual void get(const std::string& attribute, uint64_t& value) = 0;

      /// Virtual method to read signed 64 bits integer attribute value
    virtual void get(const std::string& attribute, int64_t& value) = 0;

      /// Virtual method to read float attribute value
    virtual void get(const std::string& attribute, float& value) = 0;

      /// Virtual method to read double attribute value
    virtual void get(const std::string& attribute, double& value) = 0;

      /// Virtual method to read string attribute value
    virtual void get(const std::string& attribute, std::string& value) = 0;

      /// Virtual method to read relationship single-value
    virtual void get(const std::string& association, ConfigObject& value) = 0;


  public:

      /// Virtual method to read vector-of-booleans attribute value
    virtual void get(const std::string& attribute, std::vector<bool>& value) = 0;

      /// Virtual method to read vector-of-unsigned-chars attribute value
    virtual void get(const std::string& attribute, std::vector<uint8_t>& value) = 0;

      /// Virtual method to read vector-of-signed-chars attribute value
    virtual void get(const std::string& attribute, std::vector<int8_t>& value) = 0;

      /// Virtual method to read vector-of-unsigned-shorts attribute value
    virtual void get(const std::string& attribute, std::vector<uint16_t>& value) = 0;

      /// Virtual method to read vector-of-signed-shorts attribute value
    virtual void get(const std::string& attribute, std::vector<int16_t>& value) = 0;

      /// Virtual method to read vector-of-unsigned-longs attribute value
    virtual void get(const std::string& attribute, std::vector<uint32_t>& value) = 0;

      /// Virtual method to read vector-of-signed-longs attribute value
    virtual void get(const std::string& attribute, std::vector<int32_t>& value) = 0;

      /// Virtual method to read vector-of-unsigned-64-bits-integers attribute value
    virtual void get(const std::string& attribute, std::vector<uint64_t>& value) = 0;

      /// Virtual method to read vector-of-signed-64-bits-integers attribute value
    virtual void get(const std::string& attribute, std::vector<int64_t>& value) = 0;

      /// Virtual method to read vector-of-floats attribute value
    virtual void get(const std::string& attribute, std::vector<float>& value) = 0;

      /// Virtual method to read vector-of-doubles attribute value
    virtual void get(const std::string& attribute, std::vector<double>& value) = 0;

      /// Virtual method to read vector-of-strings attribute value
    virtual void get(const std::string& attribute, std::vector<std::string>& value) = 0;

      /// Virtual method to read vector-of-config-objects relationship value
    virtual void get(const std::string& association, std::vector<ConfigObject>& value) = 0;


  public:

      /// Virtual method to read any relationship value without throwing an exception if there is no such relationship (return false)
    virtual bool rel(const std::string& name, std::vector<ConfigObject>& value) = 0;

      /// Virtual method to read vector-of-config-object referencing this object
    virtual void referenced_by(std::vector<ConfigObject>& value, const std::string& association, bool check_composite_only, unsigned long rlevel, const std::vector<std::string> * rclasses) const = 0;


  public:

      /// Virtual method to set boolean attribute value
    virtual void set(const std::string& attribute, bool value) = 0;

      /// Virtual method to set unsigned char attribute value
    virtual void set(const std::string& attribute, uint8_t value) = 0;

      /// Virtual method to set signed char attribute value
    virtual void set(const std::string& attribute, int8_t value) = 0;
 
      /// Virtual method to set unsigned short attribute value
    virtual void set(const std::string& attribute, uint16_t value) = 0;

      /// Virtual method to set signed short attribute value
    virtual void set(const std::string& attribute, int16_t value) = 0;

      /// Virtual method to set unsigned long attribute value
    virtual void set(const std::string& attribute, uint32_t value) = 0;

      /// Virtual method to set signed long attribute value
    virtual void set(const std::string& attribute, int32_t value) = 0;

      /// Virtual method to set unsigned 64 bits integer attribute value
    virtual void set(const std::string& attribute, uint64_t value) = 0;

      /// Virtual method to set signed 64 bits integer attribute value
    virtual void set(const std::string& attribute, int64_t value) = 0;

      /// Virtual method to set float attribute value
    virtual void set(const std::string& attribute, float value) = 0;

      /// Virtual method to set double attribute value
    virtual void set(const std::string& attribute, double value) = 0;

      /// Virtual method to set string attribute value
    virtual void set(const std::string& attribute, const std::string& value) = 0;

      /// Virtual method to set enumeration attribute value
    virtual void set_enum(const std::string& attribute, const std::string& value) = 0;

      /// Virtual method to set enumeration attribute value
    virtual void set_class(const std::string& attribute, const std::string& value) = 0;

      /// Virtual method to set date attribute value
    virtual void set_date(const std::string& attribute, const std::string& value) = 0;

      /// Virtual method to set time attribute value
    virtual void set_time(const std::string& attribute, const std::string& value) = 0;


  public:

      /// Virtual method to read vector-of-booleans attribute value
    virtual void set(const std::string& attribute, const std::vector<bool>& value) = 0;

      /// Virtual method to read vector-of-unsigned-chars attribute value
    virtual void set(const std::string& attribute, const std::vector<uint8_t>& value) = 0;

      /// Virtual method to read vector-of-signed-chars attribute value
    virtual void set(const std::string& attribute, const std::vector<int8_t>& value) = 0;

      /// Virtual method to read vector-of-unsigned-shorts attribute value
    virtual void set(const std::string& attribute, const std::vector<uint16_t>& value) = 0;

      /// Virtual method to read vector-of-signed-shorts attribute value
    virtual void set(const std::string& attribute, const std::vector<int16_t>& value) = 0;

      /// Virtual method to read vector-of-unsigned-longs attribute value
    virtual void set(const std::string& attribute, const std::vector<uint32_t>& value) = 0;

      /// Virtual method to read vector-of-signed-longs attribute value
    virtual void set(const std::string& attribute, const std::vector<int32_t>& value) = 0;

      /// Virtual method to read vector-of-unsigned-64-bits-integers attribute value
    virtual void set(const std::string& attribute, const std::vector<uint64_t>& value) = 0;

      /// Virtual method to read vector-of-signed-64-bits-integers attribute value
    virtual void set(const std::string& attribute, const std::vector<int64_t>& value) = 0;

      /// Virtual method to read vector-of-floats attribute value
    virtual void set(const std::string& attribute, const std::vector<float>& value) = 0;

      /// Virtual method to read vector-of-doubles attribute value
    virtual void set(const std::string& attribute, const std::vector<double>& value) = 0;

      /// Virtual method to read vector-of-strings attribute value
    virtual void set(const std::string& attribute, const std::vector<std::string>& value) = 0;

      /// Virtual method to read vector-of-enumerations attribute value
    virtual void set_enum(const std::string& attribute, const std::vector<std::string>& value) = 0;

      /// Virtual method to read vector-of-enumerations attribute value
    virtual void set_class(const std::string& attribute, const std::vector<std::string>& value) = 0;

      /// Virtual method to read vector-of-dates attribute value
    virtual void set_date(const std::string& attribute, const std::vector<std::string>& value) = 0;

      /// Virtual method to read vector-of-times attribute value
    virtual void set_time(const std::string& attribute, const std::vector<std::string>& value) = 0;

      /// Virtual method to read config-object relationship  value
    virtual void set(const std::string& association, const ConfigObject * value, bool skip_non_null_check) = 0;

      /// Virtual method to read vector-of-config-objects relationship value
    virtual void set(const std::string& association, const std::vector<const ConfigObject*>& value, bool skip_non_null_check) = 0;

      /// Virtual method to move object to a file
    virtual void move(const std::string& at) = 0;

      /// Virtual method to change object ID
    virtual void rename(const std::string& new_id) = 0;

      /// Virtual method to clean resources used by the implementation object
    virtual void clear() noexcept {;} // by default nothing to do

      /// Virtual method to reset the implementation object from unknown state
    virtual void reset() = 0;

      /// Check object and return true if the object has been deleted.
    bool
    is_deleted() const
    {
      if (m_state == daq::config::Unknown)
        {
          const_cast<ConfigObjectImpl *>(this)->reset();
        }

      return (m_state == daq::config::Deleted);
    }


  protected:

    ConfigurationImpl * m_impl;               /*!< Pointer to configuration implementation object */
    daq::config::ObjectState m_state;         /*!< State of the object */
    std::string m_id;                         /*!< Object ID */
    const std::string * m_class_name;         /*!< Name of object's class */
    mutable std::mutex m_mutex;               /*!< Mutex protecting concurrent access to this object */


  protected:

    /**
     * Check state of object and throw exception if it has been deleted
     * \throw daq::config::DeletedObject if the object has been deleted
     */
    void
    throw_if_deleted() const
    {
      if (is_deleted())
        {
          throw daq::config::DeletedObject(ERS_HERE, m_class_name->c_str(), m_id.c_str());
        }
    }


  private:

      // convert attribute values, if there is a configuration converter

    void convert(bool& value,            const ConfigObject& obj, const std::string& attr_name) noexcept;
    void convert(uint8_t& value,         const ConfigObject& obj, const std::string& attr_name) noexcept;
    void convert(int8_t& value,          const ConfigObject& obj, const std::string& attr_name) noexcept;
    void convert(uint16_t& value,        const ConfigObject& obj, const std::string& attr_name) noexcept;
    void convert(int16_t& value,         const ConfigObject& obj, const std::string& attr_name) noexcept;
    void convert(uint32_t& value,        const ConfigObject& obj, const std::string& attr_name) noexcept;
    void convert(int32_t& value,         const ConfigObject& obj, const std::string& attr_name) noexcept;
    void convert(uint64_t& value,        const ConfigObject& obj, const std::string& attr_name) noexcept;
    void convert(int64_t& value,         const ConfigObject& obj, const std::string& attr_name) noexcept;
    void convert(float& value,           const ConfigObject& obj, const std::string& attr_name) noexcept;
    void convert(double& value,          const ConfigObject& obj, const std::string& attr_name) noexcept;
    void convert(std::string& value,     const ConfigObject& obj, const std::string& attr_name) noexcept;
    void convert(ConfigObject&,          const ConfigObject&,     const std::string&          ) noexcept {;}

    void convert(std::vector<bool>& value,            const ConfigObject& obj, const std::string& attr_name) noexcept;
    void convert(std::vector<uint8_t>& value,         const ConfigObject& obj, const std::string& attr_name) noexcept;
    void convert(std::vector<int8_t>& value,          const ConfigObject& obj, const std::string& attr_name) noexcept;
    void convert(std::vector<uint16_t>& value,        const ConfigObject& obj, const std::string& attr_name) noexcept;
    void convert(std::vector<int16_t>& value,         const ConfigObject& obj, const std::string& attr_name) noexcept;
    void convert(std::vector<uint32_t>& value,        const ConfigObject& obj, const std::string& attr_name) noexcept;
    void convert(std::vector<int32_t>& value,         const ConfigObject& obj, const std::string& attr_name) noexcept;
    void convert(std::vector<uint64_t>& value,        const ConfigObject& obj, const std::string& attr_name) noexcept;
    void convert(std::vector<int64_t>& value,         const ConfigObject& obj, const std::string& attr_name) noexcept;
    void convert(std::vector<float>& value,           const ConfigObject& obj, const std::string& attr_name) noexcept;
    void convert(std::vector<double>& value,          const ConfigObject& obj, const std::string& attr_name) noexcept;
    void convert(std::vector<std::string>& value,     const ConfigObject& obj, const std::string& attr_name) noexcept;
    void convert(std::vector<ConfigObject>&,          const ConfigObject&,     const std::string&          ) noexcept {;}

};


#endif // CONFIG_CONFIGOBJECTIMPL_H_
