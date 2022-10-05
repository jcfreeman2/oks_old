  /**

   *  \file Configuration.h This file contains Configuration class,
   *  that is the entry point to access the database information.
   *  \author Igor Soloviev
   *  \brief config entry point
   */

#ifndef CONFIG_CONFIGURATION_H_
#define CONFIG_CONFIGURATION_H_

#include <string.h>

#include <atomic>
#include <typeinfo>
#include <string>
#include <vector>
#include <list>
#include <set>

#include <mutex>

#include <boost/property_tree/ptree.hpp>

#include "ers/ers.hpp"

#include "config/SubscriptionCriteria.hpp"
#include "config/ConfigObject.hpp"
#include "config/ConfigVersion.hpp"
#include "config/Errors.hpp"
#include "config/DalFactory.hpp"

#include "config/map.hpp"
#include "config/set.hpp"

class DalObject;
class ConfigAction;
class ConfigurationImpl;
class ConfigurationChange;

namespace daq {
  namespace config {
    struct class_t;
  }
}

class Configuration;

/**
 * \brief Defines base class for cache of template objects.
 *
 *  The class is used as a base class for any cached template object.
 *  It defines:
 *  - virtual destructor
 *  - DAL factory functions
 */

class CacheBase
{

  friend class Configuration;

protected:

  CacheBase(const DalFactoryFunctions& f) :
      m_functions(f)
  {
    ;
  }

  /** Method for configuration profiling */

  void
  increment_gets(::Configuration& db) noexcept;

  virtual
  ~CacheBase() noexcept
  {
    ;
  }

protected:

  const DalFactoryFunctions& m_functions;

};


  /**
   * \brief Provides abstract interface to database data.
   *
   *  The class provides interfaces to data access and notification on data changes
   *  which are independent from the database implementation.
   *
   *  The class is an entry point to the database information. It provides access
   *  to the database objects by name of the class and optionally (for named objects)
   *  by object identities. Normally, user should use this class to open/close database
   *  and to access objects via template \c get methods invoked with classes generated
   *  by the genconfig utility.
   *
   *  Below there is brief description of main methods. Most of then can only be used
   *  after successful initialization (i.e. database load) of the Configuration object.
   *
   *  Methods throw exceptions in case of an error unless \b noexcept is explicitly
   *  used in their specification. The following exceptions can be thrown:
   *  - daq::config::Generic        is used to report most of the problems (bad DB, wrong parameter, plug-in specific, etc.)
   *  - daq::config::NotFound       the config object accessed by ID is not found, class accessed by name is not found
   *  - daq::config::DeletedObject  accessing template object that has been deleted (via notification or by the user's code)
   *
   *  All above exceptions have common class daq::config::Exception, that can be used to catch all of them.
   *
\code   
try {
    // load database using oks file /tmp/mydb.data.xml
  Configuration db("oksconfig:/tmp/mydb.data.xml");

    // get object "foo@bar"
  ConfigObject obj;
  db.get("bar", "foo", obj);

    // print object to the standard output stream
  obj.print_ref(std::cout, db);
}
  // the catch of daq::config::NotFound exception is optional:
  // it is only used to distinguish NotFound exception from other possible once
catch (daq::config::NotFound & ex) {
  std::cerr << "Object foo@bar is not found: " << ex << std::endl;
}
  // always catch this exception: it can come unexpectedly from DBMS implementation,
  // e.g. because of hardware problems or lack of computer resources
catch (daq::config::Exception & ex) {
  std::cerr << "ERROR: " << ex << std::endl;
}
\endcode   
   *  \par Database Manipulation
   *
   *  To get data a database can be opened, closed and it's state can be checked by the following methods:
   *  - load() open database (by default, the database is opened by the constructor)
   *  - unload() closes database (by default, the database is closed by the destructor)
   *  - loaded() returns true, if a database is correctly loaded
   *
   *  To create or to modify data a database the following methods can be used:
   *  - create(const std::string&, const std::string&, const std::list<std::string>&) create new database
   *  - add_include() add include to an existing database
   *  - remove_include() remove include from an existing database
   *  - commit() save chain of changes
   *  - abort()  cancel all previous changes
   *
   *  \par Objects Access
   *
   *  The access to the objects is provided via two main methods:
   *  - get(const std::string& class, const std::string& id, ConfigObject&, unsigned long, const std::vector<std::string> *) get single object by name of class and id
   *  - get(const std::string& class, std::vector<ConfigObject>&, const std::string& query, unsigned long, const std::vector<std::string> *) get objects of class by name
   *
   *  Below there is example to read all computer objects
\code   
try {
  Configuration db("oksconfig:daq/partitions/part_hlt.data.xml");

  // read all objects of "Computer" class
  std::vector<ConfigObject> hosts;
  db.get("Computer", hosts);

  // print details of the Computer objects
  for (const auto& x : objects)
    x.print_ref(std::cout, db);
}
catch (daq::config::Exception & ex) {
  std::cerr << "ERROR: " << ex << std::endl;
}
\endcode   
   *
   *  For objects of classes generated by genconfig there are analogous template methods which
   *  in addition store pointers to objects in the cache and which to be used by end-user:
   *  - get(const std::string& id, bool, bool, unsigned long, const std::vector<std::string> *) return const pointer to object of given user class
   *  - get(std::vector<const T*>& objects, bool, bool, const std::string& query, unsigned long, const std::vector<std::string> *) fills vector of objects of given user class
   *
   *  Below there is an example for generated \b dal package:
\code
  // include generated files for classes used below
#include "dal/Variable.h"
#include "dal/Segment.h"

try {
  Configuration db("oksconfig:daq/partitions/part_hlt.data.xml");

  // read all variables with Name = "TDAQ_DB_NAME"
  std::vector<const daq::core::Variable*> vars;
  db.get(vars, false, true, "(all (\"Name\" \"TDAQ_ERS_INFO\" =))");

  // print variables
  std::cout << "Got " << vars.size() << " variables with name TDAQ_ERS_INFO:\n";
  for (const auto& x : vars)
    std::cout << "object " << x << " => " << x->get_Value() << std::endl;

  // get segment with id = "online" and print if found
  if (const daq::core::Segment * p = db.get<daq::core::Segment>("online"))
    std::cout << "The segment object is: " << *p << std::endl;
}
catch (daq::config::Exception & ex) {
  std::cerr << "ERROR: " << ex << std::endl;
}
\endcode
   *
   *  \par Notification
   *
   *  To subscribe and unsubscribe on changes it is necessary to create a subscription criteria
   *  (see ::ConfigurationSubscriptionCriteria class for more information).
   *  When a subscription criteria object is created, the following methods can be used:
   *  - subscribe() subscribe on any changes according criteria
   *  - unsubscribe() unsubscribe above changes (the CallbackId is returned by above method)
   *
   */

class Configuration {

  friend class DalObject;
  friend class ConfigObject;
  friend class ConfigurationImpl;
  friend class CacheBase;

  public:

      /**
       *  \brief Constructor to build a configuration object using implementation plug-in.
       *
       *  The constructor expects parameter in format "plugin-name:plugin-parameter".
       *  The plugin-name is used to get implementation shared library by adding "lib"
       *  prefix and ".so" suffix, e.g. "oksconfig" -> "liboksconfig.so". 
       *  The plugin-parameter is optional; if non-empty, it is passed to the plug-in
       *  constructor.
       *
       *  \param spec         database name to be understood by the database implementation
       *
       *  \throw daq::config::Generic in case of an error
       */

    Configuration(const std::string& spec);


      /** Get implementation plug-in and it's parameter used to build config object */

    const std::string& get_impl_spec() const noexcept {return m_impl_spec;}


      /** Get implementation plug-in name used to build config object */
     
    const std::string& get_impl_name() const noexcept {return m_impl_name;}


       /** Get implementation plug-in parameter used to build config object */

    const std::string& get_impl_param() const noexcept {return m_impl_param;}


      /**
       *  \brief Destructor to destroy a configuration object.
       *
       *  The destructor unloads database for given database implementation
       *  and destroys all user objects in cache.
       */

    ~Configuration() noexcept;


  public:

      /**
       *  \brief The user notification callback function which
       *  is invoked in case of changes.
       *
       *  \param changed_classes    vector of changed classes
       *  \param parameter          user-defined parameter
       */

    typedef void (*notify)(
      const std::vector<ConfigurationChange *> & changed_classes,
      void * parameter
    );

      /**
       *  \brief The user notification callback function which
       *  is invoked before changes are going to be applied.
       *
       *  \param parameter          user-defined parameter
       */

    typedef void (*pre_notify)(
      void * parameter
    );


  private: // Types to be used in public API below

      // structure keeps information about callback subscriptions

    struct CallbackSubscription {
      notify m_cb;
      void * m_param;
      ConfigurationSubscriptionCriteria m_criteria;
    };

    struct CallbackPreSubscription {
      pre_notify m_cb;
      void * m_param;
    };


  public:

      /**
       *  \brief Callback identifier.
       *
       *  It uniquely identifies a callback inside given process.
       *  It is returned by subscribe() method and must be used
       *  as parameter for the unsubscribe() methods.
       */

    typedef CallbackSubscription * CallbackId;


      /**
       *  \brief Subscribe on configuration changes.
       *
       *  The method is used to make a subscription. The returned value
       *  is a subscription handler.
       *
       *  When subscribed changes occurred, the user callback function is invoked with
       *  changes description and parameter 'user_param' defined by user.
       *
       *  \param criteria     subscription criteria
       *  \param user_cb      user-defined callback function
       *  \param user_param   optional user-defined parameter
       *
       *  \return \b non-null value in case of success (the value to be used for unsubscribe() method).
       *
       *  \throw daq::config::Generic in case of an error
       */

    CallbackId subscribe(const ::ConfigurationSubscriptionCriteria& criteria, notify user_cb, void * user_param = nullptr);


      /**
       *  \brief Subscribe on pre-notification on configuration changes.
       *
       *  The method is used to make complimentary subscription on pre-notification about changes,
       *  that can only be used together with real subscription on changes, i.e. using
       *  subscribe(const ::ConfigurationSubscriptionCriteria&, notify, void *) method.
       *
       *  When subscribed changes occurred, but before they are going to be applied,
       *  the user callback function is invoked with parameter 'user_param' defined by user.
       *  This subscription can be used to be informed, that some changes are took
       *  place already and will be applied immediately after user's code exits given callback function.
       *
       *  \param user_cb      user-defined callback function
       *  \param user_param   optional user-defined parameter
       *
       *  \return \b non-null value in case of success (the value to be used for unsubscribe() method).
       *
       *  \throw daq::config::Generic in case of an error
       */

    CallbackId subscribe(pre_notify user_cb, void * user_param = nullptr);


      /**
       *  \brief Remove callback function.
       *
       *  Remove callback function previously added by the subscribe() methods.
       *  If the parameter is a non-null value, it must be equal to id returned by the add_callback() method.
       *  Otherwise the method stops all subscription.
       *
       *  \throw daq::config::Generic in case of an error (e.g. bad ID, plugin-specific problems)
       */

    void unsubscribe(CallbackId cb_handler = 0);



      /**
       *  \brief Checks validity of pointer to an objects of given user class.
       *
       *  Check if the pointer to the object is a valid pointer in the cache.
       *  Dangling pointers to removed objects may appear after notification.
       *
       *  \return Return \b true if the pointer is valid and \b false otherwise.
       */

    template<class T> bool is_valid(const T * object) noexcept;


  private:

    /// \throw daq::config::Generic in case of an error
    void reset_subscription();


  public:

      /**
       *  \brief Update cache of objects in case of modification.
       *
       *  Only is called, when a user subscription to related class is set.
       *  It is used by automatically generated data access libraries.
       *
       *  \param modified  vector of modified objects of given user class (objects to be re-read in cache)
       *  \param removed   vector of removed objects of given user class (objects to be removed from cache)
       *  \param created   vector of created objects of given user class (objects to be reset in cache, if they were removed)
       */

    template<class T> void update(const std::vector<std::string>& modified,
		                  const std::vector<std::string>& removed,
				  const std::vector<std::string>& created) noexcept;


      /**
       *  \brief System function invoked in case of modifications.
       *
       *  It is used by the database implementation.
       *  Update cache of template DB objects.
       */

    void update_cache(std::vector<ConfigurationChange *>& changes) noexcept;


      /**
       *  \brief System callback function invoked in case of modifications.
       *
       *  It is used by the database implementation.
       *  Only is called, when a user subscription to related class is set.
       */

    static void system_cb(std::vector<ConfigurationChange *>&, Configuration *) noexcept;


      /**
       *  \brief System callback function invoked in case of pre-modifications.
       *
       *  It is used by the database implementation.
       *  Only is called, when a user subscription is set.
       */

    static void system_pre_cb(Configuration *) noexcept;


      /**
       *  \brief Update state of objects after abort operations.
       *
       *  It is used by automatically generated data access libraries.
       */

    template<class T> void _reset_objects() noexcept;


      /**
       *  \brief Mark object of given template class as unread (multi-thread unsafe).
       *
       *  Is used by automatically generated data access libraries code after reading parameters for substitution,
       *  since cache contains objects with non-substituted attributes. Should not be explicitly used by user.
       *
       *  The method is used by the unread_all_objects() method.
       *  \param  cache_ptr pointer to the cache of template object of given template class (has to be downcasted)
       */

    template<class T> static void _unread_objects(CacheBase * cache_ptr) noexcept;


      /**
       *  \brief Rename object of given template class (multi-thread unsafe).
       *
       *  Is used by automatically generated data access libraries when an object has been renamed by user's code.
       *  Should not be explicitly used by user.
       *
       *  The method is used by the unread_all_objects() method.
       *  \param  cache_ptr pointer to the cache of template object of given template class (has to be downcasted)
       *  \param  old_id old object ID
       *  \param  new_id new object ID
       */

    template<class T> static void _rename_object(CacheBase* cache_ptr, const std::string& old_id, const std::string& new_id) noexcept;


      /**
       *  \brief Update state of all objects in cache after abort / commit operations.
       *
       *  It is used by automatically generated data access libraries.
       */

    void _reset_all_objects() noexcept;


      /**
       *  \brief Unread all template (i.e. set their state as uninitialized) and implementation objects (i.e. clear their cache).
       *
       *  Unread template objects result changing their state to uninitialized. They will be re-initialized, when accessed by user code.
       *  This feature is used by attribute converter methods using unread_all_objects().
       *
       *  Unread implementation objects result removing any information from implementation cache, e.g. clear any object attributes data read from server.
       *  One may use the unread_all_objects() to re-read database after reload.
       *
       *  \param unread_implementation_objs  if true, clear implementation objects; otherwise only template objects are unread and the implementation config objects are still valid.
       */

    void
    unread_all_objects(bool unread_implementation_objs = false) noexcept;


      /**
       *  \brief Unread all template (i.e. set their state as uninitialized) objects.
       *
       *  Unread template objects result changing their state to uninitialized. They will be re-initialized, when accessed by user code.
       *  This feature is used by attribute converter methods using unread_all_objects().
       */

    void
    unread_template_objects() noexcept
    {
      std::lock_guard<std::mutex> scoped_lock(m_tmpl_mutex);
      _unread_template_objects();
    }


      /**
       *  \brief Unread implementation objects (i.e. clear their cache).
       *
       *  This results removing any information from implementation cache, e.g. clear any object attributes data read from server.
       *
       *  \param state  set state of implementation objects after unread; is set to "Unknown" after abort()
       */

    void
    unread_implementation_objects(daq::config::ObjectState state) noexcept
    {
      std::lock_guard<std::mutex> scoped_lock(m_impl_mutex);
      _unread_implementation_objects(state);
    }


  private:

    static void
    update_impl_objects(config::pmap<config::map<ConfigObjectImpl *> * >& cache, ConfigurationChange& change, const std::string * class_name);

    void
    _unread_template_objects() noexcept;

    void
    _unread_implementation_objects(daq::config::ObjectState state) noexcept;


  public:

      /**
       *  \brief Create new object by class name and object id.
       *
       *  The method tries to create an object with given id in given class.
       *  If found, the method fills \c 'object' reference.
       *
       *  \param at           database file where to create new object
       *  \param class_name   name of the class
       *  \param id           object identity
       *  \param object       returned value in case of success
       *
       *  \throw daq::config::Generic in case of an error
       */

    void create(const std::string& at, const std::string& class_name, const std::string& id, ConfigObject& object);


      /**
       *  \brief Create new object by class name and object id.
       *
       *  The method tries to create an object with given id in given class.
       *  If found, the method fills \c 'object' reference.
       *
       *  \param at           create new object at the same database file where \b 'at' object is located
       *  \param class_name   name of the class
       *  \param id           object identity
       *  \param object       returned value in case of success
       *
       *  \throw daq::config::Generic in case of an error
       */

    void create(const ConfigObject& at, const std::string& class_name, const std::string& id, ConfigObject& object);


      /**
       *  \brief Create object of given class by identity and instantiate the template parameter with it.
       *
       *  Such method to be used for user classes generated by the genconfig utility.
       *
       *  \param at            database file where to create new object
       *  \param id            object identity
       *  \param init_object   if true, initialise object's attributes and relationships
       *
       *  \return \b non-null pointer to created object.
       *
       *  \throw daq::config::Generic in case of an error
       */

    template<class T> const T * create(const std::string& at, const std::string& id, bool init_object = false);


      /**
       *  \brief Create object of given class by identity and instantiate the template parameter with it.
       *
       *  Such method to be used for user classes generated by the genconfig utility.
       *
       *  \param at            an existing object of class generated by genconfig to define location of the file where to store new object
       *  \param id            object identity
       *  \param init_object   if true, initialize object's attributes and relationships
       *       *
       *  \return \b non-null pointer to created object.
       *
       *  \throw daq::config::Generic in case of an error
       */

    template<class T> const T * create(const ::DalObject& at, const std::string& id, bool init_object = false);


      /**
       *  \brief Destroy object.
       *
       *  The method tries to destroy given object.
       *
       *  \param object   the object's reference
       *
       *  \throw daq::config::Generic in case of an error
       */

    void destroy_obj(ConfigObject& object);


      /**
       *  \brief Destroy object of given class.
       *
       *  The method tries to destoy given object.
       *
       *  \param  obj  the object's reference
       *
       *  \throw daq::config::Generic in case of an error
       */

    template<class T> void destroy(T& obj);


  public:

      /**
       *  \brief Test the object existence.
       *
       *  The method searches an object with given id within the class and all derived subclasses.
       *  If found, the method returns \b true, otherwise the method return \b false.
       *
       *  \param class_name   name of the class
       *  \param id           object identity
       *  \param rlevel       optional references level to optimize performance (defines how many objects referenced by given object have also to be read to the implementation cache)
       *  \param rclasses     optional array of class names to optimize performance (defines which referenced objects have to be read to the implementation cache)
       *
       *  \throw daq::config::Generic if there is no such class or in case of an error
       */

    bool test_object(const std::string& class_name, const std::string& id, unsigned long rlevel = 0, const std::vector<std::string> * rclasses = 0);


      /**
       *  \brief Get object by class name and object id (multi-thread safe).
       *
       *  The method searches an object with given id within the class and all derived subclasses.
       *
       *  \param class_name   name of the class
       *  \param id           object identity
       *  \param object       returned value in case of success
       *  \param rlevel       optional references level to optimize performance (defines how many objects referenced by given object have also to be read to the implementation cache)
       *  \param rclasses     optional array of class names to optimize performance (defines which referenced objects have to be read to the implementation cache)
       *
       *  \throw daq::config::NotFound exception if there is no such object or \b daq::config::Generic in case of an error
       */

    void get(const std::string& class_name, const std::string& id, ConfigObject& object, unsigned long rlevel = 0, const std::vector<std::string> * rclasses = 0);


      /**
       *  \brief Get all objects of class.
       *
       *  The method returns all objects of given class and objects of subclasses derived from it.
       *
       *  \param class_name   name of the class
       *  \param objects      returned value in case of success
       *  \param query        optional parameter defining selection criteria for objects of given class
       *  \param rlevel       optional references level to optimize performance (defines how many objects referenced by found objects have also to be read to the implementation cache)
       *  \param rclasses     optional array of class names to optimize performance (defines which referenced objects have to be read to the implementation cache)
       *
       *  \throw daq::config::NotFound exception if there is no such class or \b daq::config::Generic in case of an error
       */

    void get(const std::string& class_name, std::vector<ConfigObject>& objects, const std::string& query = "", unsigned long rlevel = 0, const std::vector<std::string> * rclasses = 0);


      /**
       *  \brief Get path between objects.
       *
       *  The method returns all objects which are in the path starting from source object
       *  matching to the path query pattern.
       *
       *  \param obj_from   object to start from
       *  \param query      path query
       *  \param objects    returned value in case of success
       *  \param rlevel     optional references level to optimize performance (defines how many objects referenced by found objects have also to be read to the implementation cache)
       *  \param rclasses   optional array of class names to optimize performance (defines which referenced objects have to be read to the implementation cache)
       *
       *  \throw daq::config::Generic in case of an error
       */

    void get(const ConfigObject& obj_from, const std::string& query, std::vector<ConfigObject>& objects, unsigned long rlevel = 0, const std::vector<std::string> * rclasses = 0);


      /**
       *  \brief Get object of given class by identity and instantiate the template parameter with it (multi-thread safe).
       *
       *  Such method to be used for user classes generated by the genconfig utility.
       *
       *  \param id             object identity
       *  \param init_children  if true, the referenced objects are initialized
       *  \param init           if true, the object's attributes and relationships are read
       *  \param rlevel         optional references level to optimize performance (defines how many objects referenced by given object have also to be read to the implementation cache)
       *  \param rclasses       optional array of class names to optimize performance (defines which referenced objects have to be read to the implementation cache)
       *
       *  \return  Return 0 if there is no object with such id or pointer to object otherwise.
       *
       *  \throw daq::config::Generic in case of an error
       */

  template<class T>
    const T *
    get(const std::string& id, bool init_children = false, bool init = true, unsigned long rlevel = 0, const std::vector<std::string> * rclasses = 0)
    {
      std::lock_guard<std::mutex> scoped_lock(m_tmpl_mutex);
      return _get<T>(id, init_children, init, rlevel, rclasses);
    }



      /**
       *  \brief Get object of given class by object reference and instantiate the template parameter with it (multi-thread safe).
       *
       *  Such method to be used for user classes generated by the genconfig utility.
       *
       *  \param obj            reference to config object, that is used to instantiate template object
       *  \param init_children  if true, the referenced objects are initialized
       *  \param init           if true, the object's attributes and relationships are read
       *
       *  \return Return pointer to object.
       *
       *  \throw daq::config::Generic in case of an error
       */

  template<class T>
    const T *
    get(ConfigObject& obj, bool init_children = false, bool init = true)
    {
      std::lock_guard<std::mutex> scoped_lock(m_tmpl_mutex);
      return _get<T>(obj, init_children, init);
    }


      /**
       *  \brief Get all objects of given class and instantiate a vector of 
       *  the template parameters object with it (multi-thread safe).
       *
       *  Such method to be used for user classes generated by the genconfig utility.
       *
       *  \param objects        the vector is filled by the method
       *  \param init_children  if true, the referenced objects are initialized
       *  \param init           if true, the object's attributes and relationships are read
       *  \param query          optional parameter defining selection criteria for objects of given class
       *  \param rlevel         optional references level to optimize performance (defines how many objects referenced by found objects have also to be read to the implementation cache)
       *  \param rclasses       optional array of class names to optimize performance (defines which referenced objects have to be read to the implementation cache)
       *
       *  \throw daq::config::Generic in case of a problem (e.g. no such class, plug-in specific problem)
       */

  template<class T>
    void
    get(std::vector<const T*>& objects, bool init_children = false, bool init = true, const std::string& query = "", unsigned long rlevel = 0, const std::vector<std::string> * rclasses = 0)
    {
      std::lock_guard<std::mutex> scoped_lock(m_tmpl_mutex);
      _get<T>(objects, init_children, init, query, rlevel, rclasses);
    }

  /**
   *  \brief Generate object of given class by object reference and instantiate the template parameter with it (multi-thread safe).
   *
   *  Such method to be used to generate template objects.
   *
   *  \param obj            reference to config object, that is used to instantiate template object
   *  \param id             ID of generated object could be different from obj.UID()
   *
   *  \return Return pointer to object.
   *
   *  \throw daq::config::Generic in case of an error
   */

  template<class T>
    const T *
    get(ConfigObject& obj, const std::string& id)
    {
      std::lock_guard<std::mutex> scoped_lock(m_tmpl_mutex);
      return _get<T>(obj, id);
    }


  /**
   *  \brief Find object of given class (multi-thread safe).
   *
   *  Could be used for generated template objects.
   *
   *  \param id             ID of object
   *
   *  \return Return pointer to object or nullptr if there is no such object
   *
   *  \throw daq::config::Generic in case of an error
   */

  template<class T>
    const T *
    find(const std::string& id)
    {
      std::lock_guard<std::mutex> scoped_lock(m_tmpl_mutex);
      return _find<T>(id);
    }


      /**
       *  \brief Get signle value of object's relation and instantiate result with it (multi-thread safe).
       *
       *  The method is used by the code generated by the genconfig utility.
       *
       *  \param obj   object
       *  \param name  name of the relationship
       *  \param init  if true, the object and it's referenced objects are initialized
       *
       *  \return Return non-null pointer to object of user class in case if
       *  relationship with such name exists and it's value is set.
       *  Otherwise the method returns 0.
       *
       *  \throw daq::config::Generic in case of a problem (e.g. no relationship with such name, plug-in specific problem)
       */

    template<class T> const T * ref(ConfigObject& obj, const std::string& name, bool init = false) {
      std::lock_guard<std::mutex> scoped_lock(m_tmpl_mutex);
      return _ref<T>(obj, name, init);
    }



      /**
       *  \brief Get multiple values of object's relation and instantiate result with them (multi-thread safe).
       *
       *  The method is used by the code generated by the genconfig utility.
       *
       *  \param obj       object
       *  \param name      name of the relationship
       *  \param objects   returned value
       *  \param init      if true, the objects and their referenced objects are initialized
       *
       *  \throw daq::config::Generic in case of a problem (e.g. no relationship with such name, plug-in specific problem)
       */

    template<class T> void ref(ConfigObject& obj, const std::string& name, std::vector<const T*>& objects, bool init = false) {
      std::lock_guard<std::mutex> scoped_lock(m_tmpl_mutex);
      _ref<T>(obj, name, objects, init);
    }



      /**
       *  \brief Get template DAL objects holding references on this object via given relationship (multi-thread safe).
       *
       *  The method returns objects of class V, which have references on given object via explicitly provided relationship name.
       *  If the relationship name is set to "*", then the method takes into account  all relationships of all objects.
       *  The method is efficient only for composite relationships (i.e. when a parent has composite reference on this object).
       *  For generic relationships the method performs full scan of all database objects.
       *  It is not recommended at large scale to build complete graph of relations between all database object.
       *
       *  \param obj                   object
       *  \param objects               returned value
       *  \param relationship_name     name of the relationship, via which the object is referenced
       *  \param check_composite_only  only returned composite parent objects
       *  \param init                  if true, the returned objects and their referenced objects are initialized
       *  \param rlevel                optional references level to optimize performance (defines how many objects referenced by found objects have also to be read to the implementation cache)
       *  \param rclasses              optional array of class names to optimize performance (defines which referenced objects have to be read to the implementation cache)
       *
       *  \throw daq::config::Generic in case of an error
       */

    template<class T, class V> void referenced_by(const T& obj, std::vector<const V*>& objects, const std::string& relationship_name = "*", bool check_composite_only = true, bool init = false, unsigned long rlevel = 0, const std::vector<std::string> * rclasses = nullptr);


    /**
     *  \brief Get DAL objects  holding references on this object via given relationship (multi-thread safe).
     *
     *  The method returns vector of DalObject, which have references on given object via explicitly provided relationship name.
     *  If the relationship name is set to "*", then the method takes into account  all relationships of all objects.
     *
     *  It is expected that the DAL for returned objects is generated and linked with user code. If this is not the case, then an exception will be thrown.
     *  The parameter upcast_unregistered allows to select one of the registered base classes instead.
     *  Note, this will be a random base class, not the closest based one.
     *
     *  The method is efficient only for composite relationships (i.e. when a parent has composite reference on this object).
     *  For generic relationships the method performs full scan of all database objects.
     *  It is not recommended at large scale to build complete graph of relations between all database object.
     *
     *  \param obj                   object
     *  \param relationship_name     name of the relationship, via which the object is referenced
     *  \param upcast_unregistered   if true, try to upcast objects of classes which DAL classes are not loaded; otherwise throw exception if such DAL class is not registered
     *  \param check_composite_only  only returned composite parent objects
     *  \param init                  if true, the returned objects and their referenced objects are initialized
     *  \param rlevel                optional references level to optimize performance (defines how many objects referenced by found objects have also to be read to the implementation cache)
     *  \param rclasses              optional array of class names to optimize performance (defines which referenced objects have to be read to the implementation cache)
     *  \return                      objects referencing given one
     *
     *  \throw daq::config::Generic in case of an error
     */

    std::vector<const DalObject*>
    referenced_by(const DalObject& obj, const std::string& relationship_name = "*", bool check_composite_only = true, bool upcast_unregistered = true, bool init = false, unsigned long rlevel = 0, const std::vector<std::string> * rclasses = nullptr);


      /**
       *  \brief Cast objects from one class to another (multi-thread safe).
       *
       *  Try to cast object SOURCE to TARGET. Returns 0 if not successful.
       *  Do not use the normal \b dynamic_cast<T>() for database classes.
       *
       *  \return Return nullptr if the cast is not successful.
       */

    template<class TARGET, class SOURCE> const TARGET *cast(const SOURCE *s) noexcept;


  private:

    /// \throw daq::config::Generic or daq::config::NotFound
    void _get(const std::string& class_name, const std::string& id, ConfigObject& object, unsigned long rlevel, const std::vector<std::string> * rclasses);

    /// \throw daq::config::Generic
    template<class T> const T * _get(const std::string& id, bool init_children = false, bool init = true, unsigned long rlevel = 0, const std::vector<std::string> * rclasses = 0);

    /// \throw daq::config::Generic
    template<class T> const T * _get(ConfigObject& obj, bool init_children = false, bool init = true);

    /// \throw daq::config::Generic
    template<class T> const T * _get(ConfigObject& obj, const std::string& id);

    /// \throw daq::config::Generic
    template<class T> void _get(std::vector<const T*>& objects, bool init_children = false, bool init = true, const std::string& query = "", unsigned long rlevel = 0, const std::vector<std::string> * rclasses = 0);

    /// \throw daq::config::Generic
    template<class T> DalObject * _make_instance(ConfigObject& obj, const std::string& uid)
    {
      // note upcast since the _get() returns pointer to T
      return const_cast<T*>(_get<T>(obj, uid));
    }

    std::vector<const DalObject*> make_dal_objects(std::vector<ConfigObject>& objs, bool upcast_unregistered);

    const DalObject* make_dal_object(ConfigObject& obj, const std::string& uid, const std::string& class_name);


    // should be made private

  public:

    template<class T>
    void
    downcast_dal_objects(const std::vector<const T *>& objs, bool upcast_unregistered, std::vector<const DalObject*>& result)
    {
      for (auto& i : objs)
        result.push_back(i);
    }

    template<class T>
    void
    downcast_dal_object(const T * obj, bool upcast_unregistered, std::vector<const DalObject*>& result)
    {
      if(obj)
        result.push_back(obj);
    }


      /**
       *  \brief Multi-thread unsafe version of find(const std::string&) method
       *  \throw daq::config::Generic in case of an error
       */

    template<class T> const T * _find(const std::string& id);


      /**
       *  \brief Multi-thread unsafe version of ref(ConfigObject&, const std::string&, bool);
       *  The method should not be used by user.
       *  \throw daq::config::Generic in case of an error
       */

    template<class T> const T * _ref(ConfigObject& obj, const std::string& name, bool read_children);


      /**
       *  \brief Multi-thread unsafe version of ref(ConfigObject&, const std::string&, std::vector<const T*>&, bool);
       *  The method should not be used by user.
       *  \throw daq::config::Generic in case of an error
       */

    template<class T> void _ref(ConfigObject& obj, const std::string& name, std::vector<const T*>& results, bool read_children);


      /**
       * \brief Checks if cast from source class to target class is allowed.
       *
       * \param target  name of desired class (e.g. try to cast object of "source" class to this "target" one)
       * \param source  name of casted object class
       *
       * \return Return \b true if the cast is allowed by database schema
       */

    bool try_cast(const std::string& target, const std::string& source) noexcept;

    bool try_cast(const std::string* target, const std::string* source) noexcept;


  private:

      /** Helper method to prepare exception text when template ref() method fails **/

    static std::string mk_ref_ex_text(const char * what, const std::string& cname, const std::string& rname, const ::ConfigObject& obj) noexcept;


      /** Helper method to prepare exception text when template referenced_by() method fails **/

    static std::string mk_ref_by_ex_text(const std::string& cname, const std::string& rname, const ::ConfigObject& obj) noexcept;


    // database manipulations

  public:

      /**
       *  \brief Check if database is correctly loaded.
       *
       *  Check state of the database after configuration object creation.
       *
       *  \return \b true if the database was successfully loaded and \b false otherwise.
       */

    bool loaded() const noexcept;


      /**
       *  \brief Load database according to the name.
       *  If name is empty, take it from TDAQ_DB_NAME and TDAQ_DB_DATA
       *  environment variables.
       *
       *  \throw daq::config::Generic in case of an error
       */

    void load(const std::string& db_name);


      /**
       *  \brief Unload database.
       *
       *  The database should be previously loaded.
       *  The method destroys all user objects from cache (i.e. created
       *  via config and template get methods) and frees all DB resources
       *  allocated by the implementation plug-in.
       *
       *  \throw daq::config::Generic in case of an error
       */

    void unload();


      /**
       *  \brief Create database.
       *
       *  The method creates database according to the name and list of others
       *  database files to be included.
       *
       *  \param db_name       name of new database file (must be an absolute path to non-existing file)
       *  \param includes      optional list of others database files to be included
       *
       *  \throw daq::config::Generic in case of an error
       */

    void create(const std::string& db_name, const std::list<std::string>& includes);


      /**
       *  \brief Get write access status.
       *
       *  Check if given database file is writable by current user.
       *
       *  \param db_name       name of database
       *
       *  \return \b true, if database file is writable and \b false otherwise.
       *
       *  \throw daq::config::Generic in case of an error
       */

    bool is_writable(const std::string& db_name) const;


      /**
       *  \brief Add include file to existing database.
       *
       *  The method adds (and loads) existing include file to the database.
       *
       *  \param db_name       name of database file to be included
       *  \param include       file to be included
       *
       *  \throw daq::config::Generic in case of an error
       */

    void add_include(const std::string& db_name, const std::string& include);


      /**
       *  \brief Remove include file.
       *
       *  The method removes existing include file from the database.
       *
       *  \param db_name       name of database file from which the include to be removed
       *  \param include       file to be removed from includes
       *
       *  \throw daq::config::Generic in case of an error
       */

    void remove_include(const std::string& db_name, const std::string& include);


      /**
       *  \brief Get include files.
       *
       *  The method returns list of files included by given database.
       *
       *  \param db_name       name of database file
       *  \param includes      returned list of include files
       *
       *  \throw daq::config::Generic in case of an error
       */

    void get_includes(const std::string& db_name, std::list<std::string>& includes) const;


      /**
       *  \brief Get list of updated files to be committed.
       *
       *  The method returns list of uncommitted database files.
       *
       *  \param dbs           returned list of uncommitted database files
       *
       *  \throw daq::config::Generic in case of an error
       */

    void get_updated_dbs(std::list<std::string>& dbs) const;


      /**
       *  \brief Set commit credentials.
       *
       *  The method sets credentials used by commit method.
       *
       *  \param user       user name
       *  \param password   user password
       *
       *  \throw daq::config::Generic in case of an error
       */

    void set_commit_credentials(const std::string& user, const std::string& password);


      /**
       *  \brief Commit database changes.
       *
       *  The method commits the changes after a database was modified.
       *
       *  \param log_message   log information
       *
       *  \throw daq::config::Generic in case of an error
       */

    void commit(const std::string& log_message = "");


      /**
       *  \brief Abort database changes.
       *
       *  The method rolls back non-committed database modifications.
       *
       *  \throw daq::config::Generic in case of an error
       */

    void abort();


    /**
     *  \brief Prefetch all data into client cache.
     *
     *  The method reads all objects defined in database into client cache.
     *
     *  \throw daq::config::Generic in case of an error
     */

    void prefetch_all_data();


    // access versions

  public:

    /**
     *  \brief Get new config versions.
     *  \return repository changes: new versions created on remote origin after current HEAD version, or externally modified files
     *  \throw daq::config::Generic in case of an error
     */

    std::vector<daq::config::Version>
    get_changes();


    /**
     *  \brief Get repository versions in interval.
     *
     *  Access historical versions.
     *
     *  The date/time format has to be either a date in format "yyyy-mm-dd" or date-and-time in format "yyyy-mm-dd hh:mm:ss" (UTC).
     *
     *  \param since limit the versions committed on-or-after the specified hash key, tag or date/time; if empty, start from earliest available
     *  \param until limit the versions committed on-or-before the specified hash key, tag or date/time; if empty, retrieve all versions until latest available
     *  \param type define query type
     *  \param skip_irrelevant if true, ignore changes not affecting loaded configuration
     *  \return repository versions satisfying query
     *  \throw daq::config::Generic in case of an error
     */

    std::vector<daq::config::Version>
    get_versions(const std::string& since, const std::string& until, daq::config::Version::QueryType type = daq::config::Version::query_by_date, bool skip_irrelevant = true);



    // access to schema description

  public:

      /**
       *  \brief The method provides access to description of class.
       *
       *  \param  class_name   name of the class
       *  \param  direct_only  if true is set explicitly, return descriptions of direct attributes, relationships, super- and subclasses; by default return all descriptions taking into account inheritance
       *  \return              Return pointer to class description object.
       *
       *  \throw daq::config::NotFound exception if there is no class with such name or \b daq::config::Generic in case of a problem
       */

    const daq::config::class_t& get_class_info(const std::string& class_name, bool direct_only = false);


  private:

      // cache, storing descriptions of schema

    config::map<daq::config::class_t *> p_direct_classes_desc_cache;
    config::map<daq::config::class_t *> p_all_classes_desc_cache;

  public:

    /**
     *  \brief Export configuration schema into ptree.
     *
     *  \param  tree         output ptree object
     *  \param  classes      regex defining class names; all classes if empty
     *  \param  direct_only  if true is set explicitly, return descriptions of direct attributes, relationships, super- and subclasses; by default return all descriptions taking into account inheritance
     *
     *  \throw daq::config::Generic in case of a problem
     */

    void
    export_schema(boost::property_tree::ptree& tree, const std::string& classes = "", bool direct_only = false);


    /**
     *  \brief Export configuration data into ptree.
     *
     *  \param  tree               output ptree object
     *  \param  classes            regex defining class names; ignore if empty
     *  \param  objects            regex defining object IDs; ignore if empty
     *  \param  files              regex defining data file names; ignore if empty
     *  \param  empty_array_item   if provided, add this item to mark empty arrays
     *
     *  \throw daq::config::Generic in case of a problem
     */

    void
    export_data(boost::property_tree::ptree& tree, const std::string& classes = "", const std::string& objects = "", const std::string& files = "", const std::string& empty_array_item = "");


    // user-defined converters

  public:

       /** Base converter class with a virtual destructor **/

    class AttributeConverterBase {

      public:

        virtual ~AttributeConverterBase() {;}

    };


      /**
       *  \brief Virtual converter class.
       *
       *  To implement a converter for given type of attribute, a user needs to inherit from this class
       *  providing the attribute type and implementing the convert() method.
       *  To be used an object of the user converter class has to be registered using register_converter() method.
       */

    template<class T> class AttributeConverter : public AttributeConverterBase {

      public:

          /**
           *  \brief Method to make the conversion of attribute value.
           *
           *  When the converter object is registered, the convert() method is called for each
	   *  attribute value of given type to read any database object. The parameters passed to
	   *  the convert method are described below:
           *  \param value      reference on the value to be converted
           *  \param conf       const reference on the configuration object
           *  \param obj        const reference on the converted object
           *  \param attr_name  name of the attribute which value to be converted
           *
           *  The method modifies parameter 'value', if a conversion is required.
           */

        virtual void convert(T& value, const Configuration& conf, const ConfigObject& obj, const std::string& attr_name) = 0;

    };


      /**
       *  \brief Register user function for attribute conversion.
       *
       *  The user can register several objects which are used for attribute
       *  values conversion. The attributes conversion type is defined by the
       *  template parameter, e.g. given object to be used for string attribute
       *  values conversion, another object to be used for short unsigned integers, etc.
       *  It is possible to define several converters for each type. There is no
       *  check that given object was already registered or not. It is registered
       *  several times, the conversion will be done several times.
       *  \param object  the converter object
       */

    template<class T> void register_converter(AttributeConverter<T> * object) noexcept;


      /**
       *  \brief Converts single value.
       *
       *  The method is used by the code generated by the genconfig utility.
       */

    template<class T> void convert(T& value, const ConfigObject& obj, const std::string& attr_name) noexcept;


      /**
       *  \brief Converts vector of single values.
       *
       *  The method is used by the code generated by the genconfig utility.
       */

    template<class T> void convert2(std::vector<T>& value, const ConfigObject& obj, const std::string& attr_name) noexcept;


  public:


      /**
       *  \brief Print out profiling information.
       *
       *  The method prints out to the standard output stream profiling information of configuration
       *  object and it's implementation.
       */

    void print_profiling_info() noexcept;


  private:

    std::atomic<uint_least64_t> p_number_of_cache_hits;
    std::atomic<uint_least64_t> p_number_of_template_object_created;
    std::atomic<uint_least64_t> p_number_of_template_object_read;


  private:

    config::fmap<config::fset> p_superclasses;
    config::fmap<config::fset> p_subclasses;

    void set_subclasses() noexcept;

  public:

      /** Get names of superclasses for each class **/

    const config::fmap<config::fset>& superclasses() const noexcept {return p_superclasses;}


      /** Get names of subclasses for each class **/

    const config::fmap<config::fset>& subclasses() const {return p_subclasses;}


  private:

    config::map<std::list<AttributeConverterBase*> * > m_convert_map;


    // cache of objects for user-defined classes

  public:

      /**
       * \brief Cache of template object of given type.
       *
       *  The class defines the cache of template objects of given type.
       *  The objects are stored in cache, where the key is object-ID and the value is a pointer on template object.
       *
       *  The access to cache and objects insertion are provided via two get() methods:
       *  \li <tt> T * get(Configuration&, ConfigObject&, bool, bool) </tt> - get template object for given config object
       *  \li <tt> T * get(Configuration&, const std::string&, bool, bool, unsigned long, const std::vector<std::string> *) </tt> - get template object for given object ID
       *
       */

    template<class T> class Cache : public CacheBase {
    
      friend class Configuration;
    
      public:

        Cache() :
            CacheBase(DalFactory::instance().functions(T::s_class_name))
        {
          ;
        }


        virtual ~Cache() noexcept;


           /**
            *  \brief Get template object from cache by config object.
            *
            *  The method searches an object with id of given config object within the cache.
            *  If found, the method sets given config object as implementation of the template
	    *  object and returns pointer on the template object.
            *  If there is no such object in cache, then it is created from given config object.
            *
            *  In case of success, the new object is put into cache and pointer to the object is returned.
            *  If there is no such object for given template class, then \b null pointer is returned.
            *
            *  \param config         the configuration object
            *  \param obj            the config object used to set for the template object
            *  \param init_children  if true, the referenced objects are initialized (only applicable during creation of new object)
            *  \param init_object    if true, the object's attributes and relationships are read(only applicable during creation of new object)
            *
            *  \return Return pointer to object.
            *
            *  \throw daq::config::Generic is no such class for loaded configuration DB schema or in case of an error
            */

        T * get(Configuration& config, ConfigObject& obj, bool init_children, bool init_object);


           /**
            *  \brief Get template object from cache by object's ID.
            *
            *  The method searches an object with given id within the cache.
            *  If found, the method returns pointer on it.
            *  If there is no such object in cache, there is an attempt to create new object.
            *  In case of success, the new object is put into cache and pointer to the object is returned.
            *  If there is no such object for given template class, then \b null pointer is returned.
            *
            *  \param config         the configuration object
            *  \param name           object identity
            *  \param init_children  if true, the referenced objects are initialized (only applicable during creation of new object)
            *  \param init_object    if true, the object's attributes and relationships are read(only applicable during creation of new object)
            *  \param rlevel         optional references level to optimize performance (defines how many objects referenced by given object have also to be read to the implementation cache during creation of new object)
            *  \param rclasses       optional array of class names to optimize performance (defines which referenced objects have to be read to the implementation cache during creation of new object)
            *
            *  \return Return pointer to object. It can be \b null, if there is no such object found.
            *
            *  \throw daq::config::Generic is no such class for loaded configuration DB schema or in case of an error
            */

          T * get(Configuration& config, const std::string& name, bool init_children, bool init_object, unsigned long rlevel, const std::vector<std::string> * rclasses);


           /**
            *  \brief Find template object using ID.
            *
            *  The method is suitable for generated template objects.
            *
            *  In case of success, the new object is put into cache and pointer to the object is returned.
            *  If there is no such object for given template class, then \b null pointer is returned.
            *
            *  \param id             ID of generated object
            *
            *  \return Return pointer to object.
            *
            *  \throw daq::config::Generic is no such class for loaded configuration DB schema or in case of an error
            */


          T *
          find(const std::string& id);


           /**
            *  \brief Generate template object using config object and ID.
            *
            *  The method searches an object with id of given config object within the cache using given ID.
            *  If found, the method sets given config object as implementation of the template
            *  object and returns pointer on the template object.
            *  If there is no such object in cache, then it is created from given config object.
            *
            *  In case of success, the new object is put into cache and pointer to the object is returned.
            *  If there is no such object for given template class, then \b null pointer is returned.
            *
            *  \param config         the configuration object
            *  \param obj            the config object used to set for the template object
            *  \param id             ID of generated object
            *
            *  \return Return pointer to object.
            *
            *  \throw daq::config::Generic is no such class for loaded configuration DB schema or in case of an error
            */

          T *
          get(Configuration& config, ConfigObject& obj, const std::string& id);


      private:

        config::map<T*> m_cache;
        config::multimap<T*> m_t_cache;


    };

  private:

      // Get cache for this type of objects.

    template<class T> Cache<T> * get_cache() noexcept;

    config::fmap<CacheBase*> m_cache_map;

    void rename_object(ConfigObject& obj, const std::string& new_id);

    template<class T>
    void
    set_cache_unread(const std::vector<std::string>& objects, Cache<T>& c) noexcept
    {
      for (const auto& i : objects)
        {
          // unread template objects
          auto x = c.m_cache.find(i);
          if (x != c.m_cache.end())
            {
              std::lock_guard<std::mutex> scoped_lock(x->second->m_mutex);
              x->second->p_was_read = false;
            }

          // unread generated objects if any
          auto range = c.m_t_cache.equal_range(i);
          for (auto it = range.first; it != range.second; it++)
            {
              std::lock_guard<std::mutex> scoped_lock(it->second->m_mutex);
              it->second->p_was_read = false;
            }
        }
    }


  public:

      /**
       *  \brief Prints out details of configuration object.
       * 
       *  For the moment only inheritance hierarchy of configuration database is printed.
       *  In future it is planned to add more details, such as:
       *  - config objects
       *  - status of template cache
       *  - profiling info
       */

    void print(std::ostream&) const noexcept;


    // representation

  private:

    ConfigurationImpl * m_impl;
    std::string m_impl_spec;
    std::string m_impl_name;
    std::string m_impl_param;
    void * m_shlib_h;


    // user notification

  private:

      // user callbacks with parameters

    typedef std::set< CallbackSubscription * , std::less<CallbackSubscription *> > CallbackSet;
    typedef std::set< CallbackPreSubscription * , std::less<CallbackPreSubscription *> > PreCallbackSet;

    CallbackSet m_callbacks;
    PreCallbackSet m_pre_callbacks;


      // method to find callback by handler

    CallbackSubscription * find_callback(CallbackId cb_handler) const;


  public:

    /** Add global action performed by user code on db [un]load and updates. */
    void
    add_action(ConfigAction * ac);

    /** Remove global action performed by user code on db [un]load and updates. */
    void
    remove_action(ConfigAction * ac);

  private:

    std::list<ConfigAction *> m_actions;
    void action_on_update(const ConfigObject& obj, const std::string& name);


  private:

    mutable std::mutex m_impl_mutex;  // mutex used to access implementation objects (i.e. ConfigObjectImpl objects)
    mutable std::mutex m_tmpl_mutex;  // mutex used to access template objects (i.e. generated DAL)
    mutable std::mutex m_actn_mutex;  // mutex is used to access actions
    mutable std::mutex m_else_mutex;  // mutex used to access subscription, attribute converter, etc. objects


    // prevent copy constructor and operator=

  private:

    Configuration(const Configuration&);
    Configuration& operator=(const Configuration&);

};

  /**
   *  Operator prints out to stream configuration using method print().
   */

std::ostream& operator<<(std::ostream& s, const Configuration & c);


//////////////////////////////////////////////
//// Implementation of template methods.  ////
//////////////////////////////////////////////


template<class T>
  const T *
  Configuration::create(const std::string& at, const std::string& id, bool init_object)
  {
    ConfigObject obj;

    std::lock_guard<std::mutex> scoped_lock(m_tmpl_mutex);
    create(at, T::s_class_name, id, obj);
    return get_cache<T>()->get(*this, obj, false, init_object);
  }


template<class T>
  void
  Configuration::destroy(T& obj)
  {
    destroy_obj(const_cast<ConfigObject&>(obj.config_object()));
  }

// Get object of given class and instantiate the template parameter with it.
template<class T>
  const T *
  Configuration::_get(const std::string& name, bool init_children, bool init_object, unsigned long rlevel, const std::vector<std::string> * rclasses)
  {
    return get_cache<T>()->get(*this, name, init_children, init_object, rlevel, rclasses);
  }

// Instantiate the template parameter using existing config object.
template<class T>
  const T *
  Configuration::_get(ConfigObject& obj, bool init_children, bool init_object)
  {
    return get_cache<T>()->get(*this, obj, init_children, init_object);
  }

template<class T>
  const T *
  Configuration::_get(ConfigObject& obj, const std::string& id)
  {
    return get_cache<T>()->get(*this, obj, id);
  }

template<class T>
  const T *
  Configuration::_find(const std::string& id)
  {
    auto it = m_cache_map.find(&T::s_class_name);
    return (it != m_cache_map.end() ? static_cast<Cache<T>*>(it->second)->find(id) : nullptr);
  }

// Get all objects the given class and instantiate a vector of the template parameters object with it.
template<class T>
  void
  Configuration::_get(std::vector<const T*>& result, bool init_children, bool init_object, const std::string& query, unsigned long rlevel, const std::vector<std::string> * rclasses)
  {
    std::vector<ConfigObject> objs;

    try
      {
        get(T::s_class_name, objs, query, rlevel, rclasses);
      }
    catch (daq::config::NotFound & ex)
      {
        std::ostringstream text;
        text << "wrong database schema, cannot find class \'" << ex.get_data() << '\'';
        throw daq::config::Generic(ERS_HERE, text.str().c_str());
      }

    if (!objs.empty())
      {
        if (Configuration::Cache<T> * the_cache = get_cache<T>())
          {
            for (auto& i : objs)
              {
                result.push_back(the_cache->get(*this, i, init_children, init_object));
              }
          }
      }
  }


// Get relation from object and instantiate result with it.
template<class T>
  const T *
  Configuration::_ref(ConfigObject& obj, const std::string& name, bool read_children)
  {
    ::ConfigObject res;

    try
      {
        obj.get(name, res);
      }
    catch (daq::config::Generic & ex)
      {
        throw(daq::config::Generic( ERS_HERE, mk_ref_ex_text("an object", T::s_class_name, name, obj).c_str(), ex ) );
      }

    return ((!res.is_null()) ? get_cache<T>()->get(*this, res, read_children, read_children) : nullptr);
  }


// Get multiple relations from object and instantiate result with it.
template<class T>
  void
  Configuration::_ref(ConfigObject& obj, const std::string& name, std::vector<const T*>& results, bool read_children)
  {
    std::vector<ConfigObject> objs;

    results.clear();

    try
      {
        obj.get(name, objs);
        results.reserve(objs.size());

        for (auto& i : objs)
          {
            results.push_back(get_cache<T>()->get(*this, i, read_children, read_children));
          }
      }
    catch (daq::config::Generic & ex)
      {
        throw(daq::config::Generic( ERS_HERE, mk_ref_ex_text("objects", T::s_class_name, name, obj).c_str(), ex ) );
      }
  }

template<class T, class V>
  void
  Configuration::referenced_by(const T& obj, std::vector<const V*>& results, const std::string& relationship_name, bool check_composite_only, bool init, unsigned long rlevel, const std::vector<std::string> * rclasses)
  {
    std::vector<ConfigObject> objs;

    results.clear();

    std::lock_guard<std::mutex> scoped_lock(m_tmpl_mutex);

    try
      {
        obj.p_obj.referenced_by(objs, relationship_name, check_composite_only, rlevel, rclasses);

        for (auto& i : objs)
          {
            if (try_cast(V::s_class_name, i.class_name()) == true)
              {
                if (const V * o = get_cache<V>()->get(*this, i, init, init))
                  {
                    results.push_back(o);
                  }
              }
          }
      }
    catch (daq::config::Generic & ex)
      {
        throw(daq::config::Generic( ERS_HERE, mk_ref_by_ex_text(V::s_class_name, relationship_name, obj.p_obj).c_str(), ex ) );
      }
  }


template<class T>
  Configuration::Cache<T>::~Cache() noexcept
  {
    // delete each object in cache
    for (const auto& i : m_cache)
      {
        delete i.second;
      }
  }


template<class T>
  T *
  Configuration::Cache<T>::get(Configuration& config, ConfigObject& obj, bool init_children, bool init_object)
  {
    T*& result(m_cache[obj.m_impl->m_id]);
    if (result == nullptr)
      {
        result = new T(config, obj);
        if (init_object)
          {
            std::lock_guard<std::mutex> scoped_lock(result->m_mutex);
            result->init(init_children);
          }
      }
    else if(obj.m_impl != result->p_obj.m_impl)
      {
        std::lock_guard<std::mutex> scoped_lock(result->m_mutex);
        result->set(obj); // update implementation object; to be used in case if the object is re-created
      }
    increment_gets(config);
    return result;
  }

template<class T>
  T *
  Configuration::Cache<T>::find(const std::string& id)
  {
    auto it = m_cache.find(id);
    return (it != m_cache.end() ? it->second : nullptr);
  }

template<class T>
  T *
  Configuration::Cache<T>::get(Configuration& db, ConfigObject& obj, const std::string& id)
  {
    T*& result(m_cache[id]);
    if (result == nullptr)
      {
        result = new T(db, obj);
        if (id != obj.UID())
          {
            result->p_UID = id;
            m_t_cache.emplace(obj.UID(), result);
          }
      }
    else if(obj.m_impl != result->p_obj.m_impl)
      {
        std::lock_guard<std::mutex> scoped_lock(result->m_mutex);
        result->set(obj); // update implementation object; to be used in case if the object is re-created
      }
    increment_gets(db);
    return result;
  }


  // Get object from cache or create it.

template<class T> T *
Configuration::Cache<T>::get(Configuration& config, const std::string& name, bool init_children, bool init_object, unsigned long rlevel, const std::vector<std::string> * rclasses)
{
  typename config::map<T*>::iterator i = m_cache.find(name);
  if(i == m_cache.end()) {
    try {
      ConfigObject obj;
      config._get(T::s_class_name, name, obj, rlevel, rclasses);
      return get(config, obj, init_children, init_object);
    }
    catch(daq::config::NotFound & ex) {
      if(!strcmp(ex.get_type(), "class")) {
        std::ostringstream text;
	text << "wrong database schema, cannot find class \"" << ex.get_data() << '\"';
	throw daq::config::Generic(ERS_HERE, text.str().c_str());
      }
      else {
        return 0;
      }
    }
  }
  increment_gets(config);
  return i->second;
}


template<class T>
  bool
  Configuration::is_valid(const T * object) noexcept
  {
    std::lock_guard<std::mutex> scoped_lock(m_tmpl_mutex);

    auto j = m_cache_map.find(&T::s_class_name);

    if (j != m_cache_map.end())
      {
        Cache<T> *c = static_cast<Cache<T>*>(j->second);

        for (const auto& i : c->m_cache)
          {
            if (i->second == object)
              return true;
          }
      }

    return false;
  }


template<class T> void
Configuration::update(const std::vector<std::string>& modified,
                      const std::vector<std::string>& removed,
                      const std::vector<std::string>& created) noexcept
  {
    auto j = m_cache_map.find(&T::s_class_name);

    ERS_DEBUG(4, "call for class \'" << T::s_class_name << '\'');

    if (j != m_cache_map.end())
      {
        Cache<T> *c = static_cast<Cache<T>*>(j->second);
        set_cache_unread(removed, *c);
        set_cache_unread(created, *c);
        set_cache_unread(modified, *c);
      }
  }

template<class T> void
Configuration::_reset_objects() noexcept
  {
    auto j = m_cache_map.find(&T::s_class_name);

    if (j != m_cache_map.end())
      {
        _unread_objects(static_cast<Cache<T>*>(j->second));
      }
  }

template<class T>
  void
  Configuration::_unread_objects(CacheBase* x) noexcept
  {
    Cache<T> *c = static_cast<Cache<T>*>(x);

    for (auto& i : c->m_cache)
      {
        i.second->p_was_read = false;
      }
  }

template<class T> void
Configuration::_rename_object(CacheBase* x, const std::string& old_id, const std::string& new_id) noexcept
{
  Cache<T> *c = static_cast<Cache<T>*>(x);

  // rename template object
  auto it = c->m_cache.find(old_id);
  if (it != c->m_cache.end())
    {
      ERS_DEBUG(3, " * rename \'" << old_id << "\' to \'" << new_id << "\' in class \'" << T::s_class_name << "\')");
      c->m_cache[new_id] = it->second;
      c->m_cache.erase(it);

      std::lock_guard<std::mutex> scoped_lock(it->second->m_mutex);
      it->second->p_UID = new_id;
    }

  // rename generated objects if any
  auto range = c->m_t_cache.equal_range(old_id);
  for (auto it = range.first; it != range.second;)
    {
      T * o = it->second;
      it = c->m_t_cache.erase(it);
      c->m_t_cache.emplace(new_id, o);
    }
}


template<class T> void
Configuration::register_converter(AttributeConverter<T> * object) noexcept
  {
    std::lock_guard<std::mutex> scoped_lock(m_else_mutex);

    std::list<AttributeConverterBase*> * c = m_convert_map[typeid(T).name()];
    if (c == 0)
      {
        c = m_convert_map[typeid(T).name()] = new std::list<AttributeConverterBase*>();
      }

    c->push_back(object);
  }

template<class T>
  void
  Configuration::convert(T& value, const ConfigObject& obj, const std::string& attr_name) noexcept
  {
    config::map<std::list<AttributeConverterBase*> *>::const_iterator l = m_convert_map.find(typeid(T).name());
    if (l != m_convert_map.end())
      {
        for (const auto& i : *l->second)
          {
            static_cast<AttributeConverter<T>*>(i)->convert(value, *this, obj, attr_name);
          }
      }
  }

template<class T>
  void
  Configuration::convert2(std::vector<T>& value, const ConfigObject& obj, const std::string& attr_name) noexcept
  {
    config::map<std::list<AttributeConverterBase*> *>::const_iterator l = m_convert_map.find(typeid(T).name());
    if (l != m_convert_map.end())
      {
        for (auto& j : value)
          {
            for (const auto& i : *l->second)
              {
                static_cast<AttributeConverter<T>*>(i)->convert(j, *this, obj, attr_name);
              }
          }
      }
  }

inline void
CacheBase::increment_gets(::Configuration& db) noexcept
{
  ++db.p_number_of_cache_hits;
}


#endif // CONFIG_CONFIGURATION_H_
