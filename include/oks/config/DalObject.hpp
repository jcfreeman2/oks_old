/**
 *  \file DalObject.h This file contains DalObject class,
 *  that is the base class for generated DAL classes.
 *  \author Igor Soloviev
 *  \brief base class for generated template objects
 */

#ifndef CONFIG_DAL_OBJECT_H_
#define CONFIG_DAL_OBJECT_H_

#include <iostream>
#include <memory>
#include <mutex>
#include <string>

#include <config/ConfigObject.hpp>
#include <config/Configuration.hpp>
#include <config/Change.hpp>
#include <config/DalFactoryFunctions.hpp>
#include <config/Errors.hpp>


/**
 * \brief The base class for any generated DAL object.
 *
 *  The class provides common methods for any DAL object such as:
 *  - class_name() access object's database class name
 *  - UID() access object's unique identity
 *  - full_name() full object's name returned as string
 *  - config_object() provide access to the representation of object at config layer
 *
 *  Several methods below are used to set state of the object and normally should only be used internally
 *  by generated DAL and algorithms:
 *  - unread() mark object as non-read to re-read values of attributes and relationships from config object
 *  - remove() mark object as deleted
 *  - was_removed() return state of object's removal
 *  - set() assign config object
 */

class DalObject
{

  friend class Configuration;
  friend DalFactoryFunctions;
  friend std::ostream&
  operator<<(std::ostream& s, const DalObject * obj);

protected:

  /**
   *  The constructor of DAL object.
   */

  DalObject(Configuration& db, const ::ConfigObject& o) noexcept :
    p_was_read(false), p_db(db), p_obj(o), p_UID(p_obj.UID())
    {
      increment_created();
    }

  virtual ~DalObject()
    {
      ;
    }

  /**
   *  The method resets state of object.
   *  When accessed next time, it will be completely re-read from implementation.
   */

  void clear() noexcept
    {
      p_obj._clear();
    }

  /**
   *  The method checks state of object and throws exception if it was deleted.
   *
   *  \throw daq::config::DeletedObject if object was deleted
   */

  void check() const
    {
      p_obj.m_impl->throw_if_deleted();
    }


  /**
   *  The method checks state of object and throws exception if it was deleted.
   *
   *  \throw daq::config::Exception in case of problems
   */

  bool is_deleted() const
    {
      return (p_obj.m_impl->is_deleted());
    }


protected:

  /// Used to protect changes of DAL object
  mutable std::mutex m_mutex;

  /// is true, if the object was read
  bool p_was_read;

  /// Configuration object
  Configuration& p_db;

  /// Config object used by given template object
  ::ConfigObject p_obj;

  /// Is used for template objects (see dqm_config)
  std::string p_UID;

public:

  /**
   *  Returns template object ID.
   */

  const std::string& UID() const noexcept
    {
      return p_UID;
    }

  /**
   *  Returns class name of the template object.
   */

  const std::string& class_name() const noexcept
    {
      return p_obj.class_name();
    }

  /**
   *  Check possibility to cast given object to a new class.
   *  The method returns true, if given object can be casted to target class
   *  (i.e. the target class is one of superclasses of given object)
   */

  bool castable(const std::string& target) const noexcept
    {
      return p_db.try_cast(target, *p_obj.m_impl->m_class_name);
    }

  /**
   *  Same as castable(const std::string& target), but uses pointers returned by the DalFactory (more efficient)
   */

  bool castable(const std::string * target) const noexcept
    {
      return p_db.try_cast(target, p_obj.m_impl->m_class_name);
    }

  /**
   *  \brief Casts object to different class.
   *
   *  Try to cast object to an object of TARGET class. Returns \b nullptr if not successful.
   *  Do not use the normal \b dynamic_cast<T>() for database classes.
   *
   *  \return Return pointer to DAL object of required template or \b nullptr if the cast is not successful.
   */

  template<class TARGET> const TARGET *
  cast() const noexcept
    {
      std::lock_guard<std::mutex> scoped_lock(m_mutex);
      return const_cast<Configuration&>(p_db).cast<TARGET>(this);
    }


  /**
   *  Returns fullname of object in obj-id\@class-name format.
   */

  std::string
  full_name() const noexcept
    {
      std::lock_guard<std::mutex> scoped_lock(m_mutex);
      return (p_UID + '@' + class_name());
    }


  /**
   *  Returns reference on the ConfigObject used by this template object.
   *
   *  \throw daq::config::DeletedObject if object was deleted
   */

  const ::ConfigObject&
  config_object() const
    {
      std::lock_guard<std::mutex> scoped_lock(m_mutex);
      check();
      return p_obj;
    }

  /**
   *  Returns reference on the configuration object.
   */

  ::Configuration& configuration() const noexcept
    {
      return p_db;
    }

  /**
   *  Is used to mark template object as non-read,
   *  i.e. it's attributes will be read from the implementation object during next access of an attribute
   *
   *  \throw daq::config::DeletedObject if object was deleted
   */

  void unread()
    {
      std::lock_guard<std::mutex> scoped_lock(m_mutex);
      check();
      p_was_read = false;
    }


  /**
   *  Sets the ConfigObject to be used by this template object.
   *  \param o the config object
   */

  void set(const ConfigObject& o) noexcept
    {
      p_obj = o;
    }


  /**
   *  Move object to another file.
   *  \param at new file name
   */

  void move(const std::string& at)
    {
      p_obj.move(at);
    }

  /**
   *  Rename object.
   *  \param new_id new ID of object
   */

  void rename(const std::string& new_id)
    {
      p_obj.rename(new_id);
    }

  virtual std::vector<const DalObject *> get(const std::string& name, bool upcast_unregistered = true) const = 0;


  // helper methods used by generated DALs

public:


  /**
   *  Print object details (method generated by genconfig)
   *  \param offset shift output
   *  \param print_header print header describing object (avoid when call this method for a base class from derived)
   *  \param s output stream
   *  \throw config::Exception in case of problems (e.g. OKS schema / generated DAL mismatch)
   */

  virtual void print(unsigned int offset, bool print_header, std::ostream& s) const = 0;

  /// print "(null)"
  static void p_null(std::ostream& s);

  /// print "(deleted object)"
  static void p_rm(std::ostream& s);

  /// print error text
  static void p_error(std::ostream& s, daq::config::Exception& ex);

  /// print object headers
  void p_hdr(std::ostream& s, unsigned int indent, const std::string& cl, const char * nm = nullptr) const;

  /// print object details
  std::ostream& print_object(std::ostream& s) const
    {
      if(DalObject::is_null(this))
        {
          DalObject::p_null(s);
        }
      else if(p_obj.m_impl->m_state != daq::config::Valid)
        {
          DalObject::p_rm(s);
        }
      else
        {
          print(0, true, s);
        }

      return s;
    }

  /// throw object initialisation exception (i.e. \throw daq::config::Generic)
  void throw_init_ex(daq::config::Exception& ex);

  /// throw exception in generated get method (i.e. \throw daq::config::Generic)
  static void throw_get_ex(const std::string& what, const std::string& class_name, const DalObject * obj);

  /// check a pointer on DAL object is null
  static bool
  is_null(const DalObject * ref) noexcept
    {
      return (ref == nullptr);
    }



// methods for configuration profiler

protected:

  /**
   *  Increment counter of created template objects (is used by the configuration profiler)
   */

  void increment_created() noexcept
    {
      ++(p_db.p_number_of_template_object_created);
    }

  /**
   *  Increment counter of read template objects (is used by the configuration profiler)
   */

  void increment_read() noexcept
    {
      ++(p_db.p_number_of_template_object_read);
    }


private:

  // prevent copy constructor and operator=
  DalObject(const DalObject&) = delete;
  DalObject& operator=(const DalObject&) = delete;

  template<typename T>
  static void update(::Configuration& db, const ::ConfigurationChange * change) noexcept
    {
      db.update<T>(change->get_modified_objs(), change->get_removed_objs(), change->get_created_objs());
    }

  template<typename T>
  static void change_id(::CacheBase* x, const std::string& old_id, const std::string& new_id) noexcept
    {
      ::Configuration::_rename_object<T>(x, old_id, new_id);
    }

  template<typename T>
  static void unread(::CacheBase* x) noexcept
    {
      ::Configuration::_unread_objects<T>(x);
    }

  template<typename T>
    static DalObject *
    create_instance(::Configuration& db, ::ConfigObject& obj, const std::string& uid)
    {
      return db._make_instance<T>(obj, uid);
    }

protected:

  /**
   *  Initialize object (method generated by genconfig)
   *  \param init_children if true, initialize referenced objects
   */
  virtual void init(bool init_children) = 0;

  /// Check and initialize object if necessary
  void check_init() const
    {
      if(!p_was_read)
        {
          std::lock_guard<std::mutex> scoped_lock(this->p_db.m_tmpl_mutex);
          const_cast<DalObject*>(this)->init(false);
        }
    }

  /// Helper method for generated set single-value relationship methods
  template<typename T>
    void
    _set_object(const std::string &name, const T *value)
    {
      std::lock_guard<std::mutex> scoped_lock(m_mutex);
      check();
      clear();
      p_obj.set_obj(name, (value ? &value->config_object() : (::ConfigObject*) nullptr));
    }

  /// Helper method for generated set multi-value relationship methods
  template<typename T>
    void
    _set_objects(const std::string &name, const std::vector<const T*> &value)
    {
      std::lock_guard<std::mutex> scoped_lock(m_mutex);
      check();
      clear();
      std::vector<const ConfigObject*> v;
      for (auto &i : value)
        v.push_back(&(i->config_object()));
      p_obj.set_objs(name, v);
    }


  /// Read relationship values as DAL objects using DAL factory
  bool
  get_rel_objects(const std::string& name, bool upcast_unregistered, std::vector<const DalObject*>& objs) const;

  /// Run algorithm and return result as DAL objects using DAL factory
  bool
  get_algo_objects(const std::string& name, std::vector<const DalObject*>& objs) const;

};

/** Operator to print any template's object pointer in 'obj-id\@class-name' format **/

std::ostream&
operator<<(std::ostream&, const DalObject *);

///////////////////////////////////////////////////////////////////////////////////////


template<class T>
  DalFactoryFunctions::DalFactoryFunctions(boost::compute::identity<T>, const std::set<std::string> algorithms) :
      m_update_fn(DalObject::update<T>),
      m_unread_object_fn(DalObject::unread<T>),
      m_rename_object_fn(DalObject::change_id<T>),
      m_creator_fn(DalObject::create_instance<T>),
      m_algorithms(algorithms)
  {
    ;
  }


template<class T>
  const T *
  Configuration::create(const ::DalObject& at, const std::string& id, bool init_object)
  {
    return create<T>(at.config_object().contained_in(), id, init_object);
  }

template<class T>
  Configuration::Cache<T> *
  Configuration::get_cache() noexcept
  {
    CacheBase*& c(m_cache_map[&T::s_class_name]);

    if (c == nullptr)
      c = new Cache<T>();

    return static_cast<Cache<T>*>(c);
  }

template<class TARGET, class SOURCE>
  const TARGET *
  Configuration::cast(const SOURCE *s) noexcept
  {
    if (s)
      {
        std::lock_guard<std::mutex> scoped_lock(m_tmpl_mutex);
        ConfigObjectImpl * obj = s->p_obj.m_impl;

        if (try_cast(&TARGET::s_class_name, obj->m_class_name) == true)
          {
            std::lock_guard<std::mutex> scoped_lock(obj->m_mutex);
            if (obj->m_state == daq::config::Valid)
              return _get<TARGET>(*const_cast<ConfigObject *>(&s->p_obj), s->UID());
          }
      }

    return nullptr;
  }

#endif // CONFIG_CONFIGOBJECT_H_
