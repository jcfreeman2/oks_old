  /**
   *  \file ConfigurationImpl.h This file contains abstract ConfigurationImpl class,
   *  that is used to implement configuration objects.
   *  \author Igor Soloviev
   *  \brief abstract Configuration implementation
   */

#ifndef CONFIG_CONFIGURATIONIMPL_H_
#define CONFIG_CONFIGURATIONIMPL_H_

#include <string>
#include <vector>
#include <list>
#include <set>
#include <map>

#include "config/map.hpp"
#include "config/set.hpp"
#include "config/ConfigVersion.hpp"

class ConfigurationChange;
class ConfigObject;
class ConfigObjectImpl;

namespace daq {
  namespace config {
    struct class_t;
  }
}


  /**
   * \brief Provides pure virtual interface used by the Configuration class.
   *
   *  The class has several pure virtual methods to manipulate databases,
   *  access database information, subscribe and receive notification on data changes.
   *  Any database implementation inherits from this class and implements it's methods. 

   *  The methods may throw daq::config::Exception exception (like Generic, NotFound) in
   *  case of an error unless \b noexcept is explicitly used in their specification.
   */

class ConfigurationImpl {

  friend class ConfigObject;
  friend class ConfigObjectImpl;
  friend class Configuration;

  public:

      /// The constructor.

    ConfigurationImpl() noexcept;

      /// Virtual destructor.

    virtual ~ConfigurationImpl();


    // methods to open/close database

  public:

      /// Open database implementation in accordance with given name.

    virtual void open_db(const std::string& db_name) = 0;

      /// Close database implementation.

    virtual void close_db() = 0;

      /// Check if a database is loaded.

    virtual bool loaded() const noexcept = 0;

      /// Create database.

    virtual void create(const std::string& db_name, const std::list<std::string>& includes) = 0;

      /// Return write access status.

    virtual bool is_writable(const std::string& db_name) = 0;

      /// Add include file.

    virtual void add_include(const std::string& db_name, const std::string& include)= 0;

      /// Remove include file.

    virtual void remove_include(const std::string& db_name, const std::string& include)= 0;

      /// Get included files.

    virtual void get_includes(const std::string& db_name, std::list<std::string>& includes) const = 0;

      /// Get uncommitted files.

    virtual void get_updated_dbs(std::list<std::string>& dbs) const = 0;

      /// Set commit credentials.

    virtual void set_commit_credentials(const std::string& user, const std::string& password) = 0;

      /// Commit database changes.

    virtual void commit(const std::string& log_message) = 0;

      /// Abort database changes.

    virtual void abort() = 0;

      /// Prefetch all data into client cache

    virtual void prefetch_all_data() = 0;

      /// Get newly available versions

    virtual std::vector<daq::config::Version> get_changes() = 0;

      /// Get archived versions

    virtual std::vector<daq::config::Version> get_versions(const std::string& since, const std::string& until, daq::config::Version::QueryType type, bool skip_irrelevant) = 0;


    // methods to get data from database

  public:

      /// Get object of class by id.

    virtual void get(const std::string& class_name, const std::string& id, ConfigObject& object, unsigned long rlevel, const std::vector<std::string> * rclasses) = 0;

      /// Get objects of class according to query.

    virtual void get(const std::string& class_name, std::vector<ConfigObject>& objects, const std::string& query, unsigned long rlevel, const std::vector<std::string> * rclasses) = 0;

      /// Get objects according to path.

    virtual void get(const ConfigObject& obj_from, const std::string& query, std::vector<ConfigObject>& objects, unsigned long rlevel, const std::vector<std::string> * rclasses) = 0;

      /// Test object existence (used by Python binding)

    virtual bool test_object(const std::string& class_name, const std::string& id, unsigned long rlevel, const std::vector<std::string> * rclasses) = 0;


    // methods to create and destroy objects

  public:

      /// Create object of class by id at given file.

    virtual void create(const std::string& at, const std::string& class_name, const std::string& id, ConfigObject& object) = 0;

      /// Create object of class by id at file identified by object 'at'.

    virtual void create(const ConfigObject& at, const std::string& class_name, const std::string& id, ConfigObject& object) = 0;

      /// Destroy object of class by id.

    virtual void destroy(ConfigObject& object) = 0;


    // get meta-data

  public:

      /// Get description of class in accordance with parameters.

    virtual daq::config::class_t * get(const std::string& class_name, bool direct_only) = 0;

      /// Get inheritance hierarchy

    virtual void get_superclasses(config::fmap<config::fset>& schema) = 0;


    // notification

  public:

      /// Callback to notify database changes

    typedef void (*notify)(std::vector<ConfigurationChange *> & changes, Configuration *);

      /// Callback to pre-notify database changes

    typedef void (*pre_notify)(Configuration *);

      /// Subscribe on database changes

    virtual void subscribe(const std::set<std::string>& class_names, const std::map< std::string, std::set<std::string> >& objs, notify cb, pre_notify pre_cb) = 0;

      /// Remove subscription on database changes

    virtual void unsubscribe() = 0;

      /// Print implementation specific profiling information
    
    virtual void print_profiling_info() noexcept = 0;

      /// Print profiling information about objects in cache

    void print_cache_info() noexcept;


      /// cache of implementation objects (class-name::->object_id->implementation)

  private:

    config::pmap<config::map<ConfigObjectImpl *> * > m_impl_objects;
    std::vector<ConfigObjectImpl *> m_tangled_objects; // deleted and replaced by others as result of rename

    mutable unsigned long p_number_of_cache_hits;
    mutable unsigned long p_number_of_object_read;


  protected:

      /// get object from cache

    ConfigObjectImpl * get_impl_object(const std::string& class_name, const std::string& id) const noexcept;


      /// put object to cache

    void put_impl_object(const std::string& class_name, const std::string& id, ConfigObjectImpl * obj) noexcept;


      /// insert new object (update cache or create-and-insert)

    template<class T, class OBJ>
      T *
      insert_object(OBJ& obj, const std::string& id, const std::string& class_name) noexcept
        {
          ConfigObjectImpl * p = get_impl_object(class_name, id);

          if (p == nullptr)
            {
              p = static_cast<ConfigObjectImpl *>(new T(obj, this));
              put_impl_object(class_name, id, p);
            }
          else
            {
              static_cast<T *>(p)->set(obj);
              p->m_state = daq::config::Valid;
            }

          return static_cast<T *>(p);
        }


      /// clean cache (e.g. to be used by destructor)

    void clean() noexcept;


      /// Configuration pointer is needed for notification on changes, e.g. in case of subscription or an object deletion

  protected:

    Configuration * m_conf;


      /// Is required by reload methods

    std::mutex& get_conf_impl_mutex() const;


  public:

      /// set configuration object

    void set(Configuration * db) noexcept { m_conf = db; }


  public:

      /// rename object in cache

    void rename_impl_object(const std::string * class_name, const std::string& old_id, const std::string& new_id) noexcept;

};


#endif // CONFIG_CONFIGURATIONIMPL_H_
