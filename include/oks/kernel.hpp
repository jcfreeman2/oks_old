/**	
 *  \file oks/kernel.h
 *
 *  This file %is part of the OKS package.
 *  Author: Igor SOLOVIEV "https://phonebook.cern.ch/phonebook/#personDetails/?id=432778"
 *
 *  This file contains the declarations for the OKS kernel.
 */

#ifndef OKS_KERNEL_H
#define OKS_KERNEL_H

#include <sys/time.h>

#include <ctime>
#include <string>
#include <list>
#include <mutex>
#include <shared_mutex>
#include <set>
#include <vector>

#include <boost/shared_ptr.hpp>

#include "oks/defs.hpp"
#include "oks/xml.hpp"
#include "oks/file.hpp"
#include "oks/class.hpp"
#include "oks/object.hpp"
#include "oks/exceptions.hpp"


  //
  // Forward declarations
  //

class	OksMethod;
class	OksProfiler;
class	OksString;
class	OksPipeline;


  /// @addtogroup oks

namespace oks {

    /**
     *  @ingroup oks
     *
     *  \brief Cannot load file.
     *
     *  Such exception %is thrown when OKS cannot load a file
     *  in case if file %is readable but its content %is bad.
     */

  class FailedLoadFile : public exception {

    public:

        /** The constructor gets reason from nested oks exception. **/
      FailedLoadFile(const std::string& item, const std::string& name, const exception& reason) noexcept : exception (fill(item, name, reason.what()), reason.level() + 1) { }

        /** The constructor gets reason from non-oks exception. **/
      FailedLoadFile(const std::string& item, const std::string& name, const std::string& reason) noexcept : exception (fill(item, name, reason), 0) { }

      virtual ~FailedLoadFile() noexcept { }


    private:

      static std::string fill(const std::string& item, const std::string& name, const std::string& reason) noexcept;

  };


    /**
     *  \brief Cannot re-load files.
     *
     *  Such exception %is thrown when OKS cannot re-load files.
     */

  class FailedReloadFile : public exception {

    public:

        /** The constructor gets reason from nested oks exception. **/
      FailedReloadFile(const std::string& names, const exception& reason) noexcept : exception (fill(names, reason.what()), reason.level() + 1) { }

        /** The constructor gets reason from non-oks exception. **/
      FailedReloadFile(const std::string& names, const std::string& reason) noexcept : exception (fill(names, reason), 0) { }

      virtual ~FailedReloadFile() noexcept { }


    private:

      static std::string fill(const std::string& names, const std::string& reason) noexcept;

  };


    /**
     *  \brief Cannot commit, checkout or release files.
     *
     *  Such exception %is thrown when OKS cannot perform repository operation.
     */

  class RepositoryOperationFailed : public exception {

    public:

        /** The constructor gets reason from nested oks exception. **/
      RepositoryOperationFailed(const char * op, const exception& reason) noexcept : exception (fill(op, reason.what()), reason.level() + 1) { }

        /** The constructor gets reason from non-oks exception. **/
      RepositoryOperationFailed(const char * op, const std::string& reason) noexcept : exception (fill(op, reason), 0) { }

      virtual ~RepositoryOperationFailed() noexcept { }


    private:

      static std::string fill(const char * op, const std::string& reason) noexcept;

  };


    /**
     *  \brief Cannot open file.
     *
     *  Such exception %is thrown when OKS cannot find a file provided by user.
     *  It reports test files it tried to open.
     */

  class CanNotOpenFile : public exception {

    public:

      CanNotOpenFile(const char * prefix, const std::string& name, const std::string& reason) noexcept : exception (fill(prefix, name, reason), 0) { }

      virtual ~CanNotOpenFile() noexcept { }


    private:

      static std::string fill(const char * prefix, const std::string& name, const std::string& reason) noexcept;

  };


    /**
     *  \brief Cannot create file.
     *
     *  Such exception %is thrown when OKS cannot create a new file.
     */

  class CanNotCreateFile : public exception {

    public:

      CanNotCreateFile(const char * prefix, const char * item, const std::string& name, const exception& reason) noexcept : exception (fill(prefix, item, name, reason.what()), reason.level() + 1) { }
      CanNotCreateFile(const char * prefix, const char * item, const std::string& name, const std::string& reason) noexcept : exception (fill(prefix, item, name, reason), 0) { }

      virtual ~CanNotCreateFile() noexcept { }


    private:

      static std::string fill(const char * prefix, const char * item, const std::string& name, const std::string& reason) noexcept;

  };


    /**
     *  \brief Cannot create file.
     *
     *  Such exception %is thrown when OKS cannot create a new file.
     */

  class CanNotCreateRepositoryDir : public exception {

    public:

      CanNotCreateRepositoryDir(const char * prefix, const std::string& name) noexcept : exception (fill(prefix, name), 0) { }

      virtual ~CanNotCreateRepositoryDir() noexcept { }


    private:

      static std::string fill(const char * prefix, const std::string& name) noexcept;

  };

    /**
     *  \brief Failed write to file.
     *
     *  Such exception %is thrown when OKS cannot successfully write to a file.
     */

  class CanNotWriteToFile : public exception {

    public:

        /** The constructor gets reason from nested oks exception. **/
      CanNotWriteToFile(const char * prefix, const char * item, const std::string& name, const exception& reason) noexcept : exception (fill(prefix, item, name, reason.what()), reason.level() + 1) {}

        /** The constructor gets reason from non-oks exception. **/
      CanNotWriteToFile(const char * prefix, const char * item, const std::string& name, const std::string& reason) noexcept : exception (fill(prefix, item, name, reason), 0) {}

      virtual ~CanNotWriteToFile() noexcept { }


    private:

      static std::string fill(const char * prefix, const char * item, const std::string& name, const std::string& reason) noexcept;

  };


    /**
     *  \brief Failed backup file.
     *
     *  Such exception %is thrown when OKS cannot successfully write to a backup file.
     */

  class CanNotBackupFile : public exception {

    public:

        /** The constructor gets reason from nested oks exception. **/
      CanNotBackupFile(const std::string& name, const exception& reason) noexcept : exception (fill(name, reason.what()), reason.level() + 1) {}

      virtual ~CanNotBackupFile() noexcept { }


    private:

      static std::string fill(const std::string& name, const std::string& reason) noexcept;

  };


    /**
     *  \brief Failed to set active file.
     *
     *  Such exception %is thrown when OKS cannot successfully set active schema or data file.
     */

  class CanNotSetActiveFile : public exception {

    public:

        /** The constructor gets reason from nested oks exception. **/
      CanNotSetActiveFile(const char * item, const std::string& name, const exception& reason) noexcept : exception (fill(item, name, reason.what()), reason.level() + 1) {}

        /** The constructor gets reason from non-oks exception. **/
      CanNotSetActiveFile(const char * item, const std::string& name, const std::string& reason) noexcept : exception (fill(item, name, reason), 0) {}

      virtual ~CanNotSetActiveFile() noexcept { }


    private:

      static std::string fill(const char * item, const std::string& name, const std::string& reason) noexcept;

  };


    /**
     *  \brief Failed move item to file.
     *
     *  Such exception %is thrown when OKS cannot move class or object to a new file.
     */

  class CanNotSetFile : public exception {

    public:

        /** The constructor gets reason from nested oks exception. **/
      CanNotSetFile(const OksClass * c, const OksObject * o, const OksFile& file, const exception& reason) noexcept : exception (fill(c, o, file, reason.what()), reason.level() + 1) {}

        /** The constructor gets reason from non-oks exception. **/
      CanNotSetFile(const OksClass * c, const OksObject * o, const OksFile& file, const std::string& reason) noexcept : exception (fill(c, o, file, reason), 0) {}

      virtual ~CanNotSetFile() noexcept { }


    private:

      static std::string fill(const OksClass * c, const OksObject * o, const OksFile& file, const std::string& reason) noexcept;

  };


    /**
     *  \brief Failed add new class.
     *
     *  Such exception %is thrown when OKS cannot add new class.
     */

  class CannotAddClass : public exception {

    public:

        /** The constructor gets reason from nested oks exception. **/
      CannotAddClass(const OksClass &c, const exception& reason) noexcept : exception (fill(c, reason.what()), reason.level() + 1) {}

        /** The constructor gets reason from non-oks exception. **/
      CannotAddClass(const OksClass &c, const std::string& reason) noexcept : exception (fill(c, reason), 0) {}

      virtual ~CannotAddClass() noexcept { }

    private:

      static std::string fill(const OksClass &c, const std::string& reason) noexcept;

  };
  
  
    /**
     *  \brief Failed to authenticate user.
     *
     *  Such exception %is thrown when PAM authentication failed.
     */

  class AuthenticationFailure : public exception {

    public:

        /** The constructor gets reason from non-oks exception. **/
      AuthenticationFailure(const std::string& reason) noexcept : exception (reason, 0) {}

      virtual ~AuthenticationFailure() noexcept { }

  };


    /**
     *  \brief Failed resolve path.
     *
     *  Such exception %is thrown when OKS cannot read path.
     */

  class CannotResolvePath : public exception {

    public:

        /** The constructor gets reason from non-oks exception. **/
      CannotResolvePath(const std::string& path, int error_code) noexcept : exception (fill(path, error_code), 0) {}

      virtual ~CannotResolvePath() noexcept { }

    private:

      static std::string fill(const std::string& path, int error_code) noexcept;

  };

  struct LoadErrors
  {
    public:

      void add_error(const OksFile& file, std::exception& ex);
      bool is_empty() { std::lock_guard lock(p_mutex); return m_errors.empty(); }
      void clear() { std::lock_guard lock(p_mutex); m_errors.clear(); }
      std::string get_text();

    private:

      std::list<std::string> m_errors;
      std::string m_error_string;
      std::mutex p_mutex;

      void add_parents(std::string& text, const OksFile * file, std::set<const OksFile *>& parents);

  };

  double get_time_interval(const timeval * t1, const timeval * t2);

  enum __LogSeverity__
  {
    Error = 0,
    Warning,
    Log,
    Debug
  };

  std::ostream&
  log_timestamp(__LogSeverity__ severity = Log);

}


  /**
   * @ingroup oks
   *
   *  \brief The struct OksNameTable %is used to generate unique strings
   *
   * The generated string contains ASCII characters
   * (allowed characters defined by string 'symbols')
   *
   * Each generated string %is short as possible
   *
   * Calls of get will give the following sequency:
   *  "0",  "1",  ..., "9",  "a",  "b",  ..., "z",  "A",  ..., "?",
   *  "00", "01", ..., "09", "0a", "0b", ..., "0Z", "0A", ..., "0?",
   *  "10", "11", ..., "19", "1a", ... etc.
   */

struct OksNameTable {

    OksNameTable() {reset();}

    OksString * get();
    void reset() {count = -1;}


  private:

    static const char	symbols[];
    long		count;
};


  /**
   * @ingroup oks
   *
   *  \brief The struct OksAliasTable %is used to support aliases
   *
   * The technique of aliases %is used to reduce size of OKS data files:
   * if a class name appears first time somewhere in OKS data file
   * it %is marked in front by '@' symbol and the alias to it %is used
   * later, e.g.:
   * 	- first object stored in data file %is "Detector@First"
   * 	  and it will be stored as "@Detector@First"
   * 	- second object stored in data file %is "Detector@Second"
   * 	  and it will be stored as "0@Second"
   * 	- in this case "0" %is alias for "Detector"
   * 
   * The technique of aliases %is used to keep compatibility with old data
   * format: new method to read OKS data files may read new and old
   * format at the same time
   * 
   */

struct OksAliasTable {

    ~OksAliasTable	();


      // struct Value stores pointer to class (if loaded) or pointer to its name

    struct Value {

      struct SortByName {
        bool operator() (const OksString * s1, const OksString * s2) const {
          return *s1 < *s2;
        }
      };

      typedef std::map<const OksString *, Value *, SortByName> Map;


      Value		(OksString * n, const OksClass * p) : class_name (n), class_ptr(p) {;}

      OksString *	class_name;
      const OksClass *	class_ptr;
    };

    inline void add_key(OksString *);				// class is not loaded
    inline void add_value(OksString * vn, const OksClass * vp);	// class was loaded
    inline const Value * get(const OksString * key) const;
    const Value * get(const char * key) const { OksString str(key); return get(&str); }


  private:

    Value::Map		p_aliases;
    OksNameTable	p_nameTable;

};


  /**
   *  Inline methods for struct OksAliasTable
   */

inline void
OksAliasTable::add_key(OksString * key)
{
  p_aliases[key] = new Value(p_nameTable.get(), 0);
}

inline void
OksAliasTable::add_value(OksString * vn, const OksClass * vp)
{
  p_aliases[p_nameTable.get()] = new Value(vn, vp);
}

inline const OksAliasTable::Value *
OksAliasTable::get(const OksString * key) const
{
  Value::Map::const_iterator i = p_aliases.find(key);

  if(i != p_aliases.end()) return i->second;
  else return 0;
}

struct OksRepositoryVersion
{
  std::string m_commit_hash;
  std::string m_user;
  std::time_t m_date;
  std::string m_comment;
  std::vector<std::string> m_files;

  void clear()
  {
    m_commit_hash.clear();
    m_user.clear();
    m_date = 0;
    m_comment.clear();
    m_files.clear();
  }
};

  /**
   *  @ingroup oks
   *
   *  \brief Provides interface to the OKS kernel.
   *
   *  It %is responsible for loading OKS data and schema files and
   *  it provides access to loaded OKS classes and their objects.
   *
   *  To work with OKS schema the following base methods are available:
   *   - load_schema() - load OKS schema from file (i.e. read description of classes with attributes, relationships and methods)
   *   - new_schema() - create new OKS schema file
   *   - set_active_schema() - use this schema file for newly created classes
   *   - save_schema() - save OKS schema file
   *   - close_schema() - close OKS schema file and unload all classes from it
   *   - classes() - get list of OKS classes
   *   - find_class() - get class by name
   *
   *  To work with OKS data the following base methods are available:
   *   - load_data() - load OKS data from file (i.e. read description of OKS objects)
   *   - reload_data() - reload data file, e.g. to revert to saved data or when it was modified by external process
   *   - new_data() - create new OKS data file
   *   - set_active_data() - use this data file for newly created classes
   *   - save_data() - save OKS data file
   *   - close_data() - close OKS data file and unload all objects from it
   *   - objects() - get all objects (unsorted by classes or IDs)
   *
   *  To work with OKS server the following methods can be used:
   *   - commit_repository() - commit changes from user repository files on the OKS server
   *   - update_repository() - update files on user repository from OKS server
   *
   *  When schema or data are modified, the process can give callback using the following methods:
   *   - subscribe_create_class() - notify, when new class %is created
   *   - subscribe_change_class() - notify, when existing class %is modified
   *   - subscribe_delete_class() - notify, when existing class %is removed
   *   - subscribe_create_object() - notify, when new object %is created
   *   - subscribe_change_object() - notify, when existing object %is modified
   *   - subscribe_delete_object() - notify, when existing object  %is removed
   *
   *  When objects are created in unordered manner or by efficiency reasons files are loaded with non-bind option,
   *  the objects can be bind (i.e. linked) using the following methods:
   *   - bind_objects() - link relationships of existing objects
   *   - get_bind_objects_status() - get status of last bind_objects() operation
   *
   *  In case of problems most of above methods throw exceptions derived from oks::exception class.
   *
   *  Most of the OksKernel methods are thread-safe. Those which are not thread-safe, are started from prefix "k_".
   *
   */

class OksKernel
{
  friend std::ostream& operator<<(std::ostream&, OksKernel&);
  friend class OksFile;
  friend class OksClass;
  friend class OksObject;
  friend struct OksLoadObjectsJob;
  friend struct OksData;


  public:

      /**
       *  \brief Constructor to build OksKernel object.
       *
       *  All parameters of the constructor are optional.
       *    \param silence_mode   - defines kernel silence mode
       *    \param verbose_mode   - defines kernel verbose mode
       *    \param profiling_mode - switch kernel into profiling mode
       */

    OksKernel(bool silence_mode = false, bool verbose_mode = false, bool profiling_mode = false, bool allow_repository = true, const char * version = nullptr, std::string branch_name = "");


      /**
       *  \brief Copy constructor.
       *
       *  It is used by RDB RW server to make fast copy.
       *
       *  \param src   - kernel to copy
       */

    OksKernel(const OksKernel& src, bool copy_repository = false);

    ~OksKernel();

    OksProfiler * GetOksProfiler() const {return profiler;}


      /**
       *  \brief Get OKS version.
       *  The method returns string containing CVS tag and date of OKS build.
       */

    static const char * GetVersion();


      /**
       *  \brief Get hostname of given process.
       */

    static std::string& get_host_name();


      /**
       *  \brief Get domain name of host.
      */

    static std::string& get_domain_name();


      /**
       *  \brief Get username of given process.
       */

    static std::string& get_user_name();


    /*   /\** */
    /*    *  \brief Validate user credentials (used by rdb and oksconfig). */
    /*    * */
    /*    *  PAM authentication is used. */
    /*    * */
    /*    *  \param user    - user name */
    /*    *  \param passwd  - user password */
    /*    * */
    /*    *  \throw Throw std::exception in case of problems. */
    /*    *\/ */

    /* static void validate_credentials(const char * user, const char * passwd); */


      /**
       *  \brief Get status of verbose mode.
       *  The method returns true, if the verbose mode %is switched 'On'.
       */

    bool get_verbose_mode() const {return p_verbose;}


      /**
       *  \brief Set status of verbose mode.
       *  To switch 'On'/'Off' use the method's parameter:
       *    \param b  - set 'true' to switch 'On' or 'false' to switch 'Off'.
       * 
       *  The verbose mode can also be switched 'On' using the "OKS_KERNEL_VERBOSE"
       *  environment variable set to any value except 'no'.
       */

    void set_verbose_mode(const bool b) {p_verbose = b;}


      /**
       *  \brief Get status of silence mode.
       *  The method returns true, if the silence mode %is switched 'On'.
       *  In such case the OKS will not print any info, warnings and error messages.
       */

    bool get_silence_mode() const {return p_silence;}


      /**
       *  \brief Set status of silence mode.
       *  To switch 'On'/'Off' use the method's parameter:
       *    \param b  - set 'true' to switch 'On' or 'false' to switch 'Off'.
       * 
       *  The silence mode can also be switched 'On' using the "OKS_KERNEL_SILENCE"
       *  environment variable set to any value except 'no'.
       */

    void set_silence_mode(const bool b) {p_silence = b;}


      /**
       *  \brief Get status of profiling mode.
       *  The method returns true, if the profiling mode %is switched 'On'.
       *  In such case the OKS will report time spend to execute main methods and
       *  a summary table of their calls on the OKS kernel destroy.
       */

    bool get_profiling_mode() const {return p_profiling;}


      /**
       *  \brief Set status of profiling mode.
       *  To switch 'On'/'Off' use the method's parameter:
       *    \param b  - set 'true' to switch 'On' or 'false' to switch 'Off'.
       * 
       *  The profiling mode can also be switched 'On' using the "OKS_KERNEL_PROFILING"
       *  environment variable set to any value except 'no'.
       */

    void set_profiling_mode(const bool b);


      /**
       *  \brief Get status of duplicated classes mode.
       *  The method returns true, if the duplicated classes mode %is switched 'On'.
       *  In such case the OKS will allow to load classes with equal names.
       *  Only first loaded class is taken into account, the next ones are ignored.
       */

    bool get_allow_duplicated_classes_mode() const {return p_allow_duplicated_classes;}



      /**
       *  \brief Get status of duplicated objects mode.
       *  The method returns true, if the duplicated objects mode %is switched 'On'.
       *  In such case the OKS will allow to load objects of the same class with equal IDs.
       *  The ID for duplicated object will be generated automatically.
       */

    bool get_allow_duplicated_objects_mode() const {return p_allow_duplicated_objects;}


      /**
       *  \brief Set status of test inherited duplicated objects mode.
       *  To switch 'On'/'Off' use the method's parameter:
       *    \param b  - set 'true' to switch 'On' or 'false' to switch 'Off'.
       * 
       *  When the mode %is switched 'On', the OKS kernel does not allow objects
       *  with equal IDs withing the same class inheritance hierarchy.
       * 
       *  The test inherited duplicated objects mode can also be switched 'On' using the "OKS_KERNEL_TEST_DUPLICATED_OBJECTS_VIA_INHERITANCE"
       *  environment variable set to any value except 'no'.
       */

    void set_test_duplicated_objects_via_inheritance_mode(const bool b) {p_test_duplicated_objects_via_inheritance = b;}


      /**
       *  \brief Get status of test inherited duplicated objects mode.
       *  The method returns true, if the mode %is switched 'On'.
       *  In such case the OKS will throw exception when there are objects with equal IDs within class inheritance hierarchy.
       */

    bool get_test_duplicated_objects_via_inheritance_mode() const {return p_test_duplicated_objects_via_inheritance;}


      /**
       *  \brief Set status of duplicated classes mode.
       *  To switch 'On'/'Off' use the method's parameter:
       *    \param b  - set 'true' to switch 'On' or 'false' to switch 'Off'.
       *
       *  The duplicated classes mode can also be switched 'On' using the "OKS_KERNEL_ALLOW_DUPLICATED_CLASSES"
       *  environment variable set to any value except 'no'.
       */

    void set_allow_duplicated_classes_mode(const bool b) {p_allow_duplicated_classes = b;}


      /**
       *  \brief Set status of duplicated objects mode.
       *  To switch 'On'/'Off' use the method's parameter:
       *    \param b  - set 'true' to switch 'On' or 'false' to switch 'Off'.
       * 
       *  The duplicated objects mode can also be switched 'On' using the "OKS_KERNEL_ALLOW_DUPLICATED_OBJECTS"
       *  environment variable set to any value except 'no'.
       */

    void set_allow_duplicated_objects_mode(const bool b) {p_allow_duplicated_objects = b;}


      /**
       *  \brief Get status of string range validator.
       *
       *  The method returns true, if the check %is switched 'Off'. If the check %is switched 'On',
       *  OKS will validate string value using regexp regular expression, if it is defined for related attribute.
       */

    static bool get_skip_string_range() {return p_skip_string_range;}


      /**
       *  \brief Set status of string range validator.
       *  To switch 'On'/'Off' use the method's parameter:
       *    \param b  - set 'true' to switch 'On' (i.e. to bypass the test) or 'false' to switch 'Off' (i.e. to force the test).
       *
       *  The skip max length check mode can also be switched 'On' using the "OKS_KERNEL_SKIP_STRING_RANGE"
       *  environment variable set to any value except 'no'.
       */

    static void set_skip_string_range(const bool b) {p_skip_string_range = b;}

      /**
       *  \brief Return OKS kernel mutex.
       * 
       *  The mutex %is used to lock most OKS kernel update operations like loading of files.
       *  It %is used by the RDB server performing OKS operations in multi-threaded environment.
       */

    std::shared_mutex& get_mutex() {return p_kernel_mutex;}


     //
     //  Methods to work with OKS database files
     //

      /**
       *  \brief Finds OKS schema file.
       *
       *  Method returns pointer to the OKS schema file with name s
       *  or 0, if there %is no such schema file found.
       *
       *  The user should acquire a lock on OKS kernel before calling
       *  such method in the multi-threaded environment.
       */

    OksFile * find_schema_file(const std::string & s) const;


      /**
       *  \brief Finds OKS data file.
       *
       *  Method returns pointer to the OKS data file with name s
       *  or 0, if there %is no such data file found.
       *
       *  The user should acquire a lock on OKS kernel before calling
       *  such method in the multi-threaded environment.
       */

    OksFile * find_data_file(const std::string & s) const;


      /**
       *  \brief Creates list of classes which belong to given file.
       *
       *  Method returns pointer to the list of OKS classes or 0 if the
       *  file contains no classes. The user %is responsible to destroy
       *  the list after usage.
       *
       *  The user should acquire a lock on OKS kernel before calling
       *  such method in the multi-threaded environment.
       */

    std::list<OksClass *> * create_list_of_schema_classes(OksFile *) const;


      /**
       *  \brief Creates list of objects which belong to given file.
       *
       *  Method returns pointer to the list of OKS objects or 0 if the
       *  file contains no objects. The user %is responsible to destroy
       *  the list after usage.
       *
       *  The user should acquire a lock on OKS kernel before calling
       *  such method in the multi-threaded environment.
       */

    std::list<OksObject *> * create_list_of_data_objects(OksFile *) const;


      /**
       *  \brief Creates OKS file descriptor.
       *
       *  Method returns pointer to the OKS file descriptor defined by the
       *  file_name parameter or 0 if such file does not exist. The user %is
       *  responsible to destroy the descriptor after usage.
       *  \param short_file_name  short name of the file (e.g. relative to OKS root)
       *  \param file_name        an absolute file name
       *
       *  The method %is thread-safe.
       */

    OksFile * create_file_info(const std::string& short_file_name, const std::string& file_name);


      /**
       *  \brief Check if the OKS file %is read-only.
       *
       *  Method returns true if the file %is read-only and false if it %is not.
       *  The method also changes the read-only flag in the file descriptor.
       *
       *  The method %is thread-safe.
       */

    static bool check_read_only(OksFile * f);


      /**
       *  \brief Calculates full path to file.
       *
       *  Method calculates full path to file. It takes into account the values of the OKS_DB_ROOT,
       *  TDAQ_DB_REPOSITORY, TDAQ_DB_USER_REPOSITORY and TDAQ_DB_PATH environment variables and
       *  optionally checks if the path %is relative to the parent file defined by the parent_file parameter.
       *
       *  The method %is thread-safe.
       *
       *  \param path short, relative or absolute path to file
       *  \param parent_file pointer to file including given one
       *
       *  \throw Throw std::exception in case of problems.
       */

    std::string get_file_path(const std::string& path, const OksFile * parent_file = 0, bool strict_paths = true) const;


      /**
       *  \brief Get OKS repository root.
       *
       *  The repository root %is defined by the TDAQ_DB_REPOSITORY environment variable.
       *  The method returns an empty string, if the variable %is not set or its value %is empty.
       */

    static const std::string& get_repository_root();

    const std::string&
    get_repository_version()
    {
      return p_repository_version;
    }

    bool
    is_user_repository_created() const
    {
      return p_user_repository_root_created;
    }

      /**
       *  \brief Get OKS repository name.
       *
       *  The repository name %is defined by last directory of the TDAQ_DB_REPOSITORY environment variable, e.g.:
       *  for the name of oks-repository-root="/usr/local/databases/v10" %is "v10".
       *  The method returns an empty string, if the oks-repository-root %is not set or its value contains no name.
       */

    static const std::string& get_repository_mapping_dir();


      /**
       *  \brief Get user OKS repository root.
       *
       *  The user repository root %is defined by the TDAQ_DB_USER_REPOSITORY environment variable.
       *  The method returns an empty string, if the variable %is not set or its value %is empty.
       */

    const std::string& get_user_repository_root() const;


      /**
       *  \brief Set user OKS repository root.
       *
       *  Used by OKS and RDB tools only!
       */

    void set_user_repository_root(const std::string& path, const std::string& version = "");


      /**
       *  \brief Generates temporal file name.
       *
       *  Method generates temporal file name by adding numeric suffix to the file_name
       *  and testing the file existence.
       *
       *  \return Return name of temporal file.
       *
       *  The method %is not thread-safe.
       */

    static std::string get_tmp_file(const std::string& file_name);


      /**
       *  \brief Opens file and reads its shallow includes.
       *
       *  The method %is thread-safe.
       *
       *  \throw Throw std::exception in case of problems.
       *
       */

    void get_includes(const std::string& file_name, std::set< std::string >& includes, bool use_repository_name = false);
    
    
      /**
       *  \brief Close files which lost their parent.
       *
       *  When a file is closed or an include is removed, some files may lost their parent and needs to be closed.
       *
       *  The method %is not thread-safe.
       */

    void k_close_dangling_includes();


      /**
       *  \brief Load OKS database file.
       *
       *  The method loads OKS schema or data xml file.
       *  It parses the file's xml header to detect the file type and calls appropriate load_schema() or load_data() method.
       *
       *  The method %is thread-safe. The user may not have the OKS kernel lock set in the thread which calls this method.
       *
       *  The parameters of the method are:
       *  \param name    name of the file to be loaded
       *  \param bind    if true, bind objects after load (applicable to data files only)
       *
       *  \return Return pointer to the OKS file descriptor.
       *
       *  \throw Throw oks::exception in case of problems.
       */

    OksFile * load_file(const std::string& name, bool bind = true);


      /**
       *  \brief Load OKS schema file.
       *
       *  The method loads OKS schema file.
       *
       *  The method %is thread-safe. The user must not have the OKS kernel lock set in the thread which calls this method.
       *
       *  The method parameters are:
       *  \param name    name of the file to be loaded
       *  \param parent  descriptor of the parent file (if defined, the name can be relative to the parent file)
       *
       *  \return Return pointer to the OKS file descriptor.
       *
       *  \throw Throw oks::exception in case of problems.
       */

    OksFile * load_schema(const std::string& name, const OksFile * parent = 0);


      /**
       *  \brief Create OKS schema file.
       *
       *  The method creates new OKS schema file and makes it active.
       *
       *  The method %is thread-safe. The user must not have the OKS kernel lock set in the thread which calls this method.
       *
       *  \param name    name of the new schema file
       *
       *  \return Return pointer to the OKS file descriptor
       *
       *  \throw Throw oks::exception in case of problems.
       */

    OksFile * new_schema(const std::string& name);


      /**
       *  \brief Save OKS schema file.
       *
       *  The method saves given OKS schema file.
       *
       *  The method %is thread-safe. The user must not have the OKS kernel lock set in the thread which calls this method.
       *
       *  \param file_h         a pointer to the OKS schema file descriptor returned by a kernel method
       *  \param force          if true, ignore problems if possible (e.g. dangling references)
       *  \param true_file_h    a pointer to the real OKS data file descriptor owning classes (used for backup)
       *
       *  \throw Throw oks::exception in case of problems.
       */

    void save_schema(OksFile * file_h, bool force = false, OksFile * true_file_h = 0);


      /**
       *  \brief Save classes into given OKS schema file.
       *
       *  The method saves explicit set of classes into given OKS schema file.
       *  This %is used to merge schema files by RDB server.
       *
       *  The method %is thread-safe. The user must not have the OKS kernel lock set in the thread which calls this method.
       *
       *  \param file_h     a pointer to an external OKS schema file descriptor
       *  \param force      if true, ignore problems if possible (e.g. dangling references)
       *  \param classes    set of classes to be saved
       *
       *  \throw Throw oks::exception in case of problems.
       */

    void save_schema(OksFile * file_h, bool force, const OksClass::Map& classes);


      /**
       *  \brief Backup OKS schema file.
       *
       *  The method makes a backup of given OKS schema file.
       *  The save operation %is silent and ignores any consistency rules.
       *
       *  The method %is thread-safe. The user must not have the OKS kernel lock set in the thread which calls this method.
       *
       *  \param file_h              a pointer to the OKS data file descriptor
       *  \param suffix              a suffix to be added to the name of file
       *
       *  \throw Throw oks::exception in case of problems.
       */

    void backup_schema(OksFile * pf, const char * suffix = ".bak");


      /**
       *  \brief Save OKS schema file under new name.
       *
       *  The method changes the name of given OKS schema file and saves it.
       *
       *  The method %is thread-safe. The user must not have the OKS kernel lock set in the thread which calls this method.
       *
       *  \param name     new name of the schema file
       *  \param file_h   a pointer to the OKS schema file descriptor returned by a kernel method
       *
       *  \throw Throw oks::exception in case of problems.
       */

    void save_as_schema(const std::string& name, OksFile * file_h);


      /**
       *  \brief Save all OKS schema files.
       *
       *  The method %is thread-safe. The user must not have the OKS kernel lock set in the thread which calls this method.
       *
       *  The method saves all OKS schema files which were created or loaded by the OKS kernel.
       *
       *  \throw Throw oks::exception in case of problems.
       */

    void save_all_schema();


      /**
       *  \brief Close OKS schema file.
       *
       *  The method closes given OKS schema file.
       *
       *  The method %is thread-safe. The user must not have the OKS kernel lock set in the thread which calls this method.
       *
       *  \param file_h   a pointer to the OKS schema file descriptor returned by a kernel method
       */

    void close_schema(OksFile * file_h);


      /**
       *  \brief Close all OKS schema files.
       *
       *  The method closes all OKS schema files which were created or loaded by the OKS kernel.
       *
       *  The method %is thread-safe. The user must not have the OKS kernel lock set in the thread which calls this method.
       */

    void close_all_schema();


      /**
       *  \brief Set active OKS schema file.
       *
       *  The method makes given OKS schema file active.
       *  Any created class will go to the active schema file.
       *
       *  The method %is thread-safe. The user must not have the OKS kernel lock set in the thread which calls this method.
       *
       *  \param file_h   a pointer to the OKS schema file descriptor returned by a kernel method
       *
       *  \throw Throw oks::exception in case of problems.
       */

    void set_active_schema(OksFile * file_h);


      /**
       *  \brief Set active OKS schema file.
       *  Non thread-safe version of the set_active_schema() method;
       *  \throw Throw oks::exception in case of problems.
       */

    void k_set_active_schema(OksFile * file_h);


      /**
       *  \brief Get active OKS schema file.
       *
       *  The method returns active schema file.
       *
       *  \return Return pointer to the active OKS schema file descriptor or 0 if there %is no an active schema.
       *
       *  The method %is thread-safe.
       */

    OksFile * get_active_schema() const {return p_active_schema;}


      /**
       *  \brief Get all schema files.
       *
       *  The method returns all schema files which were created or loaded by the OKS kernel.
       *
       *  \return Return const reference on map of the OKS schema file descriptors.
       *
       *  The method %is thread-safe.
       */

    const OksFile::Map & schema_files() const {return p_schema_files;}


      /**
       *  \brief Get modified schema files.
       *
       *  The method returns all schema files which were modified or removed by an external process.
       *  The method checks file system status of all OKS schema files loaded and created by the
       *  OKS kernel. The file %is considered to be updated if it was modified after it was last
       *  time saved by the OKS kernel, or modified after it was loaded or created.
       *
       *  The method return parameters are:
       *  \param updated   pointer to the list of modified schema files
       *  \param removed   pointer to the list of removed schema files
       *
       *  The user %is responsible to delete returned pointers after usage.
       *
       *  The method %is thread-safe. The user must not have the OKS kernel lock set
       *  in the thread which calls this method.
       */

    void create_lists_of_updated_schema_files(std::list<OksFile *> ** updated, std::list<OksFile *> ** removed) const;


      /**
       *  \brief Get repository modified schema files.
       *
       *  The method returns those schema files which corresponding repository files were modified or removed by an external process.
       *  The method checks file system status of all OKS schema files loaded and created by the
       *  OKS kernel. The file %is considered to be updated if it was modified after it was last
       *  time saved by the OKS kernel, or modified after it was loaded or created.
       *
       *  The method return parameters are:
       *  \param updated   pointer to the list of modified schema files
       *  \param removed   pointer to the list of removed schema files
       *
       *  The user %is responsible to delete returned pointers after usage.
       *
       *  The method %is thread-safe. The user must not have the OKS kernel lock set
       *  in the thread which calls this method.
       */

    void get_updated_repository_files(std::set<std::string>& updated, std::set<std::string>& added, std::set<std::string>& removed);


      /**
       *  \brief Load OKS data file.
       *
       *  The method loads OKS data file.
       *
       *  The method %is thread-safe. The user must not have the OKS kernel lock set in the thread which calls this method.
       *
       *  By default, after the the data file is loaded, the kernel tries to resolve links between objects.
       *  This can be rather cpu consuming operation and it is not scalable for sequential load of many files.
       *  In this case it is recommended to change the default \b bind parameter to false for all but last loading data file.
       *
       *  \param name    name of the file to be loaded
       *  \param bind    if true (by default), resolve links between objects
       *
       *  \return Return pointer to the OKS file descriptor.
       *
       *  \throw Throw oks::exception in case of problems.
       */

    OksFile * load_data(const std::string& name, bool bind = true);


      /**
       *  \brief Reload OKS data files.
       *
       *  The method reloads data files. The non-modified objects are not changed in-memory.
       *  The objects which were removed in the file are removed in-memory. New objects 
       *  created in the file are created in-memory. The values of an object' relationships
       *  and attributes changed in the file are changed in memory, the address of such object
       *  in-memory %is not changed.
       *
       *  The method %is thread-safe. The user must not have the OKS kernel lock set in the thread which calls this method.
       *
       *  The method invokes notification on changes if there %is an appropriate subscription.
       *
       *  The method parameters are:
       *  \param files                     set of pointer to the OKS data file descriptors returned by a kernel method
       *  \param allow_schema_extension    if true, new schema files can be included in modified data files
       *
       *  \throw Throw oks::exception in case of problems.
       */

    void reload_data(std::set<OksFile *>& files, bool allow_schema_extension = true);


      /**
       *  \brief Create OKS data file.
       *
       *  The method creates new OKS data file and makes it active.
       *
       *  The method %is thread-safe. The user must not have the OKS kernel lock set in the thread which calls this method.
       *
       *  The method parameters are:
       *  \param name          name of the new data file
       *  \param logical_name  user-defined logical name of the file (any string passed to the OKS file xml header)
       *  \param type          user-defined type of the file (any string passed to the OKS file xml header)
       *
       *  \return Return pointer to the OKS file descriptor.
       *
       *  \throw Throw oks::exception in case of problems.
       */

    OksFile * new_data(const std::string& name, const std::string& logical_name = "", const std::string& type = "");


      /**
       *  \brief Save OKS data file.
       *
       *  The method saves given OKS data file. By default the format of data file %is
       *  compact. To save the file in the extended format set parameter extended_format to true.
       *
       *  The method %is thread-safe. The user must not have the OKS kernel lock set in the thread which calls this method.
       *
       *  \param file_h              a pointer to the OKS data file descriptor
       *  \param ignore_bad_objects  save the file if it has inconsistent objects or misses includes
       *  \param true_file_h         a pointer to the real OKS data file descriptor owning objects (used for backup)
       *
       *  \throw Throw oks::exception in case of problems.
       */

    void save_data(OksFile * file_h, bool ignore_bad_objects = false, OksFile * true_file_h = nullptr, bool force_defaults = false);


      /**
       *  \brief Save objects into given OKS data file.
       *
       *  The method saves explicitly mentioned objects into given OKS data file.
       *
       *  The method %is thread-safe. The user must not have the OKS kernel lock set in the thread which calls this method.
       *
       *  \param file_h      a pointer to an external OKS data file descriptor
       *  \param objects     set of objects to be saved into this data file
       *
       *  \throw Throw oks::exception in case of problems.
       */

    void save_data(OksFile * file_h, const OksObject::FSet& objects);


      /**
       *  \brief Backup OKS data file.
       *
       *  The method makes a backup of given OKS data file.
       *  The save operation %is silent and ignores any consistency rules.
       *  The file %is always saved in extended format.
       *
       *  The method %is thread-safe. The user must not have the OKS kernel lock set in the thread which calls this method.
       *
       *  \param file_h      a pointer to the OKS data file descriptor
       *  \param suffix      a suffix to be added to the name of file
       *
       *  \throw Throw oks::exception in case of problems.
       */

    void backup_data(OksFile * pf, const char * suffix = ".bak");


      /**
       *  \brief Save OKS data file under new name.
       *
       *  The method changes the name of given OKS data file and saves it.
       *
       *  The method %is thread-safe. The user must not have the OKS kernel lock set in the thread which calls this method.
       *
       *  \param new_name         new name of the data file
       *  \param file_h           a pointer to the OKS data file descriptor returned by a kernel method
       *
       *  \throw Throw oks::exception in case of problems.
       */

    void save_as_data(const std::string& new_name, OksFile * file_h);


      /**
       *  \brief Save all OKS data files.
       *
       *  The method saves all OKS data files which were created or loaded by the OKS kernel.
       *  The files are saved in the same format (extended or compact), in which they
       *  were saved last time.
       *
       *  The method %is thread-safe. The user must not have the OKS kernel lock set in the thread which calls this method.
       *
       *  \throw Throw oks::exception in case of problems.
       */

    void save_all_data();


      /**
       *  \brief Close OKS data file.
       *
       *  The method closes given OKS data file. All objects belonging to this data file will
       *  be destroyed in-memory. The relationships values of objects from others data files
       *  loaded in-memory will be converted from OksObject pointer to UID for all objects
       *  of closing data file, if the unbind_objects parameter %is true. If it %is necessary
       *  to close all loaded data files, it %is recommended to use close_all_data() method
       *  and do not use close_data(..., true) method sequentially by performance reasons.
       *
       *  The method %is thread-safe. The user must not have the OKS kernel lock set in the thread which calls this method.
       *
       *  \param file_h           a pointer to the OKS schema file descriptor returned by a kernel method
       *  \param unbind_objects   convert values of relationships pointing to objects of closing data file from OksObject pointer to the UID
       */

    void close_data(OksFile * file_h, bool unbind_objects = true);


      /**
       *  \brief Close all OKS data files.
       *
       *  The method closes all OKS data files which were created or loaded by the OKS kernel.
       *
       *  The method %is thread-safe. The user must not have the OKS kernel lock set in the thread which calls this method.
       */

    void close_all_data();


      /**
       *  \brief Set active OKS data file.
       *
       *  The method makes given OKS data file active.
       *  Any created object will go to the active data file.
       *
       *  The method %is thread-safe. The user must not have the OKS kernel lock set in the thread which calls this method.
       *
       *  \param file_h   a pointer to the OKS data file descriptor returned by a kernel method
       *
       *  \throw Throw oks::exception in case of problems.
       */

    void set_active_data(OksFile * file_h);


      /**
       *  \brief Set active OKS data file.
       *
       *  Non thread-safe version of the set_active_data() method;
       *
       *  \throw Throw oks::exception in case of problems.
       */

    void k_set_active_data(OksFile *);


      /**
       *  \brief Get active OKS data file.
       *
       *  The method %is thread-safe.
       *
       *  \return Return pointer to the active OKS data file descriptor or 0 if there %is no an active data file.
       */

    OksFile * get_active_data() const {return p_active_data;}


      /**
       *  \brief Get all data files.
       *
       *  Get all data files which were created or loaded by the OKS kernel.
       *  The method %is thread-safe.
       *
       *  \return Return map of the OKS data file descriptors (sorted by file names).
       */

    const OksFile::Map & data_files() const {return p_data_files;}


      /**
       *  \brief Get modified data files.
       *
       *  The method returns all data files which were modified or removed by an external process.
       *  The method checks file system status of all OKS data files loaded and created by the
       *  OKS kernel. The file %is considered to be updated if it was modified after it was last
       *  time saved by the OKS kernel, or modified after it was loaded or created.
       *
       *  The method return parameters are:
       *  \param updated   pointer to the list of modified data files
       *  \param removed   pointer to the list of removed data files
       *
       *  The user %is responsible to delete returned pointers after usage.
       *
       *  The method %is thread-safe. The user must not have the OKS kernel lock set
       *  in the thread which calls this method.
       */

    void create_lists_of_updated_data_files(std::list<OksFile *> ** updated, std::list<OksFile *> ** removed) const;


      /**
       *  \brief Get modified data files.
       *
       *  The method returns the data files modified or removed by an external process in file system or repository.
       *
       *  The method return parameters are:
       *  \param mfs       set of modified data files
       *  \param rfs       set of removed data files
       *  \param version   repository version to compare with (if needed)
       *
       *  The user %is responsible to delete returned pointers after usage.
       *
       *  The method %is thread-safe. The user must not have the OKS kernel lock set
       *  in the thread which calls this method.
       */

    void
    get_modified_files(std::set<OksFile *>& mfs, std::set<OksFile *>& rfs, const std::string& version);


      /**
       *  \brief Get repository search directories.
       *
       *  The method %is not thread-safe.
       *
       *  \return The method returns list of repository search directories.
       */

    const std::list<std::string>& get_repository_dirs() const {return p_repository_dirs;}


      /**
       *  \brief Commit user modifications into repository.
       *
       *  The method commits changes made in user repository into origin GIT repository.
       *
       *  \param comments      user comments
       *  \param credentials   daq token to get real user name (ignore, if empty)
       *
       *  \throw Throw oks::exception in case of problems.
       */

    void
    commit_repository(const std::string& comments, const std::string& credentials = "");


      /**
       *  \brief Tag current state of repository.
       *
       *  The method assigns a tag to current state of repository.
       *
       *  \param tag           the tag
       *
       *  \throw Throw oks::exception in case of problems.
       */

    void
    tag_repository(const std::string& tag);


      /**
       *  \brief Return repository checkout timestamp.
       *
       *  Return timestamp when the repository was checkout or updated last time (e.g. to track new changes).
       */

    std::time_t
    get_repository_checkout_ts() const
    {
      return p_repository_checkout_ts;
    }


    enum RepositoryUpdateType
      {
        DiscardChanges, MergeChanges, NoChanges
      };


    /**
     *  \brief Update user repository files from origin by hash.
     *
     *  The method performs GIT update of files in local user directory.
     *
     *  \param  hash_val        SHA or "origin/master"
     *  \param  update_type     action in case of conflicts (discard changes, try to merge changes, cancel)
     *
     *  \throw Throw oks::exception in case of problems.
     */

    void
    update_repository(const std::string& hash_val, RepositoryUpdateType update_type)
    {
      update_repository("hash", hash_val, update_type);
    }


    /**
     *  \brief Update user repository files from origin.
     *
     *  The method performs GIT update of files in local user directory.
     *  Pass parameters to the oks-git-update.sh script.
     *
     *  \param  param           "tag", "date" or "hash"
     *  \param  val             SHA or "origin/master"
     *  \param  update_type     action in case of conflicts (discard changes, try to merge changes, cancel)
     *
     *  \throw Throw oks::exception in case of problems.
     */

    void
    update_repository(const std::string& param, const std::string& val, RepositoryUpdateType update_type);


    /**
     *  Return names of files updated between revisions.
     *
     *  \throw Throw oks::exception in case of problems.
     */

    std::list<std::string>
    get_repository_versions_diff(const std::string& sha1, const std::string& sha2);


    /**
     *  Special behavior of get_repository_versions_diff(), return unmerged files
     */

    std::list<std::string>
    get_repository_unmerged_files()
    {
      return get_repository_versions_diff("", "");
    }


    /**
     *  \brief Return repository versions.
     *
     *  \param  skip_irrelevant  ignore changes not affecting loaded configuration
     *  \param  command_line     forward command line parameter to the oks-git-log.sh
     *
     *  \throw Throw oks::exception in case of problems.
     */

    std::vector<OksRepositoryVersion>
    get_repository_versions(bool skip_irrelevant, const std::string& command_line);


    /**
     *  \brief Return repository versions between hash keys.
     *
     *  \param  skip_irrelevant  ignore changes not affecting loaded configuration
     *  \param  sha1             limit the versions committed on-or-after the specified hash key; if empty, start from earliest available
     *  \param  sha2             limit the versions committed on-or-before the specified hash key; if empty, retrieve all versions until "origin/master"
     *
     *  \throw Throw oks::exception in case of problems.
     */

    std::vector<OksRepositoryVersion>
    get_repository_versions_by_hash(bool skip_irrelevant = true, const std::string& sha1 = "", const std::string& sha2 = "");


    /**
     *  \brief Return repository versions between timestamps.
     *
     *  \param  skip_irrelevant  ignore changes not affecting loaded configuration
     *  \param  since            limit the versions committed on-or-after the specified date/time
     *  \param  until            limit the versions committed on-or-before the specified date/time
     *
     *  \throw Throw oks::exception in case of problems.
     */

    std::vector<OksRepositoryVersion>
    get_repository_versions_by_date(bool skip_irrelevant = true, const std::string& since = "", const std::string& until = "");


    /**
     *  \brief Read and return current repository version.
     *
     *  \throw Throw oks::exception in case of problems.
     */

    std::string
    read_repository_version();



      /**
       * \return current working dir
       */

    static const char * get_cwd();

      /**
       * \brief Reset current working dir in case of chdir() call.
       */

    static void reset_cwd() { s_cwd = 0;}


      /**
       * \brief Set flag to use strict-repository-paths check.
       */

    static void
    set_use_strict_repository_paths(bool flag)
    {
      p_use_strict_repository_paths = flag;
    }


     /**
       *  \brief Insert repository search directory.
       *
       *  The method trues to insert new repository search directory.
       *  If no such directory exists, then the method inserts new directory and returns its
       *  fully qualified name. Otherwise method does nothing and return empty string.
       *
       *  The method %is not thread-safe.
       *
       *  \param dir  directory name
       *  \param push_back   if true, push new directory back; otherwise push directory front
       */

    std::string insert_repository_dir(const std::string& dir, bool push_back = true);


      /**
       *  \brief Remove repository search directory.
       *
       *  The method removes from the search list directory with given name.
       *  The method %is not thread-safe.
       *
       *  \param dir  directory name (must be fully qualified name)
       */

    void remove_repository_dir(const std::string& dir);


      /**
       *  \brief Get classes.
       *
       *  In multi-threaded environment to iterate through the set it could be necessary
       *  to acquire at least read oks kernel lock before calling the method to be sure
       *  that another thread does not modify the set of classes (i.e. destroy or create
       *  classes, open or close oks schema files).
       *
       *  \return The method returns const reference on set of classes loaded in-memory.
       */

    const OksClass::Map & classes() const {return p_classes;}


      /**
       *  \brief Get number of classes.
       *
       *  \return The method returns number of classes loaded in-memory.
       */

    size_t number_of_classes() const {return p_classes.size();}


      /**
       *  \brief Get objects.
       *
       *  In multi-threaded environment to iterate through the set it could be necessary
       *  to acquire at least read oks kernel lock before calling the method to be sure
       *  that another thread does not modify the set of objects (i.e. destroy or create
       *  objects, open or close oks data files).
       *
       *  \return The method returns const reference on set of all objects loaded in-memory.
       */

    const OksObject::Set & objects() const {return p_objects;}


      /**
       *  \brief Get number of objects.
       *
       *  The method returns number of objects loaded in-memory.
       */

    size_t number_of_objects() const {return p_objects.size();}


      /**
       *  \brief Find class by name (C++ string).
       *
       *  If the method %is used in the multi-threaded environment, it could be necessary
       *  to set read kernel lock before calling the method, if other threads can to create
       *  or to destroy oks classes and to open or close oks schema files.
       *
       *  \param class_name   name of the class (std::string)
       *
       *  \return Return pointer on OKS class or null pointer, if such class can not be found.
       */

    OksClass * find_class(const std::string& class_name) const { return find_class(class_name.c_str()); }


      /**
       *  \brief Find class by name (C-string).
       *
       *  If the method %is used in the multi-threaded environment, it could be necessary
       *  to set read kernel lock before calling the method, if other threads can to create
       *  or to destroy oks classes and to open or close oks schema files.
       *
       *  \param class_name   name of the class
       *
       *  \return Return pointer on OKS class or null pointer, if such class cannot be found.
       */

    OksClass * find_class(const char * class_name) const;


     /**
       *  \brief The method searches all classes including subclasses for given names.
       *
       *  \param names_in     array of names of the classes to search in
       *  \param classes_out  the output containing pointers to all classes by above names and their subclasses
       */

    void get_all_classes(const std::vector<std::string>& names_in, oks::ClassSet& classes_out) const;


     /**
       *  \brief The method rebuilds all classes taking into account inheritance.
       *
       *  \param skip_registered   skip registration of already processed classes (used to improve performance)
       *
       *  \throw Throw oks::exception in case of problems.
       */

    void registrate_all_classes(bool skip_registered = false);


      /**
       *  \brief Check pointer on oks class.
       *
       *  A pointer on class becomes dangling if such class was destroyed, or the
       *  schema file which contains it was closed. The method scans through all oks
       *  classes and checks if given pointer %is valid, i.e. points to an oks class.
       *
       *  If the method %is used in the multi-threaded environment, it may be necessary
       *  to set read kernel lock before calling the method, in case if another threads
       *  can to create or to destroy classes or to open or to close oks schema files.
       *
       *  \param class_ptr   a pointer to the oks class to be tested
       *
       *  \return Return true if the pointer on class %is not valid.
       */

    bool is_dangling(OksClass * class_ptr) const;


      /**
       *  \brief Check pointer on oks object.
       *
       *  A pointer on object becomes dangling if such object was destroyed, or the
       *  data file which contains it was closed. The method scans through all oks
       *  objects and checks if given pointer %is valid, i.e. points to an oks object.
       *
       *  If the method %is used in the multi-threaded environment, it may be necessary
       *  to set read kernel lock before calling the method, in case if another threads
       *  can to create or to destroy objects or to open or to close oks data files.
       *
       *  \param obj_ptr     a pointer to the oks object to be tested
       *
       *  \return Return true if the pointer on object %is not valid.
       */

    bool is_dangling(OksObject * obj_ptr) const;


      /**
       *  \brief Subscribe on class creation.
       *
       *  The method subscribes user-provided callback function on creation of new oks class.
       *  The callback function %is called when a new class %is created and its argument %is
       *  the pointer to the new class.
       *
       *  \param f   user callback function
       */

    void subscribe_create_class(void (*f)(OksClass *));


      /**
       *  \brief Subscribe on class changing.
       *
       *  The method subscribes user-provided callback function on changes of existing oks class.
       *  The callback function %is called when a class %is changed and its arguments are
       *  the pointer to the changed class, the type of change and the change parameter.
       *
       *  \param f   user callback function
       */

    void subscribe_change_class(void (*f)(OksClass *, OksClass::ChangeType, const void *));


      /**
       *  \brief Subscribe on class destroying.
       *
       *  The method subscribes user-provided callback function on destroying of existing oks class.
       *  The callback function %is called when a class %is destroying (e.g. schema file containing the
       *  class %is closed) and its argument %is the pointer to the destroying class.
       *
       *  \param f   user callback function
       */

    void subscribe_delete_class(void (*f)(OksClass *));


      /**
       *  \brief Subscribe on object creation.
       *
       *  The method subscribes user-provided callback function on creation of new oks object.
       *  The callback function %is called when a new object %is created (except when it %is read
       *  from file using load_data() method).
       *
       *  \param cb_f        user callback function
       *  \param parameter   parameter to be passed to the user callback function
       */

    void subscribe_create_object(OksObject::notify_obj cb_f, void * parameter);


      /**
       *  \brief Subscribe on object changing.
       *
       *  The method subscribes user-provided callback function on changing of existing oks object.
       *  The callback function %is called when an object %is changed.
       *
       *  \param cb_f        user callback function
       *  \param parameter   parameter to be passed to the user callback function
       */

    void subscribe_change_object(OksObject::notify_obj, void *);


      /**
       *  \brief Subscribe on object deleting.
       *
       *  The method subscribes user-provided callback function on destroying of existing oks object.
       *  The callback function %is called when an object %is destroying (e.g. data file containing the
       *  object %is closed) and its argument %is the pointer to the destroying object.
       *
       *  \param cb_f        user callback function
       *  \param parameter   parameter to be passed to the user callback function
       */

    void subscribe_delete_object(OksObject::notify_obj, void *);


      /**
       *  \brief Bind oks objects.
       *
       *  The method sets links between oks objects converting whenever it %is possible
       *  values of relationships from the UID types to the OksObject pointer. It %is used after
       *  sequential loading of several data files with bind_objects parameter set to false.
       *
       *  The status of the last bind objects call can be checked using get_bind_objects_status() method.
       *
       *  \throw Throw oks::exception in case of problems.
       */

    void bind_objects();


    /**
     *  \brief Return status of oks objects binding.
     *
     *  The method returns string containing unbound references after last bind_objects() call.
     *
     *  \return If all objects were successfully linked, the string %is empty.
     *  Otherwise it contains description of unbound references and objects.
     */

    const std::string& get_bind_objects_status() const noexcept { return p_bind_objects_status; }


    /**
     *  \brief Return status of oks classes binding.
     *
     *  The method checks lack of dangling references on class types and returns string containing unbound references if any.
     *
     *  \return If all classes were successfully linked, the string %is empty.
     *  Otherwise it contains description of unbound references and classes.
     */

    const std::string& get_bind_classes_status() const noexcept { return p_bind_classes_status; }


    /**
     *  \brief Set repository created flag to false to avoid created repository removal in destructor;
     */

    void
    unset_repository_created();


  private:
    bool p_silence;
    bool p_verbose;
    bool p_profiling;
    bool p_allow_repository;
    bool p_allow_duplicated_classes;
    bool p_allow_duplicated_objects;
    bool p_test_duplicated_objects_via_inheritance;

    static bool p_skip_string_range;
    static bool p_use_strict_repository_paths;

    static std::string p_repository_root;
    static std::string p_repository_mapping_dir;
    std::string p_user_repository_root;
    bool p_user_repository_root_inited;
    bool p_user_repository_root_created;
    std::string p_repository_version;
    std::time_t p_repository_checkout_ts;
    std::time_t p_repository_update_ts;


      // kernel mutexes

    mutable std::shared_mutex p_kernel_mutex;
    mutable std::mutex p_objects_mutex;
    std::mutex p_objects_refs_mutex;
    std::shared_mutex p_schema_mutex;
    static std::mutex p_parallel_out_mutex;

      // schema and data files members

    OksFile::Map p_schema_files;
    OksFile::Map p_data_files;

    OksFile * p_active_schema;
    OksFile * p_active_data;
    
    bool p_close_all;


      // repository directories

    std::list<std::string> p_repository_dirs;

    static char * s_cwd;


    OksClass::Map p_classes;

    static unsigned long p_count;

    OksProfiler * profiler;

    OksObject::Set p_objects;

    static int p_threads_pool_size;

    std::string p_bind_objects_status;
    mutable std::string p_bind_classes_status;

    oks::LoadErrors p_load_errors;

    std::map<const OksFile *, OksFile *> p_preload_file_info;
    std::vector<OksFile *>  p_preload_added_files;


      // to be used by the kernel only

    std::string create_user_repository_dir();
    void remove_user_repository_dir();

    void k_copy_repository(const std::string& source, const std::string& destination);


      /**
       *  \brief Check out repository files into local user directory.
       *
       *  The method checks out GIT repository files into local user directory.
       *  The files have to be loaded from repository defined by TDAQ_DB_REPOSITORY environment.
       *  The TDAQ_DB_USER_REPOSITORY environment defines user directory and must be set.
       *
       *  \param param         forward parameter to oks-git-checkout.sh
       *  \param val           forward value of above parameter to oks-git-checkout.sh
       *  \param branch        name of the branch to check out
       *
       *  \throw Throw oks::exception in case of problems.
       */

    void k_checkout_repository(const std::string& param, const std::string& val, const std::string& branch);


    OksFile * find_file(const std::string &s, const OksFile::Map& files) const;
    void k_rename_repository_file(OksFile * file_h, const std::string& new_name);

    OksFile * k_load_file(const std::string& name, bool bind, const OksFile * parent, OksPipeline * pipeline);

    void k_load_includes(const OksFile&, OksPipeline *);
    bool k_preload_includes(OksFile * file_h, std::set<OksFile *>& new_files, bool allow_schema_extension);
    void clear_preload_file_info();
    void restore_preload_file_info();

    OksFile * k_load_schema(const std::string&, const OksFile *);
    void k_load_schema(OksFile * fp, std::shared_ptr<OksXmlInputStream> xmls, const OksFile * parent_h);
    void k_close_schema(OksFile *);
    void k_save_schema(OksFile *, bool force = false, OksFile * = 0, const OksClass::Map * = 0);
    void k_rename_schema(OksFile *, const std::string& short_name, const std::string& long_name);

    OksFile * k_load_data(const std::string&, bool, const OksFile *, OksPipeline *);
    void k_load_data(OksFile * fp, char format, std::shared_ptr<OksXmlInputStream> xmls, long file_length, bool bind, const OksFile * parent_h, OksPipeline *);
    void k_close_data(OksFile *, bool);
    void k_save_data(OksFile *, bool = false, OksFile * = nullptr, const OksObject::FSet * = nullptr, bool force_defaults = false);
    void k_rename_data(OksFile *, const std::string& short_name, const std::string& long_name);

    void k_bind_objects();
    void k_check_bind_classes_status() const noexcept;


      /**
       *  \brief Unbind all references on given oks objects.
       *  The method unbinds all relationships referencing given objects.
       *  \param rm_objs        objects to be destroyed
       *  \param updated        return updated objects (i.e. containing unbind references)
       */

    void unbind_all_rels(const OksObject::FSet& rm_objs, OksObject::FSet& updated) const;

    void k_add(OksClass*);
    void k_remove(OksClass*);

    void define(OksObject *o) { std::lock_guard lock(p_objects_mutex); p_objects.insert(o);  }
    void undefine(OksObject *o) { if(!p_objects.empty()) { std::lock_guard lock(p_objects_mutex); p_objects.erase(o); } }

    void add_data_file(OksFile *);
    void add_schema_file(OksFile *);
    void remove_data_file(OksFile *);
    void remove_schema_file(OksFile *);

    bool test_parent(OksFile * file, OksFile::IMap::iterator& i);

    OksObject::notify_obj p_create_object_notify_fn;
    void *                p_create_object_notify_param;

    OksObject::notify_obj p_change_object_notify_fn;
    void *                p_change_object_notify_param;

    OksObject::notify_obj p_delete_object_notify_fn;
    void *                p_delete_object_notify_param;

};


  /**
   *  Inline methods for class OksKernel
   */

inline void
OksKernel::subscribe_create_object(OksObject::notify_obj f, void * p)
{
  p_create_object_notify_fn = f;
  p_create_object_notify_param = p;
}


inline void
OksKernel::subscribe_change_object(OksObject::notify_obj f, void * p)
{
  p_change_object_notify_fn = f;
  p_change_object_notify_param = p;
}


inline void
OksKernel::subscribe_delete_object(OksObject::notify_obj f, void * p)
{
  p_delete_object_notify_fn = f;
  p_delete_object_notify_param = p;
}


inline void
OksKernel::subscribe_create_class(void (*f)(OksClass *))
{
  OksClass::create_notify_fn = f;
}


inline void
OksKernel::subscribe_change_class(void (*f)(OksClass *, OksClass::ChangeType, const void *))
{
  OksClass::change_notify_fn = f;
}


inline void
OksKernel::subscribe_delete_class(void (*f)(OksClass *))
{
  OksClass::delete_notify_fn = f;
}

#endif
