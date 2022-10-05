/**	
 *  \file oks/file.h
 *
 *  This file is part of the OKS package.
 *  Author: Igor SOLOVIEV "https://phonebook.cern.ch/phonebook/#personDetails/?id=432778"
 *
 *  This file contains the declarations for the OKS file.
 */

#ifndef OKS_FILE_H
#define OKS_FILE_H

#include <oks/exceptions.h>

#include <string>
#include <list>
#include <set>
#include <map>
#include <memory>

#include <boost/date_time/posix_time/ptime.hpp>

#include <unordered_set>
#include <unordered_map>

class OksXmlOutputStream;
class OksXmlInputStream;
class OksKernel;
class OksFile;

namespace boost
{
  namespace interprocess
  {
    class file_lock;
  }
}


namespace oks {

    /**
     *  \brief Cannot add include file.
     *  Such exception is thrown when OKS cannot add include file.
     */

  class FileLockError : public exception {

    public:

        /**
	 *  The constructor gets reason from non-oks exception.
	 *  If lock_action = true, then cannot lock file.
	 *  Otherwise cannot unlock file.
	 */

      FileLockError(const OksFile& file, bool lock_action, const std::string& reason) noexcept : exception (fill(file, lock_action, reason), 0) { }

      virtual ~FileLockError() noexcept { }


    private:

      static std::string fill(const OksFile& file, bool lock_action, const std::string& reason) noexcept;

  };


    /**
     *  File modification error.
     */

  class FileChangeError : public exception {

    public:

        /** The constructor gets reason from nested oks exception. **/
      FileChangeError(const OksFile& file, const std::string& action, const exception& reason) noexcept : exception (fill(file, action, reason.what()), reason.level() + 1) { }

        /** The constructor gets reason from non-oks exception. **/
      FileChangeError(const OksFile& file, const std::string& action, const std::string& reason) noexcept : exception (fill(file, action, reason), 0) { }

      virtual ~FileChangeError() noexcept { }


    private:

      static std::string fill(const OksFile& file, const std::string& action, const std::string& reason) noexcept;

  };
  

    /**
     *  File read error.
     */

  class FileCompareError : public exception {

    public:

        /** The constructor gets reason from non-oks exception. **/
      FileCompareError(const std::string& src, const std::string& dest, const std::string& reason) noexcept : exception (fill(src, dest, reason), 0) { }

      virtual ~FileCompareError() noexcept { }


    private:

      static std::string fill(const std::string& src, const std::string& dest, const std::string& reason) noexcept;

  };
  

    /**
     *  \brief Cannot add include file.
     *  Such exception is thrown when OKS cannot add include file.
     */

  class FailedAddInclude : public exception {

    public:

        /** The constructor gets reason from nested oks exception. **/
      FailedAddInclude(const OksFile& file, const std::string& include_name, const exception& reason) noexcept : exception (fill(file, include_name, reason.what()), reason.level() + 1) { }

        /** The constructor gets reason from non-oks exception. **/
      FailedAddInclude(const OksFile& file, const std::string& include_name, const std::string& reason) noexcept : exception (fill(file, include_name, reason), 0) { }

      virtual ~FailedAddInclude() noexcept { }


    private:

      static std::string fill(const OksFile& file, const std::string& include_name, const std::string& reason) noexcept;

  };


    /**
     *  \brief Cannot remove include file.
     *  Such exception is thrown when OKS cannot remove include file.
     */

  class FailedRemoveInclude : public exception {

    public:

        /** The constructor gets reason from nested oks exception. **/
      FailedRemoveInclude(const OksFile& file, const std::string& include_name, const exception& reason) noexcept : exception (fill(file, include_name, reason.what()), reason.level() + 1) { }

        /** The constructor gets reason from non-oks exception. **/
      FailedRemoveInclude(const OksFile& file, const std::string& include_name, const std::string& reason) noexcept : exception (fill(file, include_name, reason), 0) { }

      virtual ~FailedRemoveInclude() noexcept { }


    private:

      static std::string fill(const OksFile& file, const std::string& include_name, const std::string& reason) noexcept;

  };


    /**
     *  \brief Cannot rename include file.
     *  Such exception is thrown when OKS cannot rename include file.
     */

  class FailedRenameInclude : public exception {

    public:

        /** The constructor gets reason from nested oks exception. **/
      FailedRenameInclude(const OksFile& file, const std::string& from, const std::string& to, const exception& reason) noexcept : exception (fill(file, from, to, reason.what()), reason.level() + 1) { }

        /** The constructor gets reason from non-oks exception. **/
      FailedRenameInclude(const OksFile& file, const std::string& from, const std::string& to, const std::string& reason) noexcept : exception (fill(file, from, to, reason), 0) { }

      virtual ~FailedRenameInclude() noexcept { }


    private:

      static std::string fill(const OksFile& file, const std::string& from, const std::string& to, const std::string& reason) noexcept;

  };


    /**
     *  \brief Cannot add comment.
     *  Such exception is thrown when OKS cannot add new comment.
     */

  class FailedAddComment : public exception {

    public:

        /** The constructor gets reason from nested oks exception. **/
      FailedAddComment(const OksFile& file, const exception& reason) noexcept : exception (fill(file, reason.what()), reason.level() + 1) { }

        /** The constructor gets reason from non-oks exception. **/
      FailedAddComment(const OksFile& file, const std::string& reason) noexcept : exception (fill(file, reason), 0) { }

      virtual ~FailedAddComment() noexcept { }


    private:

      static std::string fill(const OksFile& file, const std::string& reason) noexcept;

  };


    /**
     *  \brief Cannot remove comment.
     *  Such exception is thrown when OKS cannot remove comment.
     */

  class FailedRemoveComment : public exception {

    public:

        /** The constructor gets reason from nested oks exception. **/
      FailedRemoveComment(const OksFile& file, const std::string& creation_time, const exception& reason) noexcept : exception (fill(file, creation_time, reason.what()), reason.level() + 1) { }

        /** The constructor gets reason from non-oks exception. **/
      FailedRemoveComment(const OksFile& file, const std::string& creation_time, const std::string& reason) noexcept : exception (fill(file, creation_time, reason), 0) { }

      virtual ~FailedRemoveComment() noexcept { }


    private:

      static std::string fill(const OksFile& file, const std::string& creation_time, const std::string& reason) noexcept;

  };


    /**
     *  \brief Cannot change comment.
     *  Such exception is thrown when OKS cannot change comment.
     */

  class FailedChangeComment : public exception {

    public:

        /** The constructor gets reason from nested oks exception. **/
      FailedChangeComment(const OksFile& file, const std::string& creation_time, const exception& reason) noexcept : exception (fill(file, creation_time, reason.what()), reason.level() + 1) { }

        /** The constructor gets reason from non-oks exception. **/
      FailedChangeComment(const OksFile& file, const std::string& creation_time, const std::string& reason) noexcept : exception (fill(file, creation_time, reason), 0) { }

      virtual ~FailedChangeComment() noexcept { }


    private:

      static std::string fill(const OksFile& file, const std::string& creation_time, const std::string& reason) noexcept;

  };


    /**
     *  \brief The comment about file modification.
     *  A comment can be added when user saves file using OKS tools.
     */

  class Comment {

    friend class ::OksFile;

    private:

        /** The constructor creates comment read from file. Only OksFile is allowed to create comments. */
      Comment() noexcept { ; }

        /** The constructor creates new comment from text. Only OksFile is allowed to create comments. \throw Throw std::exception in case of problems. */
      Comment(const std::string& text, const std::string& author);

        /** The copy constructor. Only OksFile is allowed to create comments. */
      Comment(const oks::Comment& src) noexcept;


    public:

        /** The name of user created comment (as defined by process ID). */
      const std::string& get_created_by() const { return p_created_by; }

        /** The name of author created comment (as defined from user input, e.g. first and last name, email address, etc.). */
      const std::string& get_author() const { return p_author; }

        /** The name of host where comment was created. */
      const std::string& get_created_on() const { return p_created_on; }

        /** The comment text. */
      const std::string& get_text() const { return p_text; }

        /** Parse creation time-stamp and validate text. \throw Throw std::exception in case of problems. */
      void validate(const std::string& creation_time);

        /** Validate text of the comment. \throw Throw std::exception if it is empty. */
      void validate();


    private:

      std::string p_created_by;
      std::string p_author;
      std::string p_created_on;
      std::string p_text;

  };

  struct hash_file_ptr
  {
    inline size_t operator() ( const OksFile * x ) const {
      return reinterpret_cast<size_t>(x);
    }
  };

  struct equal_file_ptr
  {
    inline bool operator()(const OksFile * __x, const OksFile * __y) const {
      return (__x == __y);
    }
  };
}


  /**
   * \brief Provides interface to the OKS XML schema and data files.
   *
   *  An object of this class is used to read/write header part of OKS XML file, includes and comments.
   *  It allows to set update lock on file creating .oks-lock-$filename file: see lock() and unlock() methods.
   *  Also, the class provides methods to get many other properties of the file, e.g. status, access mode,
   *  repository information, number of stored items, size, etc.
   */

class OksFile {

  friend class OksKernel;
  friend class OksClass;
  friend class OksObject;
  friend struct OksLoadObjectsJob;

  public:

    enum Mode {
      ReadOnly,
      ReadWrite
    };

    enum FileStatus {
      FileNotModified,
      FileModified,
      FileWasNotSaved,
      FileRemoved
    };

    struct SortByName {
      bool operator() (const std::string * s1, const std::string * s2) const {
        return *s1 < *s2;
      }
    };

    typedef std::map<const std::string *, OksFile *, SortByName> Map;

    typedef std::unordered_set<OksFile *, oks::hash_file_ptr, oks::equal_file_ptr> Set;
    typedef std::unordered_map<OksFile *, Set, oks::hash_file_ptr, oks::equal_file_ptr> IMap;

      /**
       *   \brief Lock OKS file.
       *
       *   The method locks OKS file: for file .../file.xml the lock file .../.oks-file.xml is created.
       *   The lock file contains information about process locking it and remains exclusive while process created it is alive.
       *
       *   \throw In case of problems (e.g. file was already locked) the method throws oks::exception.
       */

    void lock();


      /**
       *   \brief Unlock OKS file.
       *
       *   The method removes lock created by lock() method.
       *check_repository
       *   \throw In case of problems (e.g. cannot unlink lock file) the method throws oks::exception.
       */

    void unlock();


      /**
       *   \brief Add include file.
       *
       *   The method adds include file to given one.
       *   To load included file it is necessary to save the file and load it again.
       *
       *   \param name name of include file to be added
       *
       *   \throw In case of problems (e.g. cannot lock file) the method throws oks::exception.
       */

    void add_include_file(const std::string& name);


      /**
       *   \brief Remove include file.
       *
       *   The method removes include file from given one.
       *   The name of the file to be removed has to be exactly the same
       *   as passed to the add_include_file() or as it appears in the include section of OKS xml file.
       *   To unload included file it is necessary to save the file and load it again.
       *
       *   \param name name of include file to be removed
       *
       *   \throw In case of problems (e.g. cannot lock file, include does not exist) the method throws oks::exception.
       */

    void remove_include_file(const std::string& name);


      /**
       *   \brief Rename include file.
       *
       *   The method renames already included file.
       *   To re-load new file it is necessary to save the file and load it again.
       *
       *   \param from      exact name of already included file
       *   \param to        new name of include
       *
       *   \throw In case of problems (e.g. cannot lock file, no such include file) the method throws oks::exception.
       */

    void rename_include_file(const std::string& from, const std::string& to);


      /**
       *   \brief Add new comment to file.
       *
       *   The method allows to add text description to the file.
       *
       *   \param text       the description; must not be empty
       *   \param author     the author's description, e.g. first and last names, the e-mail address, etc.
       *
       *   \throw In case of problems (e.g. empty text) the method throws oks::exception.
       */

    void add_comment(const std::string& text, const std::string& author);


      /**
       *   /brief Modify existing comment.
       *
       *   The method allows to modify already existing comment.
       *
       *   /param creation_time   the time-stamp when comment was created ( returned by get_comments() )
       *   /param text            the description; must not be empty
       *   /param author          the author's description, e.g. first and last names, the e-mail address, etc.
       *
       *   \throw In case of problems (e.g. empty text, cannot find comment created at given time) the method throws oks::exception.
       */

    void modify_comment(const std::string& creation_time, const std::string& text, const std::string& author);


      /**
       *   \brief Modify existing comment.
       *
       *   The method allows to erase already existing comment.
       *
       *   \param creation_time   the timestamp when comment was created ( returned by get_comments() )
       *
       *   \throw In case of problems (e.g. cannot find comment created at given time, cannot lock file) the method throws oks::exception.
       */

    void remove_comment(const std::string& creation_time);


      /** Get all comments of file. */

    const std::map<std::string, oks::Comment *>& get_comments() const {return p_comments;}


      /**
       *   \brief Set logical name of file.
       *
       *   The logical name can be associated by user with the file, e.g. to describe its purpose.
       *
       *   \param name the logical name
       *
       *   \throw In case of problems (e.g. cannot lock file) the method throws oks::exception.
       */

    void set_logical_name(const std::string& name);


      /**
       *   \brief Set file type.
       *
       *   The type can be associated by user with the file, e.g. to describe its contents.
       *
       *   \param name the type
       *
       *   \throw In case of problems (e.g. cannot lock file) the method throws oks::exception.
       */

    void set_type(const std::string& type);


    const std::string& get_short_file_name() const {return p_short_name;}


      /** Return real file name (replacing all software links) */

    const std::string& get_full_file_name() const {return p_full_name;}


      /**
       *   \brief Get information about repository, the file belongs to.
       *
       *   If the global database repository is set (e.g. TDAQ_DB_REPOSITORY is defined)
       *   and the file is stored on it, the method returns GlobalRepository.
       *   If the user database repository is set (e.g. TDAQ_DB_USER_REPOSITORY is defined)
       *   and the file is stored on it, the method returns UserRepository.
       *   Otherwise file does not belong to any repository and the method returns NoneRepository.
       *
       *   The file's repository can be changed using OksKernel::checkout() and OksKernel::release() methods:
       *   they move file between global and user repositories. If file is removed in global repository,
       *   while it is stored on the user one, its repository should be changed using unset_repository() method.
       */

    bool is_repository_file() const;


      /**
       *   \brief Get name of file inside repository.
       *
       *   For example, if TDAQ_DB_REPOSITORY="/db/v15", and the file name is "/db/v15/common/params.data.xml",
       *   the file's repository name will be "common/params.data.xml".
       */

    const std::string& get_repository_name() const {return p_repository_name;}


      /**
       *   \brief Get well-formed file name.
       *
       *   In particular, strip repository root prefix.
       */

    const std::string&
    get_well_formed_name() const
      {
        return (!p_repository_name.empty() ? p_repository_name : p_full_name);
      }

    const std::string& get_lock_file() const { init_lock_name(); return p_lock_file_name; }
    const std::string& get_logical_name() const {return p_logical_name;}
    const std::string& get_type() const {return p_type;}
    const std::string& get_oks_format() const {return p_oks_format;}


      /** Return number of classes in schema file, or number of objects in data file. */

    long get_number_of_items() const {return p_number_of_items;}


      /** Return length of file in bytes. */

    long get_size() const {return p_size;}


      /** Return name of user who created this file. */

    const std::string& get_created_by() const {return p_created_by;}


      /** Return timestamp when file was created. */

    const boost::posix_time::ptime get_creation_time() const {return p_creation_time;}


      /** Return name of host where the file was created. */

    const std::string& get_created_on() const {return p_created_on;}


      /** Return name of user who modified the file last time using OKS tools. */

    const std::string& get_last_modified_by() const {return p_last_modified_by;}


      /** Return timestamp when file was modified last time using OKS tools. */

    const boost::posix_time::ptime get_last_modification_time() const {return p_last_modification_time;}


      /** Return name of host where the file was modified last time using OKS tools. */

    const std::string& get_last_modified_on() const {return p_last_modified_on;}


      /**
       *   \brief Return lock status of OKS file.
       *   \return true, if the file is locked by given process and false otherwise
       */

    bool is_locked() const {return (p_lock != nullptr);}


      /**
       *   \brief Return update status of OKS file.
       *   \return true, if the file is updated by given process and false otherwise
       */

    bool is_updated() const {return p_is_updated;}


      /**
       *   \brief Return read-only status of OKS file.
       *   \return true, if the file is read-only for given process and false otherwise
       */

    bool is_read_only() const {return p_is_read_only;}


      /**
       *   \brief Get directly include files.
       *   \return list of file names included by given file
       */

    const std::list<std::string>& get_include_files() const {return p_list_of_include_files;}


      /**
       *   \brief Get all include files.
       *   \param kernel  OKS kernel
       *   \param out     set of all files included explicitly and implicitly by given file
       */

    void get_all_include_files(const OksKernel * kernel, std::set<OksFile *>& out);

      /**
       *   \brief Return update status of file.
       *   
       *   The file can be in one of the following states:
       *   - NotModified - file is not modified
       *   - FileModified - file is modified in memory
       *   - FileWasNotSaved - file was created, but not saved yet (only exists in memory)
       *   - FileRemoved - file was removed by external process after load in memory
       *   - FileRepositoryModified - repository file was modified (given file comes from user repository)
       *   - FileRepositoryRemoved - repository file was removed (given file comes from user repository)
       */

    FileStatus get_status_of_file() const;


      /**
       *   \brief Update status of file.
       *   
       *   \param update_local          if true, update last-modified timestamp of local file
       *   \param update_repository     if true, update last-modified timestamp of repository file
       */

    void update_status_of_file(bool update_local = true, bool update_repository = true);
  

      /**
       *   \brief Return lock status of OKS file and if the file is locked, get information string about process which locked the file.
       *
       *   The method method also works for files locked by external process contrary to the is_locked() method.
       *
       *   \param info       out parameter containing info about process locked this file
       *   \return           true, if the file is locked and false otherwise
       */

    bool get_lock_string(std::string& info) const;


      /**
       *   \brief Compare two files.
       *
       *   \param file1_name  name of first file
       *   \param file2_name  name of second file
       *   \return false => files are different; true => files are equal
       *   \throw In case of problems the method throws oks::exception.
       */

    static bool compare(const char * file1_name, const char * file2_name);


      /**
       *   \brief Compare with another file.
       *
       *   \param name name of file to compare with
       *   \return false => files are different; true => files are equal
       *   \throw In case of problems the method throws oks::exception.
       */

    bool compare(const char * name) const { return compare(p_full_name.c_str(), name); }


      /**
       *   \brief Return parent including given file.
       *
       *   \return            parent file; can be NULL.
       */

    const OksFile * get_parent() const { return p_included_by; }


      /**
       *   \brief Set given parent, if this file is not yet included
       *
       *   \param parent_h    parent file
       *   \return            this file.
       */

    OksFile * check_parent(const OksFile * parent_h);



  public:

    ~OksFile() noexcept;


  private:

    std::string p_short_name;  // short path to file
    std::string p_full_name;   // path which is used to open file
    std::string p_repository_name; // the name of file inside repository
    std::string p_logical_name;// name of file, e.g. "Control Workstations"
    std::string p_type;        // type of file, e.g. "DCS"
    std::string p_oks_format;  // format of file: "data" or "schema"
    long p_number_of_items;    // number of objects or classes
    long p_size;               // size of file in bytes
    std::string p_created_by;
    boost::posix_time::ptime p_creation_time;
    std::string p_created_on;
    std::string p_last_modified_by;
    boost::posix_time::ptime p_last_modification_time;
    std::string p_last_modified_on;
    Mode p_open_mode;
    std::shared_ptr<boost::interprocess::file_lock> p_lock;
    std::string p_lock_file_name; // lock file
    bool p_is_updated;
    bool p_is_read_only;
    std::list<std::string> p_list_of_include_files;
    time_t p_last_modified;
    time_t p_repository_last_modified;
    std::map<std::string, oks::Comment *> p_comments;
    bool p_is_on_disk;
    const OksFile * p_included_by;
    OksKernel * p_kernel;

    static const char xml_file_header[];
    static const char xml_schema_file_dtd[];
    static const char xml_data_file_dtd[];
    static const char xml_info_tag[];
    static const char xml_include_tag[];
    static const char xml_file_tag[];
    static const char xml_comments_tag[];
    static const char xml_comment_tag[];


      // only to be used by the friends

    OksFile(
      const std::string&,  // file full path
      const std::string&,  // file logical name
      const std::string&,  // file type
      const std::string&,  // file oks-format
      OksKernel *
    );

    OksFile& operator=(const OksFile &);

    OksFile(const OksFile &f) { *this = f; }

    OksFile(std::shared_ptr<OksXmlInputStream>, const std::string&, const std::string&, OksKernel *);
    void write(OksXmlOutputStream&);

    void set_updated() {p_is_updated = true;} // in-memory

    void rename(const std::string& short_name, const std::string& full_name);
    void rename(const std::string& full_name);  // repository rename
    void create_lock_name();
    void init_lock_name() const { if(p_lock_file_name.empty()) const_cast<OksFile*>(this)->create_lock_name(); }
    void check_repository();

    std::string make_repository_name() const;

    oks::Comment * get_comment(const std::string& creation_time) noexcept;

    void clear_comments() noexcept;

    std::list<std::string>::iterator find_include_file(const std::string&);

};

std::ostream& operator<<(std::ostream&, const OksFile&);
std::ostream& operator<<(std::ostream&, const oks::Comment&);

#endif
