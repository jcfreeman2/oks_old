#define _OksBuildDll_

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <wchar.h>
//#include <security/pam_appl.h>
#include <sys/stat.h>
#include <sys/wait.h>

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <vector>
#include <mutex>
#include <chrono>
#include <cstring>
#include <ctime>

#include "ers/ers.hpp"
#include "logging/Logging.hpp"

#include "system/Host.hpp"
#include "system/User.hpp"
#include "system/exceptions.hpp"

//#include <daq_tokens/verify.h>

#include "oks/config/map.hpp"

#include "oks/kernel.hpp"
#include "oks/xml.hpp"
#include "oks/file.hpp"
#include "oks/attribute.hpp"
#include "oks/relationship.hpp"
#include "oks/method.hpp"
#include "oks/class.hpp"
#include "oks/object.hpp"
#include "oks/profiler.hpp"
#include "oks/pipeline.hpp"
#include "oks/cstring.hpp"

#include "oks_utils.h"

std::mutex OksKernel::p_parallel_out_mutex;

static std::mutex s_get_cwd_mutex;

bool OksKernel::p_skip_string_range = false;
bool OksKernel::p_use_strict_repository_paths = true;
char * OksKernel::s_cwd = nullptr;

std::string OksKernel::p_repository_root;
std::string OksKernel::p_repository_mapping_dir;

int OksKernel::p_threads_pool_size = 0;


namespace oks
{
  class GitFoldersHolder
  {
  public:
    ~GitFoldersHolder()
    {
      std::lock_guard scoped_lock(s_git_folders_mutex);

      for (const auto &x : s_git_folders)
        remove(x);

      s_git_folders.clear();
    }

    static void
    remove(const std::string &path)
    {
      try
        {
          std::filesystem::remove_all(path);
        }
      catch (std::exception &ex)
        {
          Oks::error_msg("OksKernel::~OksKernel") << "cannot remove user repository \"" << path << "\"" << ex.what() << std::endl;
        }
    }

    void
    insert(const std::string& path)
    {
      std::lock_guard scoped_lock(s_git_folders_mutex);
      s_git_folders.insert(path);
    }

    void
    erase(const std::string& path)
    {
      std::lock_guard scoped_lock(s_git_folders_mutex);
      s_git_folders.erase(path);
    }

  private:

    std::mutex s_git_folders_mutex;
    std::set<std::string> s_git_folders;
  };

  static GitFoldersHolder s_git_folders;
}


const std::string
oks::strerror(int error)
{
  char buffer[1024];
  buffer[0] = 0;
  return std::string(strerror_r(error, buffer, 1024));
}

namespace oks {

  ERS_DECLARE_ISSUE(
    kernel,
    SetGroupIdFailed,
    "cannot set group ID " << id << " for the file \'" << file << "\': chown() failed with code " << code << " , reason = \'" << why << '\'',
    ((long)id)
    ((const char *)file)
    ((int)code)
    ((std::string)why)
  )

  ERS_DECLARE_ISSUE(
    kernel,
    BindError,
    "Found unresolved reference(s):\n" << which,
    ((std::string)which)
  )

  ERS_DECLARE_ISSUE(
    kernel,
    ClassAlreadyDefined,
    "class \"" << name << "\" is defined in files \"" << f1 << "\" and \"" << f2 << "\"",
    ((std::string)name)
    ((std::string)f1)
    ((std::string)f2)
  )

  ERS_DECLARE_ISSUE(
    kernel,
    InternalError,
    "internal error: " << text,
    ((std::string)text)
  )

  std::string
  CanNotOpenFile::fill(const char * prefix, const std::string& name, const std::string& reason) noexcept
  {
    std::string result(prefix);
    result += "(): ";

    if(name.empty()) return (result + "file name is empty");
  
    result += "cannot load file \'" + name + '\'' ;

    if(!reason.empty()) {
      result += " because:\n" + reason;
    }

    return result;
  }

  std::string
  FailedLoadFile::fill(const std::string& what, const std::string& name, const std::string& reason) noexcept
  {
    return (std::string("Failed to load ") + what + " \"" + name + "\" because:\n" + reason);
  }

  std::string
  FailedReloadFile::fill(const std::string& names, const std::string& reason) noexcept
  {
    return (std::string("Failed to re-load file(s) ") + names + " because:\n" + reason);
  }

  std::string
  RepositoryOperationFailed::fill(const char * op, const std::string& reason) noexcept
  {
    return (std::string("Failed to ") + op + " file(s) because:\n" + reason);
  }

  std::string
  CanNotCreateFile::fill(const char * prefix, const char * what, const std::string& name, const std::string& reason) noexcept
  {
    return std::string(prefix) + "(): failed to create new " + what + " \'" + name + "\' because:\n" + reason;
  }

  std::string
  CanNotCreateRepositoryDir::fill(const char * prefix, const std::string& name) noexcept
  {
    return std::string(prefix) + "(): failed to create user repository dir, mkdtemp(\'" + name + "\') failed: " + oks::strerror(errno);
  }

  std::string
  CanNotWriteToFile::fill(const char * prefix, const char * item, const std::string& name, const std::string& reason) noexcept
  {
    return (std::string(prefix) + "(): failed to write " + item + " to \'" + name + "\' because:\n" + reason);
  }

  std::string
  CanNotBackupFile::fill(const std::string& name, const std::string& reason) noexcept
  {
    return (std::string("Failed to make a backup file of \'") + name + "\' because:\n" + reason);
  }

  std::string
  CanNotSetActiveFile::fill(const char * item, const std::string& name, const std::string& reason) noexcept
  {
    return (std::string("Failed to set active ") + item + " file \'" + name + "\' because:\n" + reason);
  }

  std::string
  CanNotSetFile::fill(const OksClass * c, const OksObject * o, const OksFile& file, const std::string& reason) noexcept
  {
    std::ostringstream s;
    s << "Failed to move ";
    if(c) s << "class " << c->get_name();
    else s << "object " << o;
    s << " to file \'" << file.get_full_file_name() << "\' because:\n" << reason;
    return s.str();
  }

  std::string
  CannotAddClass::fill(const OksClass &c, const std::string& reason) noexcept
  {
    return (std::string("Cannot add class \'") + c.get_name() + "\' because:\n" + reason);
  }

  std::string
  CannotResolvePath::fill(const std::string& path, int error_code) noexcept
  {
    std::ostringstream text;
    text << "realpath(\'" << path << "\') has failed with code " << error_code << ": \'" << oks::strerror(error_code) << '\'';
    return text.str();
  }


  static const std::string&
  get_temporary_dir()
  {
    static std::string s_dir;
    static std::once_flag flag;

    std::call_once(flag, []()
      {
        try
          {
            if (!getenv("OKS_GIT_NO_VAR_DIR"))
              {
                System::User myself;

                s_dir = "/var/run/user/";
                s_dir.append(std::to_string(myself.identity()));

                if (std::filesystem::is_directory(s_dir) == false)
                  {
                    TLOG_DEBUG(1) << "directory " << s_dir << " does not exist";
                    s_dir.clear();
                  }
                else
                  {
                    const std::filesystem::space_info si = std::filesystem::space(s_dir);
                    const double available = static_cast<double>(si.available) / static_cast<double>(si.capacity);

                    if (available < 0.5)
                      {
                        TLOG_DEBUG(1) << "usage of " << s_dir << " is " << static_cast<int>((1.0 - available) * 100.0) << "%, will use standard temporary path";
                        s_dir.clear();
                      }
                  }
              }

            if (s_dir.empty())
              s_dir = std::filesystem::temp_directory_path().string();
          }
        catch (std::exception& ex)
          {
            Oks::error_msg("OksKernel::OksKernel") << "cannot get temporary directory path: " << ex.what() << std::endl;
            s_dir = "/tmp";
          }
      }
    );

    return s_dir;
  }
}


const char *
OksKernel::GetVersion()
{
  //return OKS_VERSION " built \"" __DATE__ "\"";
  return "JCF, Oct-5-2022: please contact John Freeman at jcfree@fnal.gov or on Slack if you see this message";
}


const std::string&
OksKernel::get_repository_root()
{
  static std::once_flag flag;

  std::call_once(flag, []()
    {
       if (const char * s = getenv("TDAQ_DB_REPOSITORY"))
         {
           std::string rep(s);

           if (!std::all_of(rep.begin(), rep.end(), [](char c) { return std::isspace(c); }))
             {
               const char * p = getenv("OKS_GIT_PROTOCOL");

               if (p && !*p)
                 p = nullptr;

               Oks::Tokenizer t(rep, "|");
               std::string token;

               while (t.next(token) && p_repository_root.empty())
                 {
                   if (p)
                     {
                       if (token.find(p) == 0)
                         {
                           p_repository_root = token;
                         }
                     }
                   else
                     {
                       p_repository_root = token;
                     }
                 }

               if (p_repository_root.empty())
                 Oks::error_msg("OksKernel::OksKernel") << "cannot find OKS_GIT_PROTOCOL=\"" << p << "\" in TDAQ_DB_REPOSITORY=\"" << rep << '\"' << std::endl;
             }
         }
    }
  );

  return p_repository_root;
}

const std::string&
OksKernel::get_repository_mapping_dir()
{
  static std::once_flag flag;

  std::call_once(flag, []()
    {
      if (const char * s = getenv("OKS_REPOSITORY_MAPPING_DIR"))
        {
          p_repository_mapping_dir = s;
          Oks::real_path(p_repository_mapping_dir, true);
        }
    }
  );

  return p_repository_mapping_dir;
}

////////////////////////////////////////////////////////////////////////////////

const std::string&
OksKernel::get_user_repository_root() const
{
  if (p_allow_repository)
    {
      OksKernel * k(const_cast<OksKernel*>(this));
      if(!p_user_repository_root_inited) {
        if(const char * s = getenv("TDAQ_DB_USER_REPOSITORY")) {
          if(s[0] != 0) {
            k->set_user_repository_root(s);
          }
        }
        k->p_user_repository_root_inited = true;
      }
    }

  return p_user_repository_root;
}

void
OksKernel::set_user_repository_root(const std::string& path, const std::string& version)
{
  if(get_repository_root().empty()) {
    TLOG_DEBUG( 1) << "Failed to set user-repository-root:\n\tcaused by: repository-root is not set" ;
    p_user_repository_root.clear();
    return;
  }

  if(path.empty()) {
    p_user_repository_root.clear();
    return;
  }

  std::string s;
  s = path;

  try {
    Oks::real_path(s, false);
  }
  catch(oks::exception& ex) {
    TLOG_DEBUG( 1) << "Failed to set user-repository-root = \'" << s << "\':\n\tcaused by: " << ex.what() ;
  }

  for(std::string::size_type idx = s.size()-1; idx > 0 && s[idx] == '/' ; idx--) {
    s.erase(idx);
  }

  p_allow_repository = true;

  p_user_repository_root = s;
  p_user_repository_root_inited = true;

  // is used for backup restore by RDB
  if (!version.empty())
    p_repository_version = version;
}

////////////////////////////////////////////////////////////////////////////////

std::string&
OksKernel::get_host_name()
{
  static std::string s_host_name;
  static std::once_flag flag;

  std::call_once(flag, []()
    {
      s_host_name = System::LocalHost::instance()->full_local_name();

      if (s_host_name.empty())
        s_host_name = "unknown.host";
    }
  );

  return s_host_name;
}

std::string&
OksKernel::get_domain_name()
{
  static std::string s_domain_name;
  static std::once_flag flag;

  std::call_once(flag, []()
    {
      std::string::size_type idx = get_host_name().find('.');

      if (idx != std::string::npos)
        s_domain_name = get_host_name().substr(idx+1);

      if (s_domain_name.empty())
        s_domain_name = "unknown.domain";
    }
  );

  return s_domain_name;
}

std::string&
OksKernel::get_user_name()
{
  static std::string s_user_name;
  static std::once_flag flag;

  std::call_once(flag, []()
    {
      System::User myself;
      s_user_name = myself.name_safe();

      if (s_user_name.empty())
        s_user_name = "unknown.user";
    }
  );

  return s_user_name;
}


////////////////////////////////////////////////////////////////////////////////

static long
get_file_length(std::ifstream& f)
{
  long file_length = 0;

    // get size of file and check that it is not empty

  f.seekg(0, std::ios::end);

#if __GNUC__ >= 3
  file_length = static_cast<std::streamoff>(f.tellg());
#else
  file_length = f.tellg();
#endif

  f.seekg(0, std::ios::beg);

  return file_length;
}

  // makes name of function with parameters for methods working with files

std::string
make_fname(const char * f, size_t f_len, const std::string& file, bool * p, const OksFile * const * fh)
{
  const char _prefix [] = "OksKernel::";
  std::string fname(_prefix, (sizeof(_prefix)-1));

  fname.append(f, f_len);

  const char _x1 [] = "(file \'";
  fname.append(_x1, (sizeof(_x1)-1));
  fname.append(file);
  fname.push_back('\'');

    // add boolean parameter

  if(p) {
    if(*p) {
      const char _true_str [] = ", true";
      fname.append(_true_str, (sizeof(_true_str)-1));
    }
    else {
      const char _false_str [] = ", false";
      fname.append(_false_str, (sizeof(_false_str)-1));
    }
  }

    // add file handler parameter

  if(fh && *fh) {
    const char _x2 [] = ", included by file \'";
    fname.append(_x2, (sizeof(_x2)-1));
    fname.append((*fh)->get_full_file_name());
    fname.push_back('\'');
  }

  fname.push_back(')');

  return fname;
}

  //
  // Prototypes for error and warning messages
  //

std::ostream&
Oks::error_msg(const char *msg)
{
  std::cerr << "ERROR [" << msg << "]:\n"; return std::cerr;
}


std::ostream&
Oks::warning_msg(const char *msg)
{
  std::cerr << "WARNING [" << msg << "]:\n"; return std::cerr;
}


void
Oks::substitute_variables(std::string& s)
{
  std::string::size_type pos = 0;	// position of tested string index
  std::string::size_type p_start = 0;	// begining of variable
  std::string::size_type p_end = 0;	// begining of variable

  while(
   ((p_start = s.find("$(", pos)) != std::string::npos) &&
   ((p_end = s.find(")", p_start + 2)) != std::string::npos)
  ) {
    std::string var(s, p_start + 2, p_end - p_start - 2);

    char * env = getenv(var.c_str());

    if(env) {
      s.replace(p_start, p_end - p_start + 1, env);
    }

    pos = p_start + 1;
  }
}


bool
Oks::real_path(std::string& path, bool ignore_errors)
{
  char resolved_name[PATH_MAX];
  
  if(realpath(path.c_str(), resolved_name) != 0) {
    path = resolved_name;
    return true;
  }
  else {
    if(ignore_errors) {
      TLOG_DEBUG( 3) << "realpath(\'" << path << "\') has failed with code " << errno << ": \'" << oks::strerror(errno) << '\'' ;
      return false;
    }
    else {
      throw oks::CannotResolvePath(path, errno);
    }
  }
}


unsigned long OksKernel::p_count = 0;
const char OksNameTable::symbols[] = "0123456789"
                                     "abcdefghijklmnoprstuvwxyz"
                                     "ABCDEFGHIJKLMNOPRSTUVWXYZ";


OksString*
OksNameTable::get()
{
  static const size_t slen(sizeof(symbols)-1);
  static const size_t slen2(slen*slen);

  char b[16], *buf = b;

  count++;

  *buf++ = symbols[count%slen];

  if((size_t)count >= slen) {
    *buf++ = symbols[(count/slen)%slen];

    if((size_t)count >= slen2) {
      *buf++ = symbols[(count/slen2)%slen];
    }
  }

  *buf = '\0';

  return new OksString(b);
}


OksAliasTable::~OksAliasTable()
{
  if (p_aliases.empty())
    return;

  for (auto& i : p_aliases)
    {
      if (OksString *s = i.second->class_name)
        delete s;

      delete i.second;
      delete const_cast<OksString *>(i.first);
    }

  p_aliases.clear();
}


std::string
OksKernel::get_tmp_file(const std::string& file)
{
  unsigned int j = 0;
  std::string s2;
  std::ostringstream s;

  while(j++ < 1000000) {
    s.str("");
    s << file << '.' << get_user_name() << ':' << get_host_name() << ':' << getpid() << ':' << j;

    s2 = s.str();

    std::ofstream f(s2.c_str(), std::ios::in);

    if(!f) return s2;
  }

  std::string s3(file);
  s3 += ".tmp";

  return s3;
}

const char *
OksKernel::get_cwd()
{
  std::lock_guard scoped_lock(s_get_cwd_mutex);

  if (!s_cwd)
  {
    errno = 0;
    long size = pathconf(".", _PC_PATH_MAX);
    if (errno)
      {
        Oks::error_msg("OksKernel::OksKernel") << "pathconf(\".\", _PC_PATH_MAX) has failed with code " << errno << ": \'" << oks::strerror(errno) << '\'' << std::endl;
      }
    else
      {
        if (size == -1)
          {
            size = PATH_MAX;
          }
        s_cwd = new char[size];
        if (!getcwd(s_cwd, (size_t) size))
          {
            Oks::error_msg("OksKernel::OksKernel") << "getcwd() has failed with code " << errno << ": \'" << oks::strerror(errno) << '\'' << std::endl;
            delete[] s_cwd;
            s_cwd = 0;
          }
      }

    if (!s_cwd)
      {
        s_cwd = new char[2];
        s_cwd[0] = '/';
        s_cwd[1] = 0;
      }

    TLOG_DEBUG(1) << "Current working dir: \'" << s_cwd << '\'';
  }

  return s_cwd;
}

/******************************************************************************/
/*****************************  OKS Kernel Class  *****************************/
/******************************************************************************/

OksKernel::OksKernel(bool sm, bool vm, bool tm, bool allow_repository, const char * version, std::string branch_name) :
  p_silence	                              (sm),
  p_verbose	                              (vm),
  p_profiling	                              (tm),
  p_allow_repository                          (allow_repository),
  p_allow_duplicated_classes                  (true),
  p_allow_duplicated_objects                  (false),
  p_test_duplicated_objects_via_inheritance   (false),
  p_user_repository_root_inited               (false),
  p_user_repository_root_created              (false),
  p_active_schema                             (nullptr),
  p_active_data                               (nullptr),
  p_close_all                                 (false),
  profiler	                              (nullptr),
  p_create_object_notify_fn                   (nullptr),
  p_create_object_notify_param                (nullptr),
  p_change_object_notify_fn                   (nullptr),
  p_change_object_notify_param                (nullptr),
  p_delete_object_notify_fn                   (nullptr),
  p_delete_object_notify_param                (nullptr)
{
  OSK_VERBOSE_REPORT("Enter OksKernel::OksKernel(" << sm << ", " << vm << ", " << tm << ')')

  struct __InitFromEnv__ {
    const char * name;
    bool& value;
  } vars[] = {
    {"OKS_KERNEL_VERBOSE",                                 p_verbose                                 },
    {"OKS_KERNEL_SILENCE",                                 p_silence                                 },
    {"OKS_KERNEL_PROFILING",                               p_profiling                               },
    {"OKS_KERNEL_ALLOW_REPOSITORY",                        p_allow_repository                        },
    {"OKS_KERNEL_ALLOW_DUPLICATED_CLASSES",                p_allow_duplicated_classes                },
    {"OKS_KERNEL_ALLOW_DUPLICATED_OBJECTS",                p_allow_duplicated_objects                },
    {"OKS_KERNEL_TEST_DUPLICATED_OBJECTS_VIA_INHERITANCE", p_test_duplicated_objects_via_inheritance },
    {"OKS_KERNEL_SKIP_STRING_RANGE",                       p_skip_string_range                       }
  };

  for(unsigned int i = 0; i < sizeof(vars) / sizeof(__InitFromEnv__); ++i) {
    if(char * s = getenv(vars[i].name)) {
      vars[i].value = (strcmp(s, "no")) ? true : false;
    }
  }

  {
    const char * oks_db_root = getenv("OKS_DB_ROOT");

    if (oks_db_root == nullptr || *oks_db_root == 0)
      oks_db_root = getenv("TDAQ_DB_PATH");
    else
      p_use_strict_repository_paths = false;

    OSK_VERBOSE_REPORT("database root = \'" << oks_db_root << "\' (as defined by the OKS_DB_ROOT or TDAQ_DB_PATH)");


      // if not defined, set oks_db_root to empty string (to avoid std::string constructor crash when string is 0) 

    if (oks_db_root == nullptr)
      oks_db_root = "";


      // temporal set

    Oks::Tokenizer t(oks_db_root, ":");
    std::string token;

    while (t.next(token))
      insert_repository_dir(token);
  }




  {
    std::unique_lock lock(p_kernel_mutex);

    if(!p_count++) {
      OksClass::create_notify_fn = 0;
      OksClass::change_notify_fn = 0;
      OksClass::delete_notify_fn = 0;
    }

    get_cwd();
  }
  
  {
    static std::once_flag flag;

    std::call_once(flag, []()
      {
        if (char * s = getenv("OKS_KERNEL_THREADS_POOL_SIZE"))
          {
            if (*s != '\0')
              {
                p_threads_pool_size = atoi(s);
                if (p_threads_pool_size < 1)
                  p_threads_pool_size = 1;
              }
          }

        if (!p_threads_pool_size)
          {
            errno = 0;
            p_threads_pool_size = sysconf(_SC_NPROCESSORS_ONLN);
            if (p_threads_pool_size == -1 && !errno)
              {
                Oks::error_msg("OksKernel::OksKernel()") << " sysconf(_SC_NPROCESSORS_ONLN) has failed with code " << errno << ": \'" << oks::strerror(errno) << '\'' << std::endl;
                p_threads_pool_size = 0;
              }
          }

        if (!p_threads_pool_size)
          p_threads_pool_size = 2;

        TLOG_DEBUG(2) << "Threads pool size: " << p_threads_pool_size;
      }
    );

  }

  // process OKS server URL settings if any
  if (p_allow_repository && !get_repository_root().empty())
    {
      if (get_user_repository_root().empty())
        {
          // parse specific version option if any
          std::string param, val;

          if (!version)
            version = getenv("TDAQ_DB_VERSION");

          if (version)
            {
              if(*version)
                {
                  if(const char * value = strchr(version, ':'))
                    {
                      param.assign(version, value-version);
                      val.assign(value+1);
                    }
                  else
                    Oks::error_msg("OksKernel::OksKernel") << "bad version value \"" << version << "\" expecting parameter:value format (check TDAQ_DB_VERSION variable)" << std::endl;
                }
            }

          try
            {
              std::string tmp_dirname = OksKernel::create_user_repository_dir();

              set_user_repository_root(tmp_dirname);
              p_user_repository_root_created = true;
              oks::s_git_folders.insert(p_user_repository_root);

              if (branch_name.empty())
                  if (const char *var = getenv("TDAQ_DB_BRANCH"))
                    branch_name = var;

              k_checkout_repository(param, val, branch_name);
            }
          catch(oks::exception& ex)
            {
              Oks::error_msg("OksKernel::OksKernel") << "cannot check out user repository: " << ex.what() << std::endl;
            }
        }
      else
        {
          if(!p_silence)
            {
              try
                {
                  std::lock_guard lock(p_parallel_out_mutex);
                  std::cout << "attach to repository \"" << get_user_repository_root() << "\" version " << read_repository_version() << std::endl;
                }
              catch(oks::exception& ex)
                {
                  Oks::error_msg("OksKernel::OksKernel") << "cannot read user repository version: " << ex.what() << std::endl;
                }
            }
        }
    }

#ifndef ERS_NO_DEBUG
  if(p_profiling)
    profiler = (OksProfiler *) new OksProfiler();
#endif
}

OksKernel::OksKernel(const OksKernel& src, bool copy_repository) :
  p_silence                                   (src.p_silence),
  p_verbose                                   (src.p_verbose),
  p_profiling                                 (src.p_profiling),
  p_allow_repository                          (src.p_allow_repository),
  p_allow_duplicated_classes                  (src.p_allow_duplicated_classes),
  p_allow_duplicated_objects                  (src.p_allow_duplicated_objects),
  p_test_duplicated_objects_via_inheritance   (src.p_test_duplicated_objects_via_inheritance),
  p_user_repository_root                      (src.p_user_repository_root),
  p_user_repository_root_inited               (src.p_user_repository_root_inited),
  p_user_repository_root_created              (false),
  p_repository_version                        (src.p_repository_version),
  p_repository_checkout_ts                    (src.p_repository_checkout_ts),
  p_repository_update_ts                      (src.p_repository_update_ts),
  p_active_schema                             (nullptr),
  p_active_data                               (nullptr),
  p_close_all                                 (false),
  profiler                                    (nullptr),
  p_create_object_notify_fn                   (nullptr),
  p_create_object_notify_param                (nullptr),
  p_change_object_notify_fn                   (nullptr),
  p_change_object_notify_param                (nullptr),
  p_delete_object_notify_fn                   (nullptr),
  p_delete_object_notify_param                (nullptr)
{

  {
    std::unique_lock lock(p_kernel_mutex);
    p_count++;
  }

  if (copy_repository && p_user_repository_root_inited)
    {
      try
        {
          p_user_repository_root = OksKernel::create_user_repository_dir();
          p_user_repository_root_created = true;
          Oks::real_path(p_user_repository_root, false);
          oks::s_git_folders.insert(p_user_repository_root);
          k_copy_repository(src.p_user_repository_root, p_user_repository_root);
        }
      catch(oks::exception& ex)
        {
          Oks::error_msg("OksKernel::OksKernel") << "cannot copy user repository: " << ex.what() << std::endl;
          remove_user_repository_dir();
          throw;
        }

      p_repository_dirs.push_back(p_user_repository_root);
    }


  // copy repository directories
  for (const auto& i : src.p_repository_dirs)
    p_repository_dirs.push_back(i);


  // copy schema and data files
  {
    const OksFile::Map * src_files[2] = { &src.p_schema_files, &src.p_data_files };
    OksFile::Map * dst_files[2] = { &p_schema_files, &p_data_files };

    for (int i = 0; i < 2; ++i)
      {
        for (const auto& j : *src_files[i])
          {
            OksFile * f = new OksFile(*j.second);
            f->p_kernel = this;
            (*dst_files[i])[&f->p_full_name] = f;
          }
      }
  }

  // search a class in map is not efficient, if there are many such operations; use array with class index for fast search
  OksClass ** c_table = new OksClass * [src.p_classes.size()];
  unsigned int idx(0);


  // copy classes: first iteration to define class names and simple properties
  for(const auto& i : src.p_classes) {
    const OksClass& src_c(*i.second);
    OksClass * c = new OksClass (src_c.p_name, src_c.p_description, src_c.p_abstract, nullptr, src_c.p_transient);   // pass 0 kernel to avoid class insertion

    c->p_kernel = this;
    p_classes[c->get_name().c_str()] = c;

    const_cast<OksClass&>(src_c).p_id = idx++;
    c->p_id = src_c.p_id;
    c_table[c->p_id] = c;

    c->p_abstract          = src_c.p_abstract;
    c->p_to_be_deleted     = src_c.p_to_be_deleted;
    c->p_instance_size     = src_c.p_instance_size;
    c->p_file              = (*p_schema_files.find(&src_c.p_file->p_full_name)).second;
    c->p_objects           = (src_c.p_objects ? new OksObject::Map() : nullptr);


    // copy super-classes
    if (const std::list<std::string *> * scls = src_c.p_super_classes)
      {
        c->p_super_classes = new std::list<std::string *>();

        for (const auto& j : *scls)
          c->p_super_classes->push_back(new std::string(*j));
      }


    // copy direct attributes
    if (const std::list<OksAttribute *> * attrs = src_c.p_attributes)
      {
        c->p_attributes = new std::list<OksAttribute *>();

        for (const auto& j : *attrs)
          {
            OksAttribute * a = new OksAttribute(j->p_name, c);

            a->p_range = j->p_range;
            a->p_data_type = j->p_data_type;
            a->p_multi_values = j->p_multi_values;
            a->p_no_null = j->p_no_null;
            a->p_init_value = j->p_init_value;
            a->p_init_data = j->p_init_data;
            a->p_format = j->p_format;
            a->p_description = j->p_description;

            if (j->p_enumerators)
              a->p_enumerators = new std::vector<std::string>(*j->p_enumerators);

            c->p_attributes->push_back(a);
          }
      }


    // copy direct relationships
    if (const std::list<OksRelationship *> * rels = src_c.p_relationships)
      {
        c->p_relationships = new std::list<OksRelationship *>();

        for (const auto& j : *rels)
          {
            OksRelationship * r = new OksRelationship(j->p_name, c);

            r->p_rclass = j->p_rclass;
            r->p_low_cc = j->p_low_cc;
            r->p_high_cc = j->p_high_cc;
            r->p_composite = j->p_composite;
            r->p_exclusive = j->p_exclusive;
            r->p_dependent = j->p_dependent;
            r->p_description = j->p_description;

            c->p_relationships->push_back(r);
          }
      }


    // copy direct methods
    if (const std::list<OksMethod *> * mets = src_c.p_methods)
      {
        c->p_methods = new std::list<OksMethod *>();

        for (const auto& j : *mets)
          {
            OksMethod * m = new OksMethod(j->p_name, j->p_description, c);

            if (const std::list<OksMethodImplementation *> * impls = j->p_implementations)
              {
                m->p_implementations = new std::list<OksMethodImplementation *>();

                for (const auto& x : *impls)
                  m->p_implementations->push_back(new OksMethodImplementation(x->get_language(), x->get_prototype(), x->get_body(), m));
              }

            c->p_methods->push_back(m);
          }
      }
  }


    // copy classes: second iteration to resolve pointers to classes

  for (const auto& i : src.p_classes)
    {
      OksClass * c = c_table[i.second->p_id];

      // link p_class_type of relationship
      if (const std::list<OksRelationship *> * rels = i.second->p_relationships)
        {
          for (const auto& j : *rels)
            c->find_relationship(j->get_name())->p_class_type = (j->p_class_type ? c_table[j->p_class_type->p_id] : nullptr);
        }

      // copy all super- and sub- classes
      if (const OksClass::FList * spcls = i.second->p_all_super_classes)
        {
          c->p_all_super_classes = new OksClass::FList();

          for (const auto& j : *spcls)
            c->p_all_super_classes->push_back(c_table[j->p_id]);
        }

      if (const OksClass::FList * sbcls = i.second->p_all_sub_classes)
        {
          c->p_all_sub_classes = new OksClass::FList();

          for (const auto& j : *sbcls)
            c->p_all_sub_classes->push_back(c_table[j->p_id]);
        }

      // create oks object layout information
      c->p_data_info = new OksDataInfo::Map();
      size_t instance_size = 0;

      // copy all attributes
      if (const std::list<OksAttribute *> * attrs = i.second->p_all_attributes)
        {
          c->p_all_attributes = new std::list<OksAttribute *>();

          for (const auto& j : *attrs)
            {
              OksAttribute * a = (c_table[j->p_class->p_id])->find_direct_attribute(j->get_name());
              c->p_all_attributes->push_back(a);
              (*c->p_data_info)[a->get_name()] = new OksDataInfo(instance_size++, a);
            }
        }

      // copy all relationships
      if (const std::list<OksRelationship *> * rels = i.second->p_all_relationships)
        {
          c->p_all_relationships = new std::list<OksRelationship *>();

          for (const auto& j : *rels)
            {
              OksRelationship * r = (c_table[j->p_class->p_id])->find_direct_relationship(j->get_name());
              c->p_all_relationships->push_back(r);
              (*c->p_data_info)[r->get_name()] = new OksDataInfo(instance_size++, r);
            }
        }

      // copy all methods
      if (const std::list<OksMethod *> * mets = i.second->p_all_methods)
        {
          c->p_all_methods = new std::list<OksMethod *>();

          for (const auto& j : *mets)
            c->p_all_methods->push_back((c_table[j->p_class->p_id])->find_direct_method(j->get_name()));
        }

      // copy inheritance hierarchy
      if (const std::vector<OksClass *> * ih = i.second->p_inheritance_hierarchy)
        {
          c->p_inheritance_hierarchy = new std::vector<OksClass *>();
          c->p_inheritance_hierarchy->reserve(ih->size());

          for (const auto& j : *ih)
            c->p_inheritance_hierarchy->push_back(c_table[j->p_id]);
        }
    }


    // copy objects

  if(!src.p_objects.empty()) {

      // search an object in a class is not efficien, if there are many such operations
      // use array with class index for fast search

    OksObject ** o_table = new OksObject * [src.p_objects.size()];
    idx = 0;

      // first iteration: create objects with attributes

    for(OksObject::Set::const_iterator i = src.p_objects.begin(); i != src.p_objects.end(); ++i, ++idx) {
      OksObject * src_o(*i);
      src_o->p_user_data = reinterpret_cast<void *>(idx);

      OksClass * c = c_table[src_o->uid.class_id->p_id];

      OksObject * o = new OksObject(
        c,
        src_o->uid.object_id,
        src_o->p_user_data,
        src_o->p_int32_id,
        src_o->p_duplicated_object_id_idx,
	(*p_data_files.find(&src_o->file->p_full_name)).second
      );

      o_table[idx] = o;
      (*c->p_objects)[&o->uid.object_id] = o;
      p_objects.insert(o);

      if(size_t num_of_attrs = c->number_of_all_attributes()) {
        const OksData * src_data = src_o->data;
        OksData * dst_data = o->data;

	std::list<OksAttribute *>::const_iterator ia_dst = c->all_attributes()->begin();
	std::list<OksAttribute *>::const_iterator ia_src = src_o->uid.class_id->all_attributes()->begin();

        for(size_t j = 0; j < num_of_attrs; ++j) {
          *dst_data = *src_data;

	  OksAttribute * a_dst(*ia_dst);
	  
	    // process in a special way CLASS type (reference on class)
	  
	  if(a_dst->get_data_type() == OksData::class_type) {
	    if(a_dst->get_is_multi_values() == false) {
	      dst_data->data.CLASS = c_table[src_data->data.CLASS->p_id];
	    }
	    else {
	      OksData::List::iterator li_dst = dst_data->data.LIST->begin();
	      OksData::List::iterator li_src = src_data->data.LIST->begin();

	      while(li_dst != dst_data->data.LIST->end()) {
	        (*li_dst)->data.CLASS = c_table[(*li_src)->data.CLASS->p_id];
	        ++li_dst;
	        ++li_src;
	      }
	    }
	  }

	    // process in a special way ENUM type (reference on attribute's data)

	  else if(a_dst->get_data_type() == OksData::enum_type) {
	    OksAttribute * a_src(*ia_src);
	    const std::string * p_enumerators_first(&((*(a_src->p_enumerators))[0]));

	    if(a_dst->get_is_multi_values() == false) {
	      unsigned long dx = src_data->data.ENUMERATION - p_enumerators_first;
	      dst_data->data.ENUMERATION = &((*(a_dst->p_enumerators))[dx]);
	    }
	    else {
	      OksData::List::iterator li_dst = dst_data->data.LIST->begin();
	      OksData::List::iterator li_src = src_data->data.LIST->begin();

	      while(li_dst != dst_data->data.LIST->end()) {
	        unsigned long dx = (*li_src)->data.ENUMERATION - p_enumerators_first;
	        (*li_dst)->data.ENUMERATION = &((*(a_dst->p_enumerators))[dx]);
	        ++li_dst;
	        ++li_src;
	      }
	    }
	  }

          src_data++;
          dst_data++;

	  ia_dst++;
	  ia_src++;
        }
      }
    }


      // second iteration: link relationships of objects

    for(const auto& src_o : src.p_objects) {
      OksClass * c = c_table[src_o->uid.class_id->p_id];
      OksObject * o(o_table[reinterpret_cast<unsigned long>(src_o->p_user_data)]);

      if(size_t num_of_rels = c->number_of_all_relationships()) {
        const OksData * src_data(src_o->data + c->number_of_all_attributes());
        OksData * dst_data(o->data + c->number_of_all_attributes());

        for(size_t j = 0; j < num_of_rels; ++j) {
          dst_data->type = src_data->type;
          switch(src_data->type) {
            case OksData::list_type:
              dst_data->data.LIST = new OksData::List();
              for(const auto& x : *src_data->data.LIST) {
                OksData *d = new OksData();

                if(x->type == OksData::object_type)
                  {
                    if(const OksObject * o2 = x->data.OBJECT)
                      {
                        d->Set(o_table[reinterpret_cast<unsigned long>(o2->p_user_data)]);
                      }
                    else
                      {
                        d->Set((OksObject *)nullptr);
                      }
                  }
                else if(x->type == OksData::uid_type)
                  {
                    d->Set(c_table[x->data.UID.class_id->p_id], *x->data.UID.object_id);
                  }
                else if(x->type == OksData::uid2_type)
                  {
                    d->Set(*x->data.UID2.class_id, *x->data.UID2.object_id);
                  }
                else
                  {
                    ers::error(oks::kernel::InternalError(ERS_HERE, "unexpected data type in relationship list"));
                  }

                dst_data->data.LIST->push_back(d);
              }
              break;

            case OksData::object_type:
	      if(const OksObject * o2 = src_data->data.OBJECT) {
                dst_data->data.OBJECT = o_table[reinterpret_cast<unsigned long>(o2->p_user_data)];
	      }
	      else {
                dst_data->data.OBJECT = 0;
	      }
              break;

            case OksData::uid_type:
              dst_data->data.UID.class_id  = c_table[src_data->data.UID.class_id->p_id];
              dst_data->data.UID.object_id = new OksString(*src_data->data.UID.object_id);
              break;

            case OksData::uid2_type:
              dst_data->data.UID2.class_id  = new OksString(*src_data->data.UID2.class_id);
              dst_data->data.UID2.object_id = new OksString(*src_data->data.UID2.object_id);
              break;

            default:
              ers::error(oks::kernel::InternalError(ERS_HERE, "unexpected data type in relationship"));
              break;
          }

          src_data++;
          dst_data++;
        }
      }

      if (const std::list<OksRCR *> * src_rcrs = src_o->p_rcr)
        {
          o->p_rcr = new std::list<OksRCR *>();

          for (const auto& j : *src_rcrs)
            o->p_rcr->push_back(
                new OksRCR(
                    o_table[reinterpret_cast<unsigned long>(j->obj->p_user_data)],
                    (c_table[j->relationship->p_class->p_id])->find_direct_relationship(j->relationship->get_name())
                )
            );
        }
      else
        {
          o->p_rcr = nullptr;
        }
    }

    delete [] o_table;
  }

  delete [] c_table;

  // rename all files
  if (copy_repository && p_user_repository_root_inited)
    {
      OksFile::Map * files_map[2] = { &p_schema_files, &p_data_files };

      for (int i = 0; i < 2; ++i)
        {
          std::vector<OksFile *> files;

          for (auto& f : *files_map[i])
            files.push_back(f.second);

          files_map[i]->clear();

          for (auto& f : files)
            {
              std::string name = p_user_repository_root;
              name.push_back('/');
              name.append(f->get_repository_name());
              f->rename(name);
              (*files_map[i])[&f->p_full_name] = f;
            }
        }
    }
}


std::string
OksKernel::insert_repository_dir(const std::string& dir, bool push_back)
{
  std::unique_lock lock(p_kernel_mutex);

  std::string s(dir);
  Oks::substitute_variables(s);

  try {
    Oks::real_path(s, false);

    if(std::find(p_repository_dirs.begin(), p_repository_dirs.end(), s) == p_repository_dirs.end()) {
      if(push_back) p_repository_dirs.push_back(s); else p_repository_dirs.push_front(s);
      if(p_verbose) {
        std::cout << " * push " << (push_back ? "back" : "front") << " repository search directory \'" << s << "\'\n";
      }
      return s;
    }
  }
  catch(oks::exception& ex) {
    TLOG_DEBUG( 1) << "Cannot insert repository dir \'" << dir << "\':\n\tcaused by: " << ex ;
  }

  return "";
}

void
OksKernel::remove_repository_dir(const std::string& dir)
{
  std::unique_lock lock(p_kernel_mutex);

  std::list<std::string>::iterator i = std::find(p_repository_dirs.begin(), p_repository_dirs.end(), dir);
  if(i != p_repository_dirs.end()) {
    if(p_verbose) {
      std::cout << " * remove repository search directory \'" << dir << "\'\n";
    }
    p_repository_dirs.erase(i);
  }
}


void
OksKernel::set_profiling_mode(const bool b)
{
  if(p_profiling == b) return;

  p_profiling = b;

#ifndef ERS_NO_DEBUG
  if(p_profiling == true) {
    if(!profiler) profiler = new OksProfiler();
  }
  else {
    if(profiler) {
      delete profiler;
      profiler = 0;
    }
  }
#endif
}


OksKernel::~OksKernel()
{
  {
    OSK_PROFILING(OksProfiler::KernelDestructor, this)
    OSK_VERBOSE_REPORT("ENTER OksKernel::~OksKernel()")

    close_all_data();
    close_all_schema();

    std::unique_lock lock(p_kernel_mutex);

    p_classes.erase(p_classes.begin(), p_classes.end());

    --p_count;

    if(p_count == 0) {
      std::lock_guard scoped_lock(s_get_cwd_mutex);
      delete [] s_cwd;
      s_cwd = nullptr;
    }

    remove_user_repository_dir();
  }

#ifndef ERS_NO_DEBUG
  if(p_profiling) std::cout << *profiler << std::endl;
#endif

  OSK_VERBOSE_REPORT("LEAVE OksKernel::~OksKernel()")
}


std::ostream&
operator<<(std::ostream& s, OksKernel& k)
{
  OSK_PROFILING(OksProfiler::KernelOperatorOut, &k)

  s << "OKS KERNEL DUMP:\n" << "OKS VERSION: " << k.GetVersion() << std::endl;

  if (!k.p_schema_files.empty())
    {
      s << " Loaded schema:\n";

      for (const auto& i : k.p_schema_files)
        s << "   " << i.second->get_full_file_name() << std::endl;

      if (!k.p_data_files.empty())
        {
          s << " Loaded data:\n";

          for (const auto& j : k.p_data_files)
            s << "   " << j.second->get_full_file_name() << std::endl;
        }
      else
        s << " No loaded data files\n";

      if (!k.p_classes.empty())
        {
          s << " The classes:\n";

          for (const auto& j : k.p_classes)
            s << *j.second;
        }
    }
  else
    s << " No loaded schema files\n";

  s << "END OF OKS KERNEL DUMP.\n";

  return s;
}

bool
OksKernel::check_read_only(OksFile *fp)
{
  static std::string suffix;
  static std::once_flag flag;

  std::call_once(flag, []()
    {
      std::ostringstream s;
      s << '-' << get_user_name() << ':' << get_host_name() << ':' << getpid() << std::ends;
      suffix = s.str();
    }
  );

  std::string s(fp->p_full_name);
  s.append(suffix);

  {
    static std::mutex p_check_read_only_mutex;
    std::lock_guard scoped_lock(p_check_read_only_mutex);

    {
      std::ofstream f(s.c_str(), std::ios::out);
      fp->p_is_read_only = (!f.good());
    }

    TLOG_DEBUG( 3) << "read-only test on file \"" << s << "\" returns " << fp->p_is_read_only ;

    if(!fp->p_is_read_only) {
      unlink(s.c_str());
    }

    return fp->p_is_read_only;
  }
}

OksFile *
OksKernel::create_file_info(const std::string& short_path, const std::string& full_path)
{
  OksFile * file_h = 0;

  {
    std::shared_ptr<std::ifstream> f(new std::ifstream(full_path.c_str()));

    if(f->good()) {
      std::shared_ptr<OksXmlInputStream> xmls(new OksXmlInputStream(f));
      file_h = new OksFile(xmls, short_path, full_path, this);
    }
  }
  
    // check file access permissions

  if(file_h) {
    check_read_only(file_h);
  }

  return file_h;
}

  // The function tests file existence, touches the file,
  // prints out warnings/errors in case of problems
  // and finally prints out info message.

static void
test_file_existence(const std::string& file_name, bool silence, const std::string& fname, const char * msg)
{
    // test file existence

  bool file_exists = false;

  {
    struct stat buf;
    if(stat(file_name.c_str(), &buf) == 0) {
      if(!silence) {
        Oks::warning_msg(fname) << "  File \"" << file_name << "\" already exists\n";
      }
      file_exists = true;
    }
  }


    // test file permissions

  {
    std::ofstream f(file_name.c_str());

    if(!f) {
      if(!silence) {
        if(file_exists) {
	  throw std::runtime_error("cannot open file in write mode");
	}
        else {
	  throw std::runtime_error("cannot create file");
	}
      }
    }
  }

  if(!silence)
    std::cout << "Creating new " << msg << " file \"" << file_name << "\"..." << std::endl;
}


inline std::string
mk_name_and_test(const std::string& name, const char * test, size_t test_len)
{
  const char _s1_str[] = " - \'";
  const char _s2_str[] = "\' tested as ";

  return ((std::string(_s1_str, sizeof(_s1_str)-1) + name).append(_s2_str, sizeof(_s2_str)-1)).append(test, test_len);
}


#define TEST_PATH_TOKEN(path, file, msg)                                                                                    \
std::string token(path);                                                                                                    \
token.push_back('/');                                                                                                       \
token.append(file);                                                                                                         \
Oks::substitute_variables(token);                                                                                           \
if(Oks::real_path(token, true)) {                                                                                           \
  TLOG_DEBUG(2) << fname << " returns \'" << token << "\' (filename relative to " << msg << " database repository directory)"; \
  return token;                                                                                                             \
}                                                                                                                           \
else {                                                                                                                      \
  const char _test_name[] = "relative to database repository directory";                                                    \
  tested_files.push_back(mk_name_and_test(token, _test_name, sizeof(_test_name)-1));                                        \
}


std::string
OksKernel::get_file_path(const std::string& s, const OksFile * file_h, bool strict_paths) const
{
  strict_paths &= p_use_strict_repository_paths;

  const char _fname[] = "get_file_path";
  std::string fname;

  TLOG_DEBUG(2) << "ENTER " << make_fname(_fname, sizeof(_fname)-1, s, nullptr, &file_h);

  std::list<std::string> tested_files;

  // check absolute path or path relative to working directory
  bool is_absolute_path = false;

  std::string s2(s);
  Oks::substitute_variables(s2);
  fname = make_fname(_fname, sizeof(_fname) - 1, s2, nullptr, &file_h);

  // test if file has an absolute path
  if (s2[0] == '/')
  is_absolute_path = true;

  // continue only if the file is an absolute path or it is not included
  if (is_absolute_path || file_h == 0)
    {
      if (!is_absolute_path && s_cwd && *s_cwd)
        {
          std::string s3 = s_cwd;
          s3.push_back('/');
          s2 = s3 + s2;
        }

      if (Oks::real_path(s2, true))
        {
          if (!get_user_repository_root().empty() && strict_paths)
            {
              if (!get_repository_mapping_dir().empty() && s2.find(get_repository_mapping_dir()) == 0)
                {
                  s2.erase(0, get_repository_mapping_dir().size()+1);
                  TEST_PATH_TOKEN(get_user_repository_root(), s2, "user")
                }
              else if (s2.find(get_user_repository_root()) == 0)
                {
                  // presumably from explicitly set TDAQ_DB_USER_REPOSITORY created externally
                  TLOG_DEBUG(2) << fname << " returns external \'" << s2 << "\' (an absolute filename or relative to CWD=\'" << s_cwd << "\')";
                  return s2;
                }

              std::ostringstream text;
              text << fname << " file does not belong to oks git repository\n"
                  "provide file repository name, or unset TDAQ_DB_REPOSITORY and try again with TDAQ_DB_PATH process environment variable";
              throw std::runtime_error(text.str().c_str());
            }
          else
            {
              TLOG_DEBUG(2) << fname << " returns \'" << s2 << "\' (an absolute filename or relative to CWD=\'" << s_cwd << "\')";
              return s2;
            }
        }
      else
        {
          if (is_absolute_path)
            {
              const char _test_name[] = "an absolute file name";
              tested_files.push_back(mk_name_and_test(s2, _test_name, sizeof(_test_name) - 1));
            }
          else
            {
              const char _test_name[] = "relative to current working directory";
              tested_files.push_back(mk_name_and_test(s2, _test_name, sizeof(_test_name) - 1));
            }
        }
    }

  if( !get_user_repository_root().empty())
    {
      TEST_PATH_TOKEN(get_user_repository_root(), s, "user")
    }

  if(get_user_repository_root().empty() || !p_use_strict_repository_paths)
    {
    // check paths relative to OKS database root directories
    if (!is_absolute_path)
      for (auto & i : p_repository_dirs)
        {
          TEST_PATH_TOKEN(i, s, "TDAQ_DB_PATH")
        }

    // check non-absolute path relative to parent file if any
    if (file_h && !is_absolute_path)
      {
        std::string s2(file_h->get_full_file_name());
        std::string::size_type pos = s2.find_last_of('/');

        if (pos != std::string::npos)
          {
            s2.erase(pos + 1);
            s2.append(s);
            Oks::substitute_variables(s2);

            if (Oks::real_path(s2, true))
              {
                TLOG_DEBUG(2) << fname << " returns \'" << s2 << "\' (filename relative to parent file)";
                return s2;
              }
            else
              {
                const char _test_name[] = "relative to parent file";
                tested_files.push_back(mk_name_and_test(s2, _test_name, sizeof(_test_name) - 1));
              }
          }
      }
  }

  TLOG_DEBUG(2) << fname << " throw exception (file was not found)";

  std::ostringstream text;
  text << fname << " found no readable file among " << tested_files.size() << " tested:\n";
  for (auto & i : tested_files)
    text << i << std::endl;

  throw std::runtime_error(text.str().c_str());
}


  // user-allowed method

OksFile *
OksKernel::load_file(const std::string& short_file_name, bool bind)
{
  std::unique_lock lock(p_kernel_mutex);
  return k_load_file(short_file_name, bind, 0, 0);
}


  // kernel method

OksFile *
OksKernel::k_load_file(const std::string& short_file_name, bool bind, const OksFile * parent_h, OksPipeline * pipeline)
{
  const char _fname[] = "k_load_file";
  std::string fname = make_fname(_fname, sizeof(_fname)-1, short_file_name, &bind, &parent_h);

  OSK_VERBOSE_REPORT("ENTER " << fname)

  std::string full_file_name;

  try {
    full_file_name = get_file_path(short_file_name, parent_h);
  }
  catch (std::exception & e) {
    throw oks::CanNotOpenFile("k_load_file", short_file_name, e.what());
  }

  try {

      // check if the file is already loaded

    OksFile::Map::const_iterator i = p_schema_files.find(&full_file_name);
    if(i != p_schema_files.end()) return i->second->check_parent(parent_h);

    i = p_data_files.find(&full_file_name);
    if(i != p_data_files.end()) return i->second->check_parent(parent_h);

    std::shared_ptr<std::ifstream> f(new std::ifstream(full_file_name.c_str()));

    if(f->good()) {
      long file_length(get_file_length(*f));
      
      if(!file_length) {
        throw std::runtime_error("k_load_file(): file is empty");
      }

        // read file header and decide what to load

      std::shared_ptr<OksXmlInputStream> xmls(new OksXmlInputStream(f));

      OksFile * file_h = new OksFile(xmls, short_file_name, full_file_name, this);

      if(file_h->p_oks_format.size() == 6 && oks::cmp_str6n(file_h->p_oks_format.c_str(), "schema")) {
        k_load_schema(file_h, xmls, parent_h);
      }
      else {
        char format;

        if(file_h->p_oks_format.size() == 4 && oks::cmp_str4n(file_h->p_oks_format.c_str(), "data"))
          format = 'n';
        else if(file_h->p_oks_format.size() == 8 && oks::cmp_str8n(file_h->p_oks_format.c_str(), "extended"))
          format = 'X';
        else if(file_h->p_oks_format.size() == 7 && oks::cmp_str7n(file_h->p_oks_format.c_str(), "compact"))
          format = 'c';
        else {
          delete file_h;
          throw std::runtime_error("k_load_file(): failed to parse header");
        }

        k_load_data(file_h, format, xmls, file_length, bind, parent_h, pipeline);
      }

      OSK_VERBOSE_REPORT("LEAVE " << fname)

      return file_h;
    }
    else {
      throw std::runtime_error("k_load_file(): cannot open file");
    }
  }
  catch (oks::exception & e) {
    throw oks::FailedLoadFile("file", full_file_name, e);
  }
  catch (std::exception & e) {
    throw oks::FailedLoadFile("file", full_file_name, e.what());
  }
  catch (...) {
    throw oks::FailedLoadFile("file", full_file_name, "caught unknown exception");
  }
}


void
OksKernel::k_load_includes(const OksFile& f, OksPipeline * pipeline)
{
  for(std::list<std::string>::const_iterator i = f.p_list_of_include_files.begin(); i != f.p_list_of_include_files.end(); ++i) {
    if(!(*i).empty()) {
      //if(f.get_repository() != OksFile::NoneRepository) {
      if(!get_user_repository_root().empty()) {
        if((*i)[0] == '/') {
          throw oks::FailedLoadFile("include", *i, "inclusion of files with absolute pathname (like \"/foo/bar\") is not allowed by repository files; include files relative to repository root (like \"foo/bar\")");
        }
        else if((*i)[0] == '.') {
          throw oks::FailedLoadFile("include", *i, "inclusion of files with file-related relative pathnames (like \"../foo/bar\" or \"./bar\") is not supported by repository files; include files relative to repository root (like \"foo/bar\")");
        }
        else if((*i).find('/') == std::string::npos) {
          throw oks::FailedLoadFile("include", *i, "inclusion of files with file-related local pathnames (like \"bar\") is not supported by repository files; include files relative to repository root (like \"foo/bar\")");
        }
      }

      try {
        k_load_file(*i, false, &f, pipeline);
      }
      catch (oks::exception & e) {
        throw oks::FailedLoadFile("include", *i, e);
      }
      catch (std::exception & e) {
        throw oks::FailedLoadFile("include", *i, e.what());
      }
    }
    else {
      throw oks::FailedLoadFile("include", *i, "empty filename");
    }
  }
}

void
OksKernel::get_includes(const std::string& file_name, std::set< std::string >& includes, bool use_repository_name)
{
  try {
    std::shared_ptr<std::ifstream> f(new std::ifstream(file_name.c_str()));

    if(!f->good()) {
      throw std::runtime_error("get_includes(): cannot open file");
    }

    std::shared_ptr<OksXmlInputStream> xmls(new OksXmlInputStream(f));
    OksFile fp(xmls, file_name, file_name, this);

    if(fp.p_list_of_include_files.size()) {
      for(std::list<std::string>::iterator i = fp.p_list_of_include_files.begin(); i != fp.p_list_of_include_files.end(); ++i) {
        includes.insert(use_repository_name ? get_file_path(*i, &fp).substr(get_user_repository_root().size()+1) : get_file_path(*i, &fp));
      }
    }

    f->close();
  }
  catch (oks::exception & e) {
    throw (oks::FailedLoadFile("file", file_name, e));
  }
  catch (std::exception & e) {
    throw (oks::FailedLoadFile("file", file_name, e.what()));
  }
}

bool
OksKernel::test_parent(OksFile * file, OksFile::IMap::iterator& i)
{
  OksFile * parent(i->first);
  OksFile::Set& includes(i->second);

  if(includes.find(file) != includes.end()) {
    if(file->p_included_by != parent) {
      TLOG_DEBUG( 1 ) << "new parent of file " << file->get_full_file_name() << " is " << parent->get_full_file_name() ;
      file->p_included_by = parent;
    }

    return true;
  }

  return false;
}

void
OksKernel::k_close_dangling_includes()
{
    // build inclusion graph

  OksFile::IMap igraph;

  for(OksFile::Map::iterator i = p_schema_files.begin(); i != p_schema_files.end(); ++i) {
    for(std::list<std::string>::const_iterator x = i->second->p_list_of_include_files.begin(); x != i->second->p_list_of_include_files.end(); ++x) {
      std::string f = get_file_path(*x, i->second);
      OksFile::Map::const_iterator it = p_schema_files.find(&f);
      if(it != p_schema_files.end()) {
        igraph[i->second].insert(it->second);
      }
      else {
        std::cerr << "cannot find schema " << *x << " included by " << i->second->get_full_file_name() << std::endl;
      }
    }
  }

  for(OksFile::Map::iterator i = p_data_files.begin(); i != p_data_files.end(); ++i) {
    for(std::list<std::string>::const_iterator x = i->second->p_list_of_include_files.begin(); x != i->second->p_list_of_include_files.end(); ++x) {
      std::string f = get_file_path(*x, i->second);
      OksFile::Map::const_iterator it = p_data_files.find(&f);
      if(it != p_data_files.end()) {
        igraph[i->second].insert(it->second);
      }
      else {
        it = p_schema_files.find(&f);
	
        if(it != p_schema_files.end()) {
          igraph[i->second].insert(it->second);
        }
	else {
          std::cerr << "cannot find file " << *x << " included by " << i->second->get_full_file_name() << std::endl;
        }
      }
    }
  }


  const size_t num_of_schema_files(schema_files().size());


    // calculate files to be closed
    
  while(true) {
    std::list<OksFile *> schema_files, data_files;  // files to be closed
    
    OksFile::Map * files[2] = {&p_schema_files, &p_data_files};
    std::list<OksFile *> to_be_closed[2];

    for(int c = 0; c < 2; ++c) {
      for(OksFile::Map::iterator i = files[c]->begin(); i != files[c]->end(); ++i) {
        if(OksFile * p = const_cast<OksFile *>(i->second->p_included_by)) {
          bool found_parent = false;

            // check, the parent is valid

          if(igraph.find(p) != igraph.end()) {
	    OksFile::IMap::iterator j = igraph.find(p);
            if(test_parent(i->second, j)) {
	      continue;
	    }
          }

          TLOG_DEBUG( 1 ) << "the parent of file " << i->second->get_full_file_name() << " is not valid" ;


            // try to find new parent

          for(OksFile::IMap::iterator j = igraph.begin(); j != igraph.end(); ++j) {
            if(test_parent(i->second, j)) {
	      found_parent = true;
	      break;
	    }
          }

          if(found_parent == false) {
            TLOG_DEBUG( 1 ) << "the file " << i->second->get_full_file_name() << " is not included by any other file and will be closed" ;
            to_be_closed[c].push_back(i->second);
          }
        }
      }
    }

    int num = 0;

    for(std::list<OksFile *>::const_iterator i = to_be_closed[1].begin(); i != to_be_closed[1].end(); ++i) {
      num += (*i)->p_list_of_include_files.size();
      k_close_data(*i, true);
      igraph.erase(*i);
    }

    for(std::list<OksFile *>::const_iterator i = to_be_closed[0].begin(); i != to_be_closed[0].end(); ++i) {
      num += (*i)->p_list_of_include_files.size();
      k_close_schema(*i);
      igraph.erase(*i);
    }

    if(num > 0) {
      TLOG_DEBUG( 1 ) << "go into recursive call, number of potentially closed includes is " << num ;
    }
    else {
      TLOG_DEBUG( 1 ) << "break loop";
      break;
    }
  }
  
  if(num_of_schema_files != schema_files().size()) {
    TLOG_DEBUG( 1 ) << "rebuild classes since number of schema files has been changed from " << num_of_schema_files << " to " << schema_files().size() ;
    registrate_all_classes();
  }
}


bool
OksKernel::k_preload_includes(OksFile * fp, std::set<OksFile *>& new_files_h, bool allow_schema_extension)
{
  bool found_include_changes(false);

  try {
    std::shared_ptr<std::ifstream> file(new std::ifstream(fp->get_full_file_name().c_str()));

    if(!file->good()) {
      throw std::runtime_error(std::string("k_preload_includes(): cannot open file \"") + fp->get_full_file_name() + '\"');
    }

      // keep included files

    std::set<std::string> included;

    if(fp->p_list_of_include_files.size()) {
      for(std::list<std::string>::iterator i = fp->p_list_of_include_files.begin(); i != fp->p_list_of_include_files.end(); ++i) {
        included.insert(*i);
      }
    }

      // get size of file and check that it is not empty

    file->seekg(0, std::ios::end);

    long file_length = static_cast<std::streamoff>(file->tellg());

    if(file_length == 0) {
      throw std::runtime_error(std::string("k_preload_includes(): file \"") + fp->get_full_file_name() + "\" is empty");
    }

    file->seekg(0, std::ios::beg);

    std::shared_ptr<OksXmlInputStream> xmls(new OksXmlInputStream(file));

    {
      OksFile fp2(xmls, fp->get_short_file_name(), fp->get_full_file_name(), this);
      fp2.p_included_by = fp->p_included_by;
      p_preload_file_info[fp] = new OksFile(*fp);
      *fp = fp2;

      if(fp->p_oks_format.empty()) {
        throw std::runtime_error(std::string("k_preload_includes(): failed to read header of file \"") + fp->get_full_file_name() + '\"');
      }
      else if(fp->p_oks_format == "schema") {
        TLOG_DEBUG(2) << "skip reload of schema file \'" << fp->get_full_file_name() << '\'';
        return false;
      }
      else if(fp->p_oks_format != "data" && fp->p_oks_format != "extended" && fp->p_oks_format != "compact") {
        throw std::runtime_error(std::string("k_preload_includes(): file \"") + fp->get_full_file_name() + "\" is not valid oks file");
      }
    }

    if(fp->p_list_of_include_files.size()) {
      TLOG_DEBUG(3) << "check \'include\' of the file \'" << fp->get_full_file_name() << '\'';

      if(included.size() != fp->p_list_of_include_files.size()) found_include_changes = true;

      for(std::list<std::string>::iterator i = fp->p_list_of_include_files.begin(); i != fp->p_list_of_include_files.end(); ++i) {
        std::set<std::string>::const_iterator x = included.find(*i);
	if(x != included.end()) {
	  TLOG_DEBUG(3) << "include \'" << *i << "\' already exists, skip...";
	}
	else {
	  TLOG_DEBUG(3) << "the file \'" << *i << "\' was not previously included by \'" << fp->get_full_file_name() << '\'';

	  found_include_changes = true;

	  std::string full_file_name;

          try {
            full_file_name = get_file_path(*i, fp);
          }
          catch (std::exception & e) {
            throw oks::CanNotOpenFile("k_preload_includes", *i, e.what());
          }

          OksFile::Map::const_iterator j = p_schema_files.find(&full_file_name);
          if(j != p_schema_files.end()) {
	    TLOG_DEBUG(3) << "the include \'" << *i << "\' is already loaded schema file \'" << j->first << '\'';
	    continue;
	  }

          j = p_data_files.find(&full_file_name);
          if(j != p_data_files.end()) {
	    TLOG_DEBUG(3) << "the include \'" << *i << "\' is already loaded data file \'" << j->first << '\'';
	    continue;
	  }

          if(OksFile * f = create_file_info(*i, full_file_name)) {
            if(f->p_oks_format == "schema") {
	      std::string new_schema_full_file_name = f->get_full_file_name();
	      delete f;
	      if(p_schema_files.find(&new_schema_full_file_name) != p_schema_files.end()) {
	        TLOG_DEBUG(3) << "the include \'" << *i << "\' is a schema file, that was already loaded";
		continue;
	      }
	      else {
	        if(allow_schema_extension) {
	          TLOG_DEBUG(3) << "the include \'" << *i << "\' is new schema file, loading...";
	          k_load_schema(*i, fp);
	          continue;
		}
		else {
	          std::ostringstream text;
		  text << "k_preload_includes(): include of new schema file (\'" << *i << "\') is not allowed on data reload";
		  throw std::runtime_error(text.str().c_str());
		}
	      }
	    }
            else if(f->p_oks_format == "data" || f->p_oks_format == "extended" || f->p_oks_format == "compact") {
	      TLOG_DEBUG(3) << "the include \'" << *i << "\' is new data file, pre-loading...";
              add_data_file(f);
	      p_preload_added_files.push_back(f);
	      new_files_h.insert(f);
	      f->p_list_of_include_files.clear();
	      if(k_preload_includes(f, new_files_h, allow_schema_extension)) found_include_changes = true;
              f->p_included_by = fp;
              f->update_status_of_file();
	    }
            else {
              delete f;
	      std::ostringstream text;
	      text << "k_preload_includes(): failed to parse header of included \'" << full_file_name << "\' file";
              throw std::runtime_error(text.str().c_str());
            }
          }
          else {
            throw std::runtime_error("k_load_file(): cannot open file");
          }
	}
      }
    }
  }
  catch (oks::exception & e) {
    throw oks::FailedLoadFile("data file", fp->get_full_file_name(), e);
  }
  catch (std::exception & e) {
    throw oks::FailedLoadFile("data file", fp->get_full_file_name(), e.what());
  }
  
  return found_include_changes;
}


/******************************************************************************/

std::string
OksKernel::create_user_repository_dir()
{
  if (const char * path = getenv("TDAQ_DB_USER_REPOSITORY_PATH"))
    return path;

  // create user repository directory if necessary
  std::string user_repo;

  if (const char * user_repo_base = getenv("TDAQ_DB_USER_REPOSITORY_ROOT"))
    user_repo = user_repo_base;
  else
    user_repo = oks::get_temporary_dir();

  user_repo.push_back('/');
  if (const char * user_repo_pattern = getenv("TDAQ_DB_USER_REPOSITORY_PATTERN"))
    user_repo.append(user_repo_pattern);
  else
    user_repo.append("oks.XXXXXX");

  std::unique_ptr<char[]> dir_template(new char[user_repo.size() + 1]);
  strcpy(dir_template.get(), user_repo.c_str());

  if (char * tmp_dirname = mkdtemp(dir_template.get()))
    {
      return tmp_dirname;
    }

  throw oks::CanNotCreateRepositoryDir("make_user_repository_dir", user_repo);
}


OksFile *
OksKernel::find_file(const std::string &s, const OksFile::Map& files) const
{
  try {
    std::string at = get_file_path(s, nullptr);
    OksFile::Map::const_iterator i = files.find(&at);
    if(i != files.end()) return (*i).second;
  }
  catch (...) {
    // return 0;
  }

  // search files included as relative to parent
  for(const auto& f : files)
    {
      if (s == f.second->get_short_file_name())
        {
          return f.second;
        }
    }

  return nullptr;
}

OksFile *
OksKernel::find_schema_file(const std::string &s) const
{
  return find_file(s, p_schema_files);
}

OksFile *
OksKernel::find_data_file(const std::string &s) const
{
  return find_file(s, p_data_files);
}

/******************************************************************************/

  // user-allowed method

OksFile *
OksKernel::load_schema(const std::string& short_file_name, const OksFile * parent_h)
{
  std::unique_lock lock(p_kernel_mutex);
  return k_load_schema(short_file_name, parent_h);
}

  // kernel method

OksFile *
OksKernel::k_load_schema(const std::string& short_file_name, const OksFile * parent_h)
{
  const char _fname[] = "k_load_schema";
  std::string fname = make_fname(_fname, sizeof(_fname)-1, short_file_name, nullptr, &parent_h);

  OSK_VERBOSE_REPORT("ENTER " << fname)

  std::string full_file_name;

  try {
    full_file_name = get_file_path(short_file_name, parent_h);
  }
  catch (std::exception & e) {
    throw oks::CanNotOpenFile("k_load_schema", short_file_name, e.what());
  }

  try {
    OksFile * fp = find_schema_file(full_file_name);

    if(fp != 0) {
      if(p_verbose) {
        std::lock_guard lock(p_parallel_out_mutex);
        Oks::warning_msg(fname) << "  The file was already loaded\n";
      }
      return fp->check_parent(parent_h);
    }

    {
      std::shared_ptr<std::ifstream> f(new std::ifstream(full_file_name.c_str()));

      if(!f->good()) {
        throw std::runtime_error("k_load_schema(): cannot open file");
      }

      std::shared_ptr<OksXmlInputStream> xmls(new OksXmlInputStream(f));

      fp = new OksFile(xmls, short_file_name, full_file_name, this);

      if(fp->p_oks_format.empty()) {
        throw std::runtime_error("k_load_schema(): failed to read header of file");
      }
      else if(fp->p_oks_format != "schema") {
        throw std::runtime_error("k_load_schema(): file is not oks schema file");
      }

      k_load_schema(fp, xmls, parent_h);

      OSK_VERBOSE_REPORT("LEAVE " << fname)

      return fp;
    }
  }
  catch (oks::FailedLoadFile &) {
    throw; //forward
  }
  catch (oks::exception & e) {
    throw oks::FailedLoadFile("schema file", full_file_name, e);
  }
  catch (std::exception & e) {
    throw oks::FailedLoadFile("schema file", full_file_name, e.what());
  }
  catch (...) {
    throw oks::FailedLoadFile("schema file", full_file_name, "caught unknown exception");
  }
}


void
OksKernel::k_load_schema(OksFile * fp, std::shared_ptr<OksXmlInputStream> xmls, const OksFile * parent_h)
{
  OSK_PROFILING(OksProfiler::KernelLoadSchema, this)

  fp->p_included_by = parent_h;
  check_read_only(fp);

  try {
    if(!p_silence) {
      std::lock_guard lock(p_parallel_out_mutex);
      std::cout << (parent_h ? " * loading " : "Loading ") << fp->p_number_of_items << " classes from file \"" << fp->get_full_file_name() << "\"...\n";
      if(parent_h == 0 && fp->get_full_file_name() != fp->get_short_file_name()) {
        std::cout << "(non-fully-qualified filename was \"" << fp->get_short_file_name() << "\")\n";
      }
    }

    k_load_includes(*fp, 0);

    add_schema_file(fp);

    std::list<OksClass *> set;

    while(true) {
      OksClass *c = 0;

      try {
	c = new OksClass(*xmls, this);
      }
      catch(oks::EndOfXmlStream &) {
        delete c;
        break;
      }

      if(c->get_name().empty()) {
        throw std::runtime_error("k_load_schema(): failed to read a class");
      }

      OksClass::Map::const_iterator ic = p_classes.find(c->get_name().c_str());
      if(ic != p_classes.end()) {
        bool are_different = (*ic->second != *c);

        c->p_transient = true;
        delete c;
        c = ic->second;
        if(p_allow_duplicated_classes && !are_different) {
          static bool ers_report = (getenv("OKS_KERNEL_ERS_REPORT_DUPLICATED_CLASSES") != nullptr);
          if(ers_report) {
            ers::warning(oks::kernel::ClassAlreadyDefined(ERS_HERE,c->get_name(),fp->get_full_file_name(),c->p_file->get_full_file_name()));
          }
          else {
            const char _fname[] = "k_load_schema";
            std::lock_guard lock(p_parallel_out_mutex);
            Oks::warning_msg(make_fname(_fname, sizeof(_fname)-1, fp->get_full_file_name(), nullptr, &parent_h))
              << "  Class \"" << c->get_name() << "\" was already loaded from file \'" << c->get_file()->get_full_file_name() << "\'\n";
          }
        }
        else {
          std::stringstream text;
          text << (are_different ? "different " : "") << "class \"" << c->get_name() << "\" was already loaded from file \'" << c->get_file()->get_full_file_name() << '\'';
          throw std::runtime_error(text.str().c_str());
        }
      }
      else {
        {
          std::unique_lock lock(p_schema_mutex);  // protect schema and all objects from changes
          p_classes[c->get_name().c_str()] = c;
        }
        c->p_file = fp;
        if(OksClass::create_notify_fn) set.push_back(c);
      }
    }

    fp->p_size = xmls->get_position();

    registrate_all_classes(true);

    if(OksClass::create_notify_fn)
      for(std::list<OksClass *>::iterator i2 = set.begin(); i2 != set.end(); ++i2)
        (*OksClass::create_notify_fn)(*i2);

    if(OksClass::change_notify_fn) {
      std::set<OksClass *> set2;

      for(std::list<OksClass *>::iterator i2 = set.begin(); i2 != set.end(); ++i2) {
        OksClass * c = *i2;
        if(c->p_all_sub_classes && !c->p_all_sub_classes->empty()) {
          for(OksClass::FList::iterator i3 = c->p_all_sub_classes->begin(); i3 != c->p_all_sub_classes->end(); ++i3) {
            if(set2.find(*i3) == set2.end()) {
              (*OksClass::change_notify_fn)(*i3, OksClass::ChangeSuperClassesList, (const void *)(&(c->p_name)));
              set2.insert(*i3);
            }
          }
        }
      }
    }

    fp->update_status_of_file();
  }
  catch (oks::exception & e) {
    throw oks::FailedLoadFile("schema file", fp->get_full_file_name(), e);
  }
  catch (std::exception & e) {
    throw oks::FailedLoadFile("schema file", fp->get_full_file_name(), e.what());
  }
  catch (...) {
    throw oks::FailedLoadFile("schema file", fp->get_full_file_name(), "caught unknown exception");
  }
}


OksFile *
OksKernel::new_schema(const std::string& s)
{
  const char _fname[] = "new_schema";
  std::string fname = make_fname(_fname, sizeof(_fname)-1, s, nullptr, nullptr);
  OSK_VERBOSE_REPORT("ENTER " << fname)

  if(!s.length()) {
    throw oks::CanNotOpenFile("new_schema", s, "file name is empty");
  }

  std::string file_name(s);

  try {

    Oks::substitute_variables(file_name);

    test_file_existence(file_name, p_silence, fname, "schema");

    file_name = get_file_path(s, nullptr, false);

    std::unique_lock lock(p_kernel_mutex);

    if(find_schema_file(file_name) != 0) {
      throw std::runtime_error("the file is already loaded");
    }

    OksFile * file_h = new OksFile(file_name, "", "", "schema", this);

    file_h->p_short_name = s;

    k_set_active_schema(file_h);

    add_schema_file(p_active_schema);

    registrate_all_classes(true);

  }
  catch (std::exception & e) {
    throw oks::CanNotCreateFile("new_schema", "schema file", file_name, e.what());
  }
  catch (...) {
    throw oks::CanNotCreateFile("new_schema", "schema file", file_name, "caught unknown exception");
  }

  OSK_VERBOSE_REPORT("LEAVE " << fname)

  return p_active_schema;
}


  // user-allowed method

void
OksKernel::save_schema(OksFile * pf, bool force, OksFile * fh)
{
  std::shared_lock lock(p_kernel_mutex);
  k_save_schema(pf, force, fh);
}


void
OksKernel::save_schema(OksFile * file_h, bool force, const OksClass::Map & classes)
{
  std::shared_lock lock(p_kernel_mutex);
  k_save_schema(file_h, force, 0, &classes);
}


void
OksKernel::backup_schema(OksFile * pf, const char * suffix)
{
  std::shared_lock lock(p_kernel_mutex);

  OksFile f(
    pf->get_full_file_name() + suffix,
    pf->get_logical_name(),
    pf->get_type(),
    pf->get_oks_format(),
    this
  );

  f.p_created_by = pf->p_created_by;
  f.p_creation_time = pf->p_creation_time;
  f.p_created_on = pf->p_created_on;
  f.p_list_of_include_files = pf->p_list_of_include_files;

  if(!p_silence) {
    std::cout << "Making backup of schema file \"" << pf->p_full_name << "\"...\n";
  }

  bool silence = p_silence;

  p_silence = true;

  try {
    k_save_schema(&f, true, pf);
  }
  catch(oks::exception& ex) {
    p_silence = silence;
    throw oks::CanNotBackupFile(pf->p_full_name, ex);
  }

  p_silence = silence;
}

  // kernel method

void
OksKernel::k_save_schema(OksFile * pf, bool force, OksFile * fh, const OksClass::Map * classes)
{
  const char _fname[] = "k_save_schema";
  std::string fname = make_fname(_fname, sizeof(_fname)-1, pf->p_full_name, nullptr, nullptr);

  OSK_PROFILING(OksProfiler::KernelSaveSchema, this)
  OSK_VERBOSE_REPORT("ENTER " << fname)

  std::string tmp_file_name;

  if(!fh) fh = pf;

  try {

      // lock the file, if it is not locked already

    if(pf->is_locked() == false) {
      pf->lock();
    }


      // calculate number of classes, going into the schema file

    size_t numberOfClasses = 0;

    if(classes) {
      numberOfClasses = classes->size();
    }
    else if(!p_classes.empty()) {
      for(OksClass::Map::iterator i = p_classes.begin(); i != p_classes.end(); ++i) {
        if(i->second->p_file == fh) numberOfClasses++;
      }
    }


      // create temporal file to store the schema

    tmp_file_name = get_tmp_file(pf->p_full_name);

    {
      std::ofstream f(tmp_file_name.c_str());
      f.exceptions ( std::ostream::failbit | std::ostream::badbit );

      if(!f) {
        std::ostringstream text;
        text << "cannot create temporal file \'" << tmp_file_name << "\' to save schema";
        throw std::runtime_error(text.str().c_str());
      }
      else {
        if(!p_silence)
          std::cout << "Saving " << numberOfClasses << " classes to schema file \"" << pf->p_full_name << "\"...\n";
      }


      OksXmlOutputStream xmls(f);


        // write oks xml file header

      pf->p_number_of_items = numberOfClasses;
      pf->p_oks_format = "schema";

      pf->write(xmls);


        // write oks classes

      std::ostringstream errors;
      bool found_errors = false;

      if (classes)
        {
          for (auto & i : *classes)
            {
              found_errors |= i.second->check_relationships(errors, false);
              i.second->save(xmls);
            }
        }
      else
        {
          for (auto & i : p_classes)
            {
              if (i.second->p_file == fh)
                {
                  found_errors |= i.second->check_relationships(errors, false);
                  i.second->save(xmls);
                }
            }
        }

      if(found_errors)
        {
          oks::kernel::BindError ex(ERS_HERE, errors.str());

          if(force == false)
            {
              throw std::runtime_error(ex.what());
            }
          else if(p_silence == false)
            {
              ers::warning(ex);
            }
        }

        // close oks xml file

      xmls.put_last_tag("oks-schema", sizeof("oks-schema")-1);


        // flush the buffers

      f.close();
    }

    if(rename(tmp_file_name.c_str(), pf->p_full_name.c_str())) {
      std::ostringstream text;
      text << "cannot rename \'" << tmp_file_name << "\' to \'" << pf->p_full_name << '\'';
      throw std::runtime_error(text.str().c_str());
    }

    tmp_file_name.erase(0);

    if(pf != p_active_schema) {
      try {
        pf->unlock();
      }
      catch(oks::exception& ex) {
        throw std::runtime_error(ex.what());
      }
    }
    pf->p_is_updated = false;
    pf->update_status_of_file();

  }
  catch (oks::exception & ex) {
    if(pf != p_active_schema) { try { pf->unlock();} catch(...) {} }
    if(!tmp_file_name.empty()) { unlink(tmp_file_name.c_str()); }
    throw oks::CanNotWriteToFile("k_save_schema", "schema file", pf->p_full_name, ex);
  }
  catch (std::exception & ex) {
    if(pf != p_active_schema) { try { pf->unlock();} catch(...) {} }
    if(!tmp_file_name.empty()) { unlink(tmp_file_name.c_str()); }
    throw oks::CanNotWriteToFile("k_save_schema", "schema file", pf->p_full_name, ex.what());
  }

  OSK_VERBOSE_REPORT("LEAVE " << fname)
}


  // note: the file name is used as a key in the map
  // to rename a file it is necessary to remove file from map, change name and insert it back

void
OksKernel::k_rename_schema(OksFile * pf, const std::string& short_name, const std::string& long_name)
{
  remove_schema_file(pf);
  pf->rename(short_name, long_name);
  add_schema_file(pf);
}


void
OksKernel::save_as_schema(const std::string& new_name, OksFile * pf)
{
  const char _fname[] = "save_as_schema";
  std::string fname = make_fname(_fname, sizeof(_fname)-1, new_name, nullptr, &pf);
  OSK_VERBOSE_REPORT("ENTER " << fname)

  try {

    if(!new_name.length()) {
      throw std::runtime_error("the new filename is empty");
    }

    std::string old_short_name = pf->p_short_name;
    std::string old_full_name  = pf->p_full_name;

    std::unique_lock lock(p_kernel_mutex);

    k_rename_schema(pf, new_name, new_name);

    try {
      k_save_schema(pf);
    }
    catch (...) {
      k_rename_schema(pf, old_short_name, old_full_name);
      throw;
    }

  }
  catch (oks::exception & ex) {
    throw oks::CanNotWriteToFile("k_save_as_schema", "schema file", pf->p_full_name, ex);
  }
  catch (std::exception & ex) {
    throw oks::CanNotWriteToFile("k_save_as_schema", "schema file", pf->p_full_name, ex.what());
  }

  OSK_VERBOSE_REPORT("LEAVE " << fname)
}

void
OksKernel::save_all_schema()
{
  OSK_VERBOSE_REPORT("ENTER OksKernel::save_all_schema()")

  {
    std::shared_lock lock(p_kernel_mutex);

    for(OksFile::Map::iterator i = p_schema_files.begin(); i != p_schema_files.end(); ++i) {
      if(check_read_only(i->second) == false) {
        k_save_schema(i->second);
      }
      else {
        TLOG_DEBUG(2) << "skip read-only schema file \'" << *(i->first) << '\'';
      }

    }

  }

  OSK_VERBOSE_REPORT("LEAVE OksKernel::save_all_schema()")
}


  // user-allowed method

void
OksKernel::close_schema(OksFile * pf)
{
  std::unique_lock lock(p_kernel_mutex);
  k_close_schema(pf);
}

  // kernel method

void
OksKernel::k_close_schema(OksFile * pf)
{
  OSK_PROFILING(OksProfiler::KernelCloseSchema, this)

  if(!pf) {
    TLOG_DEBUG(1) << "enter for file (null)";
    return;
  }
  else {
    TLOG_DEBUG(2) << "enter for file " << pf->p_full_name;
  }

  if(p_active_schema == pf) p_active_schema = 0;

  try {
    pf->unlock();
  }
  catch(oks::exception& ex) {
    Oks::error_msg("OksKernel::k_close_schema()") << ex.what() << std::endl;
  }

  if(!p_silence)
    std::cout << (pf->p_included_by ? " * c" : "C") << "lose OKS schema \"" << pf->p_full_name << "\"..." << std::endl;

  if(!p_classes.empty()) {

      //
      // The fastest way to delete classes is delete
      // them in order of superclasses descreasing
      // because this reduces amount of work to restructure
      // child classes when their parent was deleted and
      // there will be no dangling classes
      //

    std::set<OksClass *> sorted;

    for(OksClass::Map::iterator i = p_classes.begin(); i != p_classes.end(); ++i) {
      OksClass *c = i->second;
      if(pf == c->p_file) sorted.insert(c);
      c->p_to_be_deleted = true;
    }

    for(std::set<OksClass *>::iterator j = sorted.begin(); j != sorted.end(); ++j) {
      delete *j;
    }
  }

  remove_schema_file(pf);
  delete pf;

  TLOG_DEBUG(4) << "exit for file " << (void *)pf;
}

void
OksKernel::close_all_schema()
{
  TLOG_DEBUG(4) << "enter";

  {
    std::unique_lock lock(p_kernel_mutex);

    for(OksClass::Map::iterator i = p_classes.begin(); i != p_classes.end(); ++i) {
      i->second->p_to_be_deleted = true;
    }

    while(!p_schema_files.empty()) {
      k_close_schema(p_schema_files.begin()->second);
    }
  }

  TLOG_DEBUG(4) << "exit";
}

void
OksKernel::set_active_schema(OksFile * f)
{
  std::unique_lock lock(p_kernel_mutex);
  k_set_active_schema(f);
}

void
OksKernel::k_set_active_schema(OksFile * f)
{
  TLOG_DEBUG(4) << "enter for file " << (void *)f;

    // check if active schema is different from given file

  if(p_active_schema == f) return;


    // unlock current active schema, if it was saved

  if(p_active_schema && p_active_schema->is_updated() == false) {
    try {
      p_active_schema->unlock();
    }
    catch(oks::exception& ex) {
      throw oks::CanNotSetActiveFile("schema", f->get_full_file_name(), ex);
    }
  }


    // exit, if file is (null)

  if(!f) {
    p_active_schema = 0;
    return;    
  }

  try {
    f->lock();
    p_active_schema = f;
  }
  catch(oks::exception& ex) {
    throw oks::CanNotSetActiveFile("schema", f->get_full_file_name(), ex);
  }

  TLOG_DEBUG(4) << "exit for file " << (void *)f;
}

std::list<OksClass *> *
OksKernel::create_list_of_schema_classes(OksFile * pf) const
{
  std::string fname("OksKernel::create_list_of_schema_classes(");
  fname.append(pf->get_full_file_name());
  fname.push_back('\'');

  OSK_PROFILING(OksProfiler::KernelCreateSchemaClassList, this)
  OSK_VERBOSE_REPORT("ENTER " << fname)

  std::list<OksClass *> * clist = nullptr;

  if(!p_classes.empty()) {
    for(OksClass::Map::const_iterator i = p_classes.begin(); i != p_classes.end(); ++i) {
      if(pf == i->second->p_file) {
        if(!clist && !(clist = new std::list<OksClass *>())) {
          return 0;
        }

        clist->push_back(i->second);
      }
    }
  }

  OSK_VERBOSE_REPORT("LEAVE " << fname)

  return clist;
}

/******************************************************************************/

static void
create_updated_lists(const OksFile::Map& files, std::list<OksFile *> ** ufs, std::list<OksFile *> ** rfs, OksFile::FileStatus wu, OksFile::FileStatus wr)
{
  *ufs = 0;  // updated files
  *rfs = 0;  // removed files

  for(OksFile::Map::const_iterator i = files.begin(); i != files.end(); ++i) {
    OksFile::FileStatus fs = i->second->get_status_of_file();

    if(fs == wu /* i.e. Which Updated */ ) {
      if(*ufs == 0) { *ufs = new std::list<OksFile *>(); }
      (*ufs)->push_back(i->second);
    }
    else if(fs == wr /* i.e. Which Removed */ ) {
      if(*rfs == 0) { *rfs = new std::list<OksFile *>(); }
      (*rfs)->push_back(i->second);
    }
  }
}

void
OksKernel::create_lists_of_updated_schema_files(std::list<OksFile *> ** ufs, std::list<OksFile *> ** rfs) const
{
  std::shared_lock lock(p_kernel_mutex);

  create_updated_lists(p_schema_files, ufs, rfs, OksFile::FileModified, OksFile::FileRemoved);
}

void
OksKernel::create_lists_of_updated_data_files(std::list<OksFile *> ** ufs, std::list<OksFile *> ** rfs) const
{
  std::shared_lock lock(p_kernel_mutex);

  create_updated_lists(p_data_files, ufs, rfs, OksFile::FileModified, OksFile::FileRemoved);
}

void
OksKernel::get_modified_files(std::set<OksFile *>& mfs, std::set<OksFile *>& rfs, const std::string& version)
{
  std::list<OksFile *> * files[] =
    { nullptr, nullptr, nullptr, nullptr };

  std::set<OksFile *> * sets[] =
    { &mfs, &rfs };

  create_lists_of_updated_data_files(&files[0], &files[1]);
  create_lists_of_updated_schema_files(&files[2], &files[3]);

  for (int i = 0; i < 4; ++i)
    if (std::list<OksFile *> * f = files[i])
      {
        while (!f->empty())
          {
            sets[i % 2]->insert(f->front());
            f->pop_front();
          }
        delete f;
      }

  if (!get_user_repository_root().empty() && !version.empty())
    for (const auto& file_name : get_repository_versions_diff(get_repository_version(), version))
      {
        if (OksFile * f = find_data_file(file_name))
          sets[0]->insert(f);
        else if (OksFile * f = find_schema_file(file_name))
          sets[0]->insert(f);
      }
}

/******************************************************************************/


namespace oks {

  void LoadErrors::add_parents(std::string& text, const OksFile * file, std::set<const OksFile *>& parents)
  {
    if(file) {
      if(parents.insert(file).second == false) {
        text += "(ignoring circular dependency between included files...)\n";
      }
      else {
        add_parents(text, file->get_parent(), parents);
        text += "file \'";
        text += file->get_full_file_name();
        text += "\' includes:\n";
      }
    }
  }

  void LoadErrors::add_error(const OksFile& file, std::exception& ex)
  {
    std::string text;
    std::set<const OksFile *> parents;
    add_parents(text, file.get_parent(), parents);

    bool has_no_parents(text.empty());

    text += "file \'";
    text += file.get_full_file_name();
    text += (has_no_parents ? "\' has problem:\n" : "\' that has problem:\n");
    text += ex.what();

    std::lock_guard lock(p_mutex);
    m_errors.push_back(text);
  }

  std::string LoadErrors::get_text()
  {
    std::lock_guard lock(p_mutex);

    {
      std::ostringstream s;

      if(m_errors.size() == 1) {
        s << "Found 1 error parsing OKS data:\n" << *m_errors.begin();
      }
      else {
        s << "Found " << m_errors.size() << " errors parsing OKS data:";
        int j = 1;
        for(std::list<std::string>::const_iterator i = m_errors.begin(); i != m_errors.end(); ++i) {
          s << "\nERROR [" << j++ << "] ***: " << *i; 
        }
      }

      m_error_string = s.str();
    }

    return m_error_string;
  }

}


struct OksLoadObjectsJob : public OksJob
{
  public:

    OksLoadObjectsJob( OksKernel * kernel, OksFile * fp, std::shared_ptr<OksXmlInputStream> xmls, char format)
      : m_kernel (kernel),
        m_fp     (fp),
        m_xmls   (xmls),
        m_format (format)
    { ; }       


    void run()
    {
      try {
        OksAliasTable alias_table;
        oks::ReadFileParams read_params( m_fp, *m_xmls, ((m_format == 'X') ? 0 : &alias_table), m_kernel, m_format, 0 );

        m_fp->p_number_of_items = 0;

        try {
          while(OksObject::read(read_params)) {
            m_fp->p_number_of_items++;
          }
        }
        catch(oks::FailedCreateObject & ex) {
	  m_kernel->p_load_errors.add_error(*m_fp, ex);
	  return;
        }

        m_fp->p_size = m_xmls->get_position();
      }
      catch (std::exception& ex) {
	m_kernel->p_load_errors.add_error(*m_fp, ex);
      }
    }


  private:

    OksKernel * m_kernel;
    OksFile * m_fp;
    std::shared_ptr<OksXmlInputStream> m_xmls;
    char m_format;


      // protect usage of copy constructor and assignment operator

  private:

    OksLoadObjectsJob(const OksLoadObjectsJob&);
    OksLoadObjectsJob& operator=(const OksLoadObjectsJob &);

};

/******************************************************************************/

static bool _find_file(const OksFile::Map & files, const OksFile * f)
{
  for(OksFile::Map::const_iterator i = files.begin(); i != files.end(); ++i) {
    if(i->second == f) return true;
  }
  return false;
}

void
oks::ReloadObjects::put(OksObject * o)
{
  config::map<OksObject *> *& c = data[o->uid.class_id];
  if(!c) c = new config::map<OksObject *>();
  (*c)[o->GetId()] = o;
}

OksObject *
oks::ReloadObjects::pop(const OksClass* c, const std::string& id)
{
  std::map< const OksClass *, config::map<OksObject *> * >::iterator i = data.find(c);
  if(i != data.end()) {
    config::map<OksObject *>::iterator j = i->second->find(id);
    if(j != i->second->end()) {
      OksObject * o = j->second;
      i->second->erase(j);
      if(i->second->empty()) {
        delete i->second;
	data.erase(i);
      }
      return o;
    }
  }

  return 0;
}

oks::ReloadObjects::~ReloadObjects()
{
  for(std::map< const OksClass *, config::map<OksObject *> * >::iterator i = data.begin(); i != data.end(); ++i) {
    delete i->second;
  }
}

void
OksKernel::reload_data(std::set<OksFile *>& files_h, bool allow_schema_extension)
{
  std::map<OksFile *, std::vector<std::string> > included;

  std::set<OksFile *>::const_iterator i;

  bool check_includes(false);
  bool found_schema_files(false);
  std::string file_names;

  std::unique_lock lock(p_kernel_mutex);

  for(std::set<OksFile *>::const_iterator fi = files_h.begin(); fi != files_h.end();) {
    if(_find_file(p_schema_files, *fi)) {
      found_schema_files = true;
    }
    else if(!_find_file(p_data_files, *fi)) {
      Oks::error_msg("OksKernel::reload_data") << "file " << (void *)(*fi) << " is not OKS data or schema file, skip..." << std::endl;
      files_h.erase(fi++);
      continue;
    }

    if(fi != files_h.begin()) file_names.append(", ");
    file_names.push_back('\"');
    file_names.append((*fi)->get_full_file_name());
    file_names.push_back('\"');
    ++fi;
  }

  try {

      // throw exception on schema_files

    if(found_schema_files) {
      throw std::runtime_error("Reload of modified schema files is not supported");
    }

      // exit, if there are no files to reload (e.g. dangling refs.)

    if(files_h.empty()) return;


      // unlock any locked file
      // build container of objects which can be updated (and removed, if not reloaded)

    oks::ReloadObjects reload_objects;

    for(i = files_h.begin(); i != files_h.end(); ++i) {
      if((*i)->is_locked()) {
        if(p_active_data == (*i)) { p_active_data = 0; }
        (*i)->unlock();
      }

      for(OksObject::Set::const_iterator oi = p_objects.begin(); oi != p_objects.end(); ++oi) {
        if((*oi)->file == *i) reload_objects.put(*oi);
      }
    }


      // preload includes of modified files

    {
      std::set<OksFile *> new_files;

      for(i = files_h.begin(); i != files_h.end(); ++i) {
        try {
          check_includes |= k_preload_includes(*i, new_files, allow_schema_extension);
        }
        catch(...) {
          restore_preload_file_info();
          throw;
        }
      }

      clear_preload_file_info();

      if(!new_files.empty()) {
        TLOG_DEBUG(2) << new_files.size() << " new files will be loaded";
      }

      for(i = new_files.begin(); i != new_files.end(); ++i) {
        files_h.insert(*i);
      }
    }

      // build list of files to be closed:
      // close files which are no more referenced by others
      // this may happen if there are changes in the list of includes

    std::set<OksFile *> files_to_be_closed;

    if(check_includes) {
      size_t num_of_closing_files = 0;

      while(true) {

          // build list of good (i.e. "included") files
          // rebuild the list, if at least one file was excluded

        std::map<std::string, OksFile *> good_files;
    
        for(OksFile::Map::iterator j = p_data_files.begin(); j != p_data_files.end(); ++j) {
          if(files_to_be_closed.find(j->second) == files_to_be_closed.end()) {
            for(std::list<std::string>::iterator l = j->second->p_list_of_include_files.begin(); l != j->second->p_list_of_include_files.end(); ++l) {
              try {
	        good_files[get_file_path(*l, j->second)] = j->second;
              }
              catch (...) {
              }
            }
          }
        }

        for(OksFile::Map::iterator i = p_data_files.begin(); i != p_data_files.end(); ) {
          if(i->second->p_included_by && files_to_be_closed.find(i->second) == files_to_be_closed.end()) {
            TLOG_DEBUG(3) << "check the file \'" << i->second->get_full_file_name() << "\' is included";
            const std::string& s = i->second->get_full_file_name();
            OksFile * f2 = i->second;
            ++i;

            f2->p_included_by = 0;

              // search for a file which could include given one

            std::map<std::string, OksFile *>::const_iterator j = good_files.find(s);

	    if(j != good_files.end()) {
	      f2->p_included_by = j->second;
	    }
            else {
              files_to_be_closed.insert(f2);

              for(OksObject::Set::const_iterator oi = p_objects.begin(); oi != p_objects.end(); ++oi) {
                if((*oi)->file == f2) {
		  reload_objects.put(*oi); 
	        }
              }

              if (files_h.erase(f2)) {
                TLOG_DEBUG(2) << "skip reload of updated file \'" << f2->get_full_file_name() << " since it will be closed";
              }

              TLOG_DEBUG(3) << "file \'" << f2->get_full_file_name() << " will be closed";
            }
          }
          else {
            ++i;
          }
        }

	if(num_of_closing_files == files_to_be_closed.size()) {
	  break;
	}
	else {
	  num_of_closing_files = files_to_be_closed.size();
	}
      }
    }
    else {
      TLOG_DEBUG(2) << "no changes in the list of includes";
    }

      // remove exclusive RCRs (will be restored when read, if object was not changed)

    {
      for(std::map< const OksClass *, config::map<OksObject *> * >::const_iterator cx = reload_objects.data.begin(); cx != reload_objects.data.end(); ++cx) {
        const OksClass * c(cx->first);
	if(c->p_all_relationships && !c->p_all_relationships->empty()) {
	  const unsigned int atts_num(c->number_of_all_attributes());
	  for(config::map<OksObject *>::const_iterator j = cx->second->begin(); j != cx->second->end(); ++j) {
	    OksObject * obj = j->second;
            OksData * d(obj->data + atts_num);

            for(std::list<OksRelationship *>::iterator i = c->p_all_relationships->begin(); i != c->p_all_relationships->end(); ++i, ++d) {
              OksRelationship * r = *i;
              if(r->get_is_composite() && r->get_is_exclusive()) {
                if(d->type == OksData::object_type && d->data.OBJECT) {
                  if(!is_dangling(d->data.OBJECT)) {
                    d->data.OBJECT->remove_RCR(obj, r);
                  }
                }
                else if(d->type == OksData::list_type && d->data.LIST) {
                  for(OksData::List::iterator i2 = d->data.LIST->begin(); i2 != d->data.LIST->end(); ++i2) {
                    OksData *d2(*i2);
                    if(d2->type == OksData::object_type && d2->data.OBJECT) {
                      if(!is_dangling(d2->data.OBJECT)) {
                        d2->data.OBJECT->remove_RCR(obj, r);
                      }
                    }
                  }
                }
	      }
            }	    
	  }
	}
      }
    }

      // need to allow "duplicated_objects_via_inheritance_mode", since the objects with the same ID can be created and removed

    bool duplicated_objs_mode = get_test_duplicated_objects_via_inheritance_mode();
    set_test_duplicated_objects_via_inheritance_mode(false);


      // read objects

    for(i = files_h.begin(); i != files_h.end(); ++i) {
      std::shared_ptr<std::ifstream> f(new std::ifstream((*i)->get_full_file_name().c_str()));

      std::shared_ptr<OksXmlInputStream> xmls(new OksXmlInputStream(f));

      OksFile fp(xmls, (*i)->get_short_file_name(), (*i)->get_full_file_name(), this);
      char format = (fp.p_oks_format == "data" ? 'n' : ((fp.p_oks_format == "extended") ? 'X': 'c'));

      if(!p_silence) {
        std::lock_guard lock(p_parallel_out_mutex);
        std::cout << " * reading data file \"" << (*i)->get_full_file_name() << "\"..." << std::endl;
      }

      {
        OksAliasTable alias_table;
        oks::ReadFileParams read_params( *i, *xmls, ((format == 'X') ? 0 : &alias_table), this, format, &reload_objects );

        try {
          while(OksObject::read(read_params)) { ; }
        }
        catch(oks::FailedCreateObject & ex) {
          set_test_duplicated_objects_via_inheritance_mode(duplicated_objs_mode);
          throw ex;
        }
      }

      (*i)->update_status_of_file();
      (*i)->p_is_updated = false;
    }

    set_test_duplicated_objects_via_inheritance_mode(duplicated_objs_mode);


      // remove objects which were not re-read

    OksObject::FSet oset;

    {
      for(std::map< const OksClass *, config::map<OksObject *> * >::const_iterator cx = reload_objects.data.begin(); cx != reload_objects.data.end(); ++cx) {
        for(config::map<OksObject *>::const_iterator ox = cx->second->begin(); ox != cx->second->end(); ++ox) {
	  oset.insert(ox->second);
	}
      }

#ifndef ERS_NO_DEBUG
      if(ers::debug_level() >= 3) {
        std::ostringstream text;
	text << "there are " << oset.size() << " removed objects:\n";

        for(OksObject::FSet::iterator x = oset.begin(); x != oset.end(); ++x) {
	  text << " - object " << *x << std::endl;
          TLOG_DEBUG(3) << text.str();
        }
      }
#endif

      {
        OksObject::FSet refs;
        unbind_all_rels(oset, refs);
        if(p_change_object_notify_fn) {
          for(OksObject::FSet::const_iterator x2 = refs.begin(); x2 != refs.end(); ++x2) {
            TLOG_DEBUG(3) << "*** add object " << *x2 << " to the list of updated *** ";
            (*p_change_object_notify_fn)(*x2, p_change_object_notify_param);
          }
        }
      }
    }

    for(OksObject::FSet::iterator ox = oset.begin(); ox != oset.end(); ++ox) {
      OksObject * o = *ox;

      if(is_dangling(o)) {
        TLOG_DEBUG(4) << "skip dangling object " << (void *)o;
        continue;
      }

      TLOG_DEBUG(3) << "*** remove non-reloaded object " << o << " => " << (void *)o << " ***";

      delete o;
    }


    for(std::set<OksFile *>::const_iterator x = files_to_be_closed.begin(); x != files_to_be_closed.end(); ++x) {
      k_close_data(*x, true);
    }

    k_bind_objects();


    // check that created objects do not have duplicated IDs within inheritance hierarchy

    if (duplicated_objs_mode == true)
      {
        for (auto& o : reload_objects.created)
          {
            o->check_ids();
          }
      }
  }
  catch (oks::exception & e) {
    throw oks::FailedReloadFile(file_names, e);
  }
  catch (std::exception & e) {
    throw oks::FailedReloadFile(file_names, e.what());
  }
}

void
OksKernel::clear_preload_file_info()
{
  for(std::map<const OksFile *, OksFile *>::iterator i = p_preload_file_info.begin(); i != p_preload_file_info.end(); ++i) {
    delete i->second;
  }

  p_preload_file_info.clear();
  p_preload_added_files.clear();
}

void
OksKernel::restore_preload_file_info()
{
  for(std::vector<OksFile *>::iterator j = p_preload_added_files.begin(); j != p_preload_added_files.end(); ++j) {
    TLOG_DEBUG( 1 ) << "remove file " << (*j)->get_full_file_name() ;
    remove_data_file(*j);
    delete *j;
  }

  for(OksFile::Map::iterator i = p_data_files.begin(); i != p_data_files.end(); ++i) {
    std::map<const OksFile *, OksFile *>::iterator j = p_preload_file_info.find(i->second);
    if(j != p_preload_file_info.end()) {
      TLOG_DEBUG( 1 ) << "restore file " << i->second->get_full_file_name() ;
      (*i->second) = (*j->second);
    }
  }

  clear_preload_file_info();
}


  // user-allowed method

OksFile *
OksKernel::load_data(const std::string& short_file_name, bool bind)
{
  std::unique_lock lock(p_kernel_mutex);
  return k_load_data(short_file_name, bind, 0, 0);
}


  // kernel method

OksFile *
OksKernel::k_load_data(const std::string& short_file_name, bool bind, const OksFile * parent_h, OksPipeline * pipeline)
{
  const char _fname[] = "k_load_data";
  std::string fname = make_fname(_fname, sizeof(_fname)-1, short_file_name, &bind, &parent_h);

  OSK_VERBOSE_REPORT("ENTER " << fname)

  std::string full_file_name;

  try {
    full_file_name = get_file_path(short_file_name, parent_h);
  }
  catch (std::exception & e) {
    throw oks::CanNotOpenFile("k_load_data", short_file_name, e.what());
  }

  try {

    OksFile *fp = find_data_file(full_file_name);

    if(fp != 0) {
      if(p_verbose) {
        std::lock_guard lock(p_parallel_out_mutex);
        Oks::warning_msg(fname) << "  The file \'" << full_file_name << "\' was already loaded";
      }
      return fp->check_parent(parent_h);
    }

    {
      std::shared_ptr<std::ifstream> f(new std::ifstream(full_file_name.c_str()));

      if(!f->good()) {
        throw std::runtime_error("k_load_data(): cannot open file");
      }

      long file_length(get_file_length(*f));
      
      if(!file_length) {
        throw std::runtime_error("k_load_data(): file is empty");
      }

      std::shared_ptr<OksXmlInputStream> xmls(new OksXmlInputStream(f));

      fp = new OksFile(xmls, short_file_name, full_file_name, this);

      if(fp->p_oks_format.empty()) {
        throw std::runtime_error("k_load_data(): failed to read header of file");
      }

      char format;

      if(fp->p_oks_format.size() == 4 && oks::cmp_str4n(fp->p_oks_format.c_str(), "data"))
        format = 'n';
      else if(fp->p_oks_format.size() == 8 && oks::cmp_str8n(fp->p_oks_format.c_str(), "extended"))
        format = 'X';
      else if(fp->p_oks_format.size() == 7 && oks::cmp_str7n(fp->p_oks_format.c_str(), "compact"))
        format = 'c';
      else {
        throw std::runtime_error("k_load_data(): file is not an oks data file");
      }

      k_load_data(fp, format, xmls, file_length, bind, parent_h, pipeline);

      OSK_VERBOSE_REPORT("LEAVE " << fname)

      return fp;
    }
  }
  catch (oks::FailedLoadFile&) {
    throw;
  }
  catch (oks::exception & e) {
    throw (oks::FailedLoadFile("data file", full_file_name, e));
  }
  catch (std::exception & e) {
    throw (oks::FailedLoadFile("data file", full_file_name, e.what()));
  }
}


void
OksKernel::k_load_data(OksFile * fp, char format, std::shared_ptr<OksXmlInputStream> xmls, long file_length, bool bind, const OksFile * parent_h, OksPipeline * pipeline)
{
  OSK_PROFILING(OksProfiler::KernelLoadData, this)

  fp->p_included_by = parent_h;
  check_read_only(fp);

  try {
    if(!p_silence) {
      std::lock_guard lock(p_parallel_out_mutex);
      std::cout << (parent_h ? " * r" : "R") << "eading data file \"" << fp->get_full_file_name()
                << "\" in " << (format == 'n' ? "normal" : (format == 'X' ? "extended" : "compact")) << " format (" << file_length << " bytes)...\n";
      if(parent_h == 0 && fp->get_full_file_name() != fp->get_short_file_name()) {
        std::cout << "(non-fully-qualified filename was \"" << fp->get_short_file_name() << "\")\n";
      }
    }

    fp->update_status_of_file();
    add_data_file(fp);

    {
      std::unique_ptr<OksPipeline> pipeline_guard(pipeline ? 0 : new OksPipeline(p_threads_pool_size));

      OksPipeline * m_pipeline;

      if(pipeline) {
        m_pipeline = pipeline;
      }
      else {
        m_pipeline = pipeline_guard.get();
        p_load_errors.clear();
      }

      k_load_includes(*fp, m_pipeline);


      if(p_threads_pool_size > 1) {
        m_pipeline->addJob( new OksLoadObjectsJob( this, fp, xmls, format) );
      }
      else {
        OksLoadObjectsJob job(this, fp, xmls, format);
        job.run();
      }
    }

    if(!pipeline) {
      if(!p_load_errors.is_empty()) {
        throw (oks::FailedLoadFile("data file", fp->get_full_file_name(), p_load_errors.get_text()));
      }

      if(bind) {
        k_bind_objects();
      }
    }
  }
  catch (oks::FailedLoadFile&) {
    throw;
  }
  catch (oks::exception& e) {
    throw (oks::FailedLoadFile("data file", fp->get_full_file_name(), e));
  }
  catch (std::exception& e) {
    throw (oks::FailedLoadFile("data file", fp->get_full_file_name(), e.what()));
  }
}


OksFile *
OksKernel::new_data(const std::string& s, const std::string& logical_name, const std::string& type)
{
  const char _fname[] = "new_data";
  std::string fname = make_fname(_fname, sizeof(_fname)-1, s, nullptr, nullptr);
  OSK_VERBOSE_REPORT("ENTER " << fname)

  if(!s.length()) {
    throw oks::CanNotOpenFile("new_data", s, "file name is empty");
  }

  std::string file_name(s);

  try {

    Oks::substitute_variables(file_name);

    test_file_existence(file_name, p_silence, fname, "data");

    file_name = get_file_path(s, nullptr, false);

    std::unique_lock lock(p_kernel_mutex);

    if(find_data_file(file_name) != 0) {
      throw std::runtime_error("the file is already loaded");
    }

    OksFile * file_h = new OksFile(file_name, logical_name, type, "data", this);

    file_h->p_short_name = s;

    k_set_active_data(file_h);

    add_data_file(p_active_data);

  }
  catch (oks::exception & e) {
    throw oks::CanNotCreateFile("new_data", "data file", file_name, e);
  }
  catch (std::exception & e) {
    throw oks::CanNotCreateFile("new_data", "data file", file_name, e.what());
  }

  OSK_VERBOSE_REPORT("LEAVE " << fname)

  return p_active_data;
}


void
OksKernel::add_data_file(OksFile * f)
{
  p_data_files[&f->p_full_name] = f;
}

void
OksKernel::add_schema_file(OksFile * f)
{
  p_schema_files[&f->p_full_name] = f;
}

void
OksKernel::remove_data_file(OksFile * f)
{
  p_data_files.erase(&f->p_full_name);
}

void
OksKernel::remove_schema_file(OksFile * f)
{
  p_schema_files.erase(&f->p_full_name);
}


  // user-allowed method

void
OksKernel::save_data(OksFile * pf, bool ignoreBadObjects, OksFile * true_file_h, bool force_defaults)
{
  std::shared_lock lock(p_kernel_mutex);
  k_save_data(pf, ignoreBadObjects, true_file_h, nullptr, force_defaults);
}

void
OksKernel::save_data(OksFile * file_h, const OksObject::FSet& objects)
{
  std::shared_lock lock(p_kernel_mutex);
  k_save_data(file_h, false, 0, &objects);
}

void
OksKernel::backup_data(OksFile * pf, const char * suffix)
{
  std::shared_lock lock(p_kernel_mutex);

  OksFile f(
    pf->get_full_file_name() + suffix,
    pf->get_logical_name(),
    pf->get_type(),
    pf->get_oks_format(),
    this
  );

  f.p_created_by = pf->p_created_by;
  f.p_creation_time = pf->p_creation_time;
  f.p_created_on = pf->p_created_on;
  f.p_list_of_include_files = pf->p_list_of_include_files;

  if(!p_silence) {
    std::lock_guard lock(p_parallel_out_mutex);
    std::cout << "Making backup of data file \"" << pf->p_full_name << "\"...\n";
  }

  bool silence = p_silence;

  p_silence = true;

  try {
    k_save_data(&f, true, pf);
  }
  catch(oks::exception& ex) {
    p_silence = silence;
    throw oks::CanNotBackupFile(pf->p_full_name, ex);
  }

  p_silence = silence;
}


  // kernel method

void
OksKernel::k_save_data(OksFile * pf, bool ignoreBadObjects, OksFile * fh, const OksObject::FSet * objects, bool force_defaults)
{
  const char _fname[] = "k_save_data";
  std::string fname = make_fname(_fname, sizeof(_fname)-1, pf->p_full_name, nullptr, nullptr);

  OSK_PROFILING(OksProfiler::KernelSaveData, this)
  OSK_VERBOSE_REPORT("ENTER " << fname)

  std::string tmp_file_name;
  
  if(!fh) fh = pf;

  try {

      // calculate number of objects in file and check objects

    size_t numberOfObjects = 0;

    if(objects) {
      numberOfObjects = objects->size();

      if(!ignoreBadObjects) {
        std::string errors;

        for(OksObject::FSet::const_iterator i = objects->begin(); i != objects->end(); ++i) {
          errors += (*i)->report_dangling_references();
        }

        if(!errors.empty()) {
          std::ostringstream text;
	  text << "the file contains objects with dangling references:\n" << errors;
          throw std::runtime_error(text.str().c_str());
        }
      }
    }
    else {
        // get list of all includes

      std::set<OksFile *> includes;
      pf->get_all_include_files(this, includes);

      OSK_VERBOSE_REPORT("Test consistency of objects in file \"" << pf->p_full_name << '\"')

      bool found_bad_object = false;

      for(OksObject::Set::iterator i = p_objects.begin(); i != p_objects.end(); ++i) {
        if((*i)->file == fh) {
          numberOfObjects++;
	  if(!ignoreBadObjects || !p_silence) {
            if((*i)->is_consistent(includes, "WARNING") == false) {
              found_bad_object = true;
            }
            if((*i)->is_duplicated() == true) {
              if(!p_silence) {
                Oks::error_msg(fname) << "  The file contains duplicated object " << *i << std::endl;
              }
              found_bad_object = true;
            }
          }
        }
      }

      if(found_bad_object && ignoreBadObjects == false) {
	throw std::runtime_error("the file contains inconsistent/duplicated objects or misses includes");
      }
    }


      // lock the file, if it is not locked already

    if(pf->is_locked() == false) {
      pf->lock();
    }


      // check if it possible to open the non-existent file in write mode

    {
        // check first if file already exists

      std::fstream f(pf->p_full_name.c_str(), std::ios::in);

      if(!f) {

          // no check for non-existent file

        std::fstream f2(pf->p_full_name.c_str(), std::ios::out);

        if(!f2) {
	  std::ostringstream text;
          text << "cannot open file \'" << pf->p_full_name << "\' for writing";
          throw std::runtime_error(text.str().c_str());
        }
      }
    }


    tmp_file_name = get_tmp_file(pf->p_full_name);

    long file_len = 0;

    {
      std::ofstream f(tmp_file_name.c_str());

      f.exceptions ( std::ostream::failbit | std::ostream::badbit );

      if(!f) {
        std::ostringstream text;
        text << "cannot create temporal file \'" << tmp_file_name << "\' to save data";
        throw std::runtime_error(text.str().c_str());
      }
      else {
        if(!p_silence) {
	  std::lock_guard lock(p_parallel_out_mutex);
          std::cout << "Saving " << numberOfObjects << " objects " << (force_defaults ? "with enforced default values " : "") << "to data file \"" << pf->p_full_name << "\"...\n";
        }
      }


        // set header parameters

      OksXmlOutputStream xmls(f);

      pf->p_number_of_items = numberOfObjects;
      pf->p_oks_format = "data";

      pf->write(xmls);

      if(!p_objects.empty()) {
        for(OksClass::Map::const_iterator i = p_classes.begin(); i != p_classes.end() && f.good(); ++i) {
          OksObject::SMap sorted;

          for(OksObject::Map::const_iterator j = i->second->p_objects->begin(); j != i->second->p_objects->end(); ++j) {
            if(j->second->file == fh || (objects && (objects->find(j->second) != objects->end()))) {
	      sorted[j->first] = j->second;
	    }
	  }

          for(OksObject::SMap::iterator j = sorted.begin(); j != sorted.end(); ++j) {
	    j->second->put(xmls, force_defaults);
	    xmls.put_raw('\n');
          }
        }
      }

      xmls.put_last_tag("oks-data", sizeof("oks-data")-1);
      file_len = f.tellp();

      f.close();
    }


        // check that the written file is OK
	// FIXME: can be removed later, if exceptions work well enough

    {
      long written_len = 0;

      std::shared_ptr<std::ifstream> f(new std::ifstream(tmp_file_name.c_str()));

      if(*f) {
        f->seekg(0, std::ios::end);
        written_len = static_cast<std::streamoff>(f->tellg());
      }

      if(written_len != file_len) {
        unlink(tmp_file_name.c_str());
        std::ostringstream text;
        text << "write error in file \'" << tmp_file_name << "\': " << written_len << " bytes been written instead of " << file_len;
        throw std::runtime_error(text.str().c_str());
      }
    }


      // remember file's mode

    struct stat buf;
    if(int code = stat(pf->p_full_name.c_str(), &buf)) {
      std::ostringstream text;
      text << "cannot get information about file \'" << pf->p_full_name << "\': stat() failed with code " << code << ", reason = \'" << oks::strerror(errno) << '\'';
      throw std::runtime_error(text.str().c_str());
    }


      // rename temporal file

    if(int code = rename(tmp_file_name.c_str(), pf->p_full_name.c_str())) {
      std::ostringstream text;
      text << "cannot rename file \'" << tmp_file_name << "\' to \'" << pf->p_full_name << "\': rename() failed with code " << code << ", reason = \'" << oks::strerror(errno) << '\'';
      throw std::runtime_error(text.str().c_str());
    }

    tmp_file_name.erase(0);


    if(pf != p_active_data) {
      try {
        pf->unlock();
      }
      catch(oks::exception& ex) {
        throw std::runtime_error(ex.what());
      }
    }
    pf->p_is_updated = false;
    pf->update_status_of_file();


      // get mode for new file

    struct stat buf2;
    if(int code = stat(pf->p_full_name.c_str(), &buf2)) {
      std::ostringstream text;
      text << "cannot get information about file \'" << pf->p_full_name << "\': stat() failed with code " << code << ", reason = \'" << oks::strerror(errno) << '\'';
      throw std::runtime_error(text.str().c_str());
    }


      // set file's mode if needed

    if(buf.st_mode != buf2.st_mode) {
      if(int code = chmod(pf->p_full_name.c_str(), buf.st_mode) != 0) {
        std::ostringstream text;
        text << "cannot set protection mode for file \'" << pf->p_full_name << "\': chmod() failed with code " << code << ", reason = \'" << oks::strerror(errno) << '\'';
        throw std::runtime_error(text.str().c_str());
      }
    }

      // set file's group if needed (in case of problems report error, but do not throw exception)

    if(buf.st_gid != buf2.st_gid) {
      if(int code = chown(pf->p_full_name.c_str(), (gid_t)(-1), buf.st_gid) != 0) {
        ers::warning(oks::kernel::SetGroupIdFailed(ERS_HERE, buf.st_gid, pf->p_full_name.c_str(), code, oks::strerror(errno)));
      }
    }

  }

  catch (oks::exception & e) {
    if(pf != p_active_data) { try { pf->unlock();} catch(...) {} }
    if(!tmp_file_name.empty()) { unlink(tmp_file_name.c_str()); }
    throw oks::CanNotWriteToFile("k_save_data", "data file", pf->p_full_name, e);
  }
  catch (std::exception & e) {
    if(pf != p_active_data) { try { pf->unlock();} catch(...) {} }
    if(!tmp_file_name.empty()) { unlink(tmp_file_name.c_str()); }
    throw oks::CanNotWriteToFile("k_save_data", "data file", pf->p_full_name, e.what());
  }
  catch (...) {
    if(pf != p_active_data) { try { pf->unlock();} catch(...) {} }
    if(!tmp_file_name.empty()) { unlink(tmp_file_name.c_str()); }
    throw oks::CanNotWriteToFile("k_save_data", "data file", pf->p_full_name, "unknown");
  }

  OSK_VERBOSE_REPORT("LEAVE " << fname)
}


  // note: the file name is used as a key in the map
  // to rename a file it is necessary to remove file from map, change name and insert it back

void
OksKernel::k_rename_data(OksFile * pf, const std::string& short_name, const std::string& long_name)
{
  remove_data_file(pf);
  pf->rename(short_name, long_name);
  add_data_file(pf);
}

void
OksKernel::save_as_data(const std::string& new_name, OksFile * pf)
{
  const char _fname[] = "save_as_data";
  std::string fname = make_fname(_fname, sizeof(_fname)-1, new_name, nullptr, &pf);
  OSK_VERBOSE_REPORT("ENTER " << fname)

  try {

    if(!new_name.length()) {
      throw std::runtime_error("the new filename is empty");
    }

    std::unique_lock lock(p_kernel_mutex);

    std::string old_short_name = pf->p_short_name;
    std::string old_full_name  = pf->p_full_name;
    std::string old_format     = pf->p_oks_format;

    k_rename_data(pf, new_name, new_name);

    try {
      k_save_data(pf);
    }
    catch (...) {
      k_rename_data(pf, old_short_name, old_full_name);
      pf->p_oks_format = old_format;
      throw;
    }

  }
  catch (oks::exception & ex) {
    throw oks::CanNotWriteToFile("k_save_as_data", "data file", new_name, ex);
  }
  catch (std::exception & ex) {
    throw oks::CanNotWriteToFile("k_save_as_data", "data file", new_name, ex.what());
  }

  OSK_VERBOSE_REPORT("LEAVE " << fname)
}


void
OksKernel::save_all_data()
{
  TLOG_DEBUG(4) << "enter";

  {
    std::shared_lock lock(p_kernel_mutex);

    for(OksFile::Map::iterator i = p_data_files.begin(); i != p_data_files.end(); ++i) {
      if(check_read_only(i->second) == false) {
        k_save_data(i->second);
      }
      else {
        TLOG_DEBUG(2) << "skip read-only data file \'" << *(i->first) << '\'';
      }
    }
  }

  TLOG_DEBUG(4) << "exit";
}

  // user-allowed method

void
OksKernel::close_data(OksFile * fp, bool unbind)
{
  std::unique_lock lock(p_kernel_mutex);
  k_close_data(fp, unbind);
}

  // kernel method

void
OksKernel::k_close_data(OksFile * fp, bool unbind)
{
  const char _fname[] = "k_close_data";
  std::string fname = make_fname(_fname, sizeof(_fname)-1, fp->p_full_name, &unbind, nullptr);

  OSK_PROFILING(OksProfiler::KernelCloseData, this)
  OSK_VERBOSE_REPORT("ENTER " << fname)

  try {
    fp->unlock();
  }
  catch(oks::exception& ex) {
    Oks::error_msg(fname) << ex.what() << std::endl;
  }

  if(!p_silence) {
    std::lock_guard lock(p_parallel_out_mutex);
    std::cout << (fp->p_included_by ? " * c" : "C") << "lose OKS data \"" << fp->p_full_name << "\"..." << std::endl;
  }

  if(unbind) {
    for(OksObject::Set::const_iterator i = p_objects.begin(); i != p_objects.end(); ++i) {
      OksObject *o = *i;
      if(o->file != fp) o->unbind_file(fp);
    }
  }

  if(p_close_all == false) {
    std::list<OksObject *> * olist = create_list_of_data_objects(fp);

    if(olist) {
      while(!olist->empty()) {
        OksObject *o = olist->front();
        olist->pop_front();
        if(!is_dangling(o)) delete o;
      }

      delete olist;
    }
  }

  remove_data_file(fp);
  delete fp;

  if(p_active_data == fp) p_active_data = 0;

  OSK_VERBOSE_REPORT("LEAVE " << fname)
}


void
OksKernel::close_all_data()
{
  TLOG_DEBUG(4) << "enter";

  {
    std::unique_lock lock(p_kernel_mutex);

    p_close_all = true;

    while(!p_data_files.empty()) {
      k_close_data(p_data_files.begin()->second, false);
    }

    if(!p_objects.empty()) {
      for(OksObject::Set::iterator i = p_objects.begin(); i != p_objects.end(); ++i) {
        delete *i;
      }

      p_objects.clear();

      for(OksClass::Map::const_iterator i = p_classes.begin(); i != p_classes.end(); ++i) {
        if(i->second->p_objects) {
          delete i->second->p_objects;
	  i->second->p_objects = 0;
        }
      }
    }

    p_close_all = false;
  }

  TLOG_DEBUG(4) << "exit";
}

void
OksKernel::set_active_data(OksFile * fp)
{
  std::unique_lock lock(p_kernel_mutex);
  k_set_active_data(fp);
}

void
OksKernel::k_set_active_data(OksFile * fp)
{
  TLOG_DEBUG(4) << "enter for file " << (void *)fp;


    // check if active data is different from given file

  if(p_active_data == fp) return;


    // first unlock current active data

  if(p_active_data && p_active_data->is_updated() == false) {
    try {
      p_active_data->unlock();
    }
    catch(oks::exception& ex) {
      throw oks::CanNotSetActiveFile("data", fp->get_full_file_name(), ex);
    }
  }


    // exit, if file is (null)

  if(!fp) {
    p_active_data = 0;
    return;    
  }

  try {
    fp->lock();
    p_active_data = fp;
  }
  catch(oks::exception& ex) {
    throw oks::CanNotSetActiveFile("data", fp->get_full_file_name(), ex);
  }

  TLOG_DEBUG(4) << "leave for file " << (void *)fp;
}


std::list<OksObject *> *
OksKernel::create_list_of_data_objects(OksFile * fp) const
{
  const char _fname[] = "create_list_of_data_objects";
  std::string fname = make_fname(_fname, sizeof(_fname)-1, fp->get_full_file_name(), nullptr, nullptr);

  OSK_PROFILING(OksProfiler::KernelCreateDataObjectList, this)
  OSK_VERBOSE_REPORT("ENTER " << fname)


  std::list<OksObject *> * olist = new std::list<OksObject *>();

  if(!p_objects.empty()) {
    for(OksObject::Set::const_iterator i = p_objects.begin(); i != p_objects.end(); ++i) {
      if((*i)->file == fp) olist->push_back(*i);
    }
  }

  if(olist->empty()) {
    delete olist;
    olist = 0;
  }


  OSK_VERBOSE_REPORT("LEAVE " << fname)

  return olist;
}


/******************************************************************************/

  // user-allowed method

void
OksKernel::bind_objects()
{
  std::unique_lock lock(p_kernel_mutex);
  k_bind_objects();
}

void
OksKernel::k_bind_objects()
{
  TLOG_DEBUG(4) << "enter";

  p_bind_objects_status.clear();

  if(!p_objects.empty()) {
    for(OksObject::Set::iterator i = p_objects.begin(); i != p_objects.end(); ++i) {
      try {
        (*i)->bind_objects();
      }
      catch(oks::ObjectBindError& ex) {
        if(ex.p_is_error) {
	  throw;
	}
	else {
	  const std::string error_text(strchr(ex.what(), '\n')+1);
          if(!p_bind_objects_status.empty()) p_bind_objects_status.push_back('\n');
            p_bind_objects_status.append(error_text);

	    TLOG_DEBUG(1) << error_text;
	}
      }
    }
  }

  if(!p_silence && !p_bind_objects_status.empty()) {
    ers::warning(oks::kernel::BindError(ERS_HERE, p_bind_objects_status));
  }

  TLOG_DEBUG(4) << "exit with status:\n" << p_bind_objects_status;
}


void
OksKernel::unbind_all_rels(const OksObject::FSet& rm_objs, OksObject::FSet& updated) const
{
  const OksClass::Map& all_classes(classes());

  for(OksClass::Map::const_iterator i = all_classes.begin(); i != all_classes.end(); ++i) {
    OksClass * c(i->second);
    if(const OksObject::Map * objs = i->second->objects()) {
      for(OksObject::Map::const_iterator j = objs->begin(); j != objs->end(); ++j) {
        OksObject *o(j->second);
        unsigned short l1 = c->number_of_all_attributes();
        unsigned short l2 = l1 + c->number_of_all_relationships();

        while(l1 < l2) {
          OksDataInfo odi(l1, (OksRelationship *)0);
          OksData * d(o->GetRelationshipValue(&odi));

          if(d->type == OksData::object_type) {
            if(rm_objs.find(d->data.OBJECT) != rm_objs.end()) {
              updated.insert(o);
              const OksClass * __c(d->data.OBJECT->GetClass());
              const OksString& __id(d->data.OBJECT->GetId());
              d->Set(__c,__id);
              TLOG_DEBUG(5) << "set relationship of " << o << ": " << *d;
            }
          }
          else if(d->type == OksData::list_type) {
            for(OksData::List::const_iterator li = d->data.LIST->begin(); li != d->data.LIST->end(); ++li) {
              OksData * lid(*li);
              if(lid->type == OksData::object_type) {
                if(rm_objs.find(lid->data.OBJECT) != rm_objs.end()) {
		  updated.insert(o);
                  const OksClass * __c(lid->data.OBJECT->GetClass());
                  const OksString& __id(lid->data.OBJECT->GetId());
		  lid->Set(__c,__id);
                  TLOG_DEBUG(5) << "set relationship of " << o << ": " << *d;
                }
              }
            }
          }
          ++l1;
        }
      }
    }
  }
}


OksClass *
OksKernel::find_class(const char * name) const
{
  OksClass::Map::const_iterator i = p_classes.find(name);
  return (i != p_classes.end() ? i->second : 0);
}


void
OksKernel::k_add(OksClass *c)
{
  TLOG_DEBUG(4) << "enter for class \"" << c->get_name() << '\"';

  if(p_active_schema == 0) {
    throw oks::CannotAddClass(*c, "no active schema");
  }

  if(p_classes.find(c->get_name().c_str()) != p_classes.end()) {
    throw oks::CannotAddClass(*c, "class already exists");
  }

  c->p_kernel = this;
  c->p_file = p_active_schema;
  c->p_file->p_is_updated = true;

  p_classes[c->get_name().c_str()] = c;

  try {
    registrate_all_classes(false);
  }
  catch(oks::exception& ex) {
    throw oks::CannotAddClass(*c, ex);
  }

  if(OksClass::create_notify_fn) (*OksClass::create_notify_fn)(c);
}

void
OksKernel::k_remove(OksClass *c)
{
  TLOG_DEBUG(4) << "enter for class \"" << c->get_name() << '\"';

  if(OksClass::delete_notify_fn) (*OksClass::delete_notify_fn)(c);

  p_classes.erase(c->get_name().c_str());
}


void
OksKernel::registrate_all_classes(bool skip_registered)
{
  std::unique_lock lock(p_schema_mutex);  // protect schema and all objects from changes

  if(size_t num_of_classes = p_classes.size()) {
    OksClass ** table = new OksClass * [num_of_classes];
    size_t array_size = num_of_classes * sizeof(OksClass*) / sizeof(wchar_t);  // is used by wmemset() to init above array with NULLs

    OksClass::Map::iterator i = p_classes.begin();
    unsigned long idx(0);

    for(; i != p_classes.end(); ++i) i->second->registrate_class(skip_registered);

      // create lists of sub-classes
      // note: do not use unscalable OksClass::create_sub_classes()
    for(i = p_classes.begin(); i != p_classes.end(); ++i) {
      OksClass * c(i->second);

      if(!c->p_all_sub_classes) c->p_all_sub_classes = new OksClass::FList();
      else c->p_all_sub_classes->clear();

      c->p_id = idx++;
    }

    for(i = p_classes.begin(); i != p_classes.end(); ++i) {
      OksClass * c(i->second);
      if(const OksClass::FList * scl = c->p_all_super_classes) {
        for(OksClass::FList::const_iterator j = scl->begin(); j != scl->end(); ++j) {
	  (*j)->p_all_sub_classes->push_back(c);
	}
      }
    }

    if(get_test_duplicated_objects_via_inheritance_mode() && !get_allow_duplicated_objects_mode()) {
      for(i = p_classes.begin(); i != p_classes.end(); ++i) {
        OksClass *c(i->second);
        if(!c->get_is_abstract()) {
          if(const OksClass::FList * spc = c->all_super_classes()) {
	    if(!spc->empty()) {
              wmemset(reinterpret_cast<wchar_t *>(table), 0, array_size);  // [re-]set table by NULLs
              for(OksClass::FList::const_iterator j1 = spc->begin(); j1 != spc->end(); ++j1) {
	        if(const OksClass::FList * sbc = (*j1)->all_sub_classes()) {
	          for(OksClass::FList::const_iterator j2 = sbc->begin(); j2 != sbc->end(); ++j2) {
                    OksClass *c2(*j2);
                    if((c2 != c) && !c2->get_is_abstract()) {
                      table[c2->p_id] = c2;
                    }
	          }
	        }
	      }
              
              unsigned int count(0);
              for(unsigned int x = 0; x < num_of_classes; ++x) {
                if(table[x]) count++;
              }

              if(count) {
                std::vector<OksClass *> * cih;
                if(!c->p_inheritance_hierarchy) {
                  cih = c->p_inheritance_hierarchy = new std::vector<OksClass *>();
                }
                else {
                  cih = c->p_inheritance_hierarchy;
                  cih->clear();
                }

                cih->reserve(count);

                for(unsigned int x = 0; x < num_of_classes; ++x) {
                  if(table[x]) cih->push_back(table[x]);
                }
              }
	    }
	  }
	}
      }

#ifndef ERS_NO_DEBUG
      if(ers::debug_level() >= 2) {
        std::ostringstream s;

        for(i = p_classes.begin(); i != p_classes.end(); ++i) {
          if(std::vector<OksClass *> * cis = i->second->p_inheritance_hierarchy) {
	    s << " - class \'" << i->second->get_name() << "\' shares IDs with " << cis->size() << " classes: ";
	    OksClass::Set sorted;
	    for(std::vector<OksClass *>::const_iterator j = cis->begin(); j != cis->end(); ++j) {
              sorted.insert(*j);
	    }
	    for(OksClass::Set::const_iterator j = sorted.begin(); j != sorted.end(); ++j) {
              if(j != sorted.begin()) s << ", ";
	      s << '\'' << (*j)->get_name() << '\'';
	    }
	    s << std::endl;
          }
	}

	TLOG_DEBUG(2) << "Schema inheritance hierarchy used to test objects with equal IDs:\n" << s.str();
      }
#endif
    }

    delete [] table;

    k_check_bind_classes_status();
  }
}

bool
OksKernel::is_dangling(OksObject *o) const
{
  OSK_PROFILING(OksProfiler::KernelIsDanglingObject, this)
  OSK_VERBOSE_REPORT("Enter OksKernel::is_dangling(OksObject *)")

  bool not_found;

  {
    std::lock_guard lock(p_objects_mutex);
    not_found = (p_objects.find(o) == p_objects.end());
  }

  if(p_verbose) {
    std::string fname("OksKernel::is_dangling(");

    if(not_found)
      fname += "?object?) returns true";
    else {
      fname += '\"';
      fname += o->GetId();
      fname += "\") returns false";
    }

    OSK_VERBOSE_REPORT("LEAVE " << fname.c_str())
  }

  return not_found;
}


bool
OksKernel::is_dangling(OksClass *c) const
{
  OSK_PROFILING(OksProfiler::KernelIsDanglingClass, this)
  OSK_VERBOSE_REPORT("Enter OksKernel::is_dangling(OksClass *)")

  bool not_found = true;

  {
    if(!p_classes.empty()) {
      for(OksClass::Map::const_iterator i = p_classes.begin(); i != p_classes.end(); ++i) {
        if(i->second == c) {
          not_found = false;
	  break;
        }
      }
    }
  }

  if(p_verbose) {
    std::string fname("OksKernel::is_dangling(");

    if(not_found)
      fname += "?class?) returns true";
    else {
      fname += '\"';
      fname += c->get_name();
      fname += "\") returns false";
    }

    OSK_VERBOSE_REPORT("LEAVE " << fname.c_str())
  }

  return not_found;
}


void
OksKernel::get_all_classes(const std::vector<std::string>& names_in, oks::ClassSet& classes_out) const
{
  for(std::vector<std::string>::const_iterator i = names_in.begin(); i != names_in.end(); ++i) {
    OksClass * c = find_class(*i);
    if(c != 0 && classes_out.find(c) == classes_out.end()) {
      classes_out.insert(c);
      if(const OksClass::FList * sc = c->all_sub_classes()) {
        for(OksClass::FList::const_iterator j = sc->begin(); j != sc->end(); ++j) {
          classes_out.insert(*j);
        }
      }
    }
  }
}


  // keep information about repository root
  // throw exception if TDAQ_DB_REPOSITORY or TDAQ_DB_USER_REPOSITORY are not defined

struct ReposDirs {

  ReposDirs(const char * op, const char * cwd, OksKernel * kernel);
  ~ReposDirs();

  const char * p_repository_root;
  const char * p_user_repository_root;
  size_t p_repos_dir_len;
  size_t p_user_dir_len;
  const char * p_dir;

  static std::mutex p_mutex;
};


std::mutex ReposDirs::p_mutex;


ReposDirs::ReposDirs(const char * op, const char * cwd, OksKernel * kernel)
{
  p_repository_root = OksKernel::get_repository_root().c_str();
  if(OksKernel::get_repository_root().empty()) {
    throw oks::RepositoryOperationFailed(op, "the repository-root is not set (check environment variable TDAQ_DB_REPOSITORY)");
  }
  else {
    p_repos_dir_len = strlen(p_repository_root);
  }

  p_user_repository_root = kernel->get_user_repository_root().c_str();
  if(!*p_user_repository_root) {
    throw oks::RepositoryOperationFailed(op, "the user-repository-root is not set (check environment variable TDAQ_DB_USER_REPOSITORY)");
  }
  else {
    p_user_dir_len = strlen(p_user_repository_root);
  }

  p_mutex.lock();

  if(strcmp(cwd, p_user_repository_root)) {
    if(int result = chdir(p_user_repository_root)) {
      p_mutex.unlock();
      std::ostringstream text;
      text << "chdir (\'" << p_user_repository_root << "\') failed with code " << result << ": " << oks::strerror(errno);
      throw oks::RepositoryOperationFailed(op, text.str());
    }
    p_dir = cwd;
    TLOG_DEBUG( 2 ) << "change cwd: \"" << p_user_repository_root << '\"' ;
  }
  else {
    p_dir = 0;
    TLOG_DEBUG( 2 ) << "cwd: \"" << p_user_repository_root << "\" is equal to target dir" ;
  }
}

ReposDirs::~ReposDirs()
{
  if(p_dir) {
    if(int result = chdir(p_dir)) {
      Oks::error_msg("ReposDirs::~ReposDirs()")
        << "chdir (\'" << p_dir << "\') failed with code " << result << ": " << oks::strerror(errno) << std::endl;
    }
    TLOG_DEBUG( 2 ) << "restore cwd: \"" << p_dir  << '\"' ;
  }

  p_mutex.unlock();
}



  // keep information and delete output file on exit

struct CommandOutput {

  CommandOutput(const char * command_name, const OksKernel * k, std::string& cmd);
  ~CommandOutput();

  const std::string& file_name() const {return p_file_name;}
  std::string cat2str() const;
  std::string last_str() const;

  void check_command_status(int status);

  std::string p_file_name;
  std::string& p_command;
  const char * p_command_name;

};

CommandOutput::CommandOutput(const char *command_name, const OksKernel *k, std::string &cmd) :
p_command(cmd), p_command_name(command_name)
{
  p_file_name = oks::get_temporary_dir() + '/' + command_name + '.' + std::to_string(getpid()) + ':' +  std::to_string(reinterpret_cast<std::uintptr_t>(k)) + ".txt";

  p_command.append(" &>");
  p_command.append(p_file_name);
}

CommandOutput::~CommandOutput()
{
  unlink(p_file_name.c_str());
}

std::string
CommandOutput::cat2str() const
{
  std::ifstream f(p_file_name.c_str());
  std::string out;

  if(f) {
    std::filebuf * buf = f.rdbuf();
    while(true) {
      char c = buf->sbumpc();
      if(c == EOF) break;
      else out += c;
    }
  }

  return out;
}

std::string
CommandOutput::last_str() const
{
  std::ifstream f(p_file_name.c_str());
  std::string lastline;

  if (f.is_open())
  {
    f.seekg(-1, std::ios_base::end);
    if (f.peek() == '\n')
      {
        f.seekg(-1, std::ios_base::cur);
        for (int i = f.tellg(); i >= 0; i--)
          {
            if (f.peek() == '\n')
              {
                f.get();
                break;
              }
            f.seekg(i, std::ios_base::beg);
          }
      }

    getline(f, lastline);
  }
  return lastline;
}


void
CommandOutput::check_command_status(int status)
{
  if (status == -1)
    {
      std::ostringstream text;
      text << "cannot execute command \'" << p_command << "\': " << oks::strerror(errno);
      throw oks::RepositoryOperationFailed(p_command_name, text.str());
    }
  else if (int error_code = WEXITSTATUS(status))
    {
      std::ostringstream text;
      text << p_command.substr(0, p_command.find_first_of(' ')) << " exit with error code " << error_code << ", output:\n" << cat2str();
      throw oks::RepositoryOperationFailed(p_command_name, text.str());
    }
}


void
OksKernel::k_rename_repository_file(OksFile * file_h, const std::string& new_name)
{
  if (file_h->p_oks_format == "schema")
    {
      remove_schema_file(file_h);
      file_h->rename(new_name);
      add_schema_file(file_h);
    }
  else
    {
      remove_data_file(file_h);
      file_h->rename(new_name);
      add_data_file(file_h);
    }
}


void
OksKernel::k_checkout_repository(const std::string& param, const std::string& val, const std::string& branch_name)
{
  if (get_repository_root().empty())
    throw oks::RepositoryOperationFailed("checkout", "the repository root is not set");

  ReposDirs reps("checkout", s_cwd, this);

  std::string cmd("oks-checkout.sh");

  if(get_verbose_mode()) { cmd += " -v"; }

  cmd.append(" -u ");
  cmd.append(get_user_repository_root());

  if (!param.empty())
    {
      cmd.append(" --");
      cmd.append(param);
      cmd.push_back(' ');
      cmd.push_back('\"');
      cmd.append(val);
      cmd.push_back('\"');
    }

  if (!branch_name.empty())
    {
      cmd.append(" -b ");
      cmd.append(branch_name);
    }

  auto start_usage = std::chrono::steady_clock::now();

  if (!p_silence)
    {
      std::lock_guard lock(p_parallel_out_mutex);
      oks::log_timestamp() << "[OKS checkout] => " << cmd << std::endl;
    }

  CommandOutput cmd_out("oks-checkout", this, cmd);
  cmd_out.check_command_status(system(cmd.c_str()));

  p_repository_checkout_ts = p_repository_update_ts = std::time(0);

  if(!p_silence)
    {
      std::lock_guard lock(p_parallel_out_mutex);
      std::cout << cmd_out.cat2str();
      oks::log_timestamp() << "[OKS checkout] => done in " << std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now()-start_usage).count() / 1000. << " ms" << std::endl;
    }

  static std::string version_prefix("checkout oks version ");
  std::string version = cmd_out.last_str();
  std::string::size_type pos = version.find(version_prefix);

  if (pos == 0)
    p_repository_version = version.substr(version_prefix.size());
  else
    throw oks::RepositoryOperationFailed("checkout", "cannot read oks version");
}

void
OksKernel::remove_user_repository_dir()
{
  if (p_allow_repository && p_user_repository_root_created)
    {
      oks::s_git_folders.remove(p_user_repository_root);
      oks::s_git_folders.erase(p_user_repository_root);
      p_user_repository_root_created = false;
    }
}

void
OksKernel::unset_repository_created()
{
  oks::s_git_folders.erase(p_user_repository_root);
  p_user_repository_root_created = false;
}

void
OksKernel::k_copy_repository(const std::string& source, const std::string& destination)
{
  std::string cmd("oks-copy.sh");

  if (get_verbose_mode())
    cmd.append(" -v");

  cmd.append(" -s ");
  cmd.append(source);

  cmd.append(" -d ");
  cmd.append(destination);

  cmd.append(" -c ");
  cmd.append(get_repository_version());

  auto start_usage = std::chrono::steady_clock::now();

  if(!p_silence) {
    std::lock_guard lock(p_parallel_out_mutex);
    oks::log_timestamp() << "[OKS copy] => " << cmd << std::endl;
  }

  CommandOutput cmd_out("oks-copy", this, cmd);
  cmd_out.check_command_status(system(cmd.c_str()));

  if(!p_silence) {
    std::lock_guard lock(p_parallel_out_mutex);
    std::cout << cmd_out.cat2str();
    oks::log_timestamp() << "[OKS copy] => done in " << std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now()-start_usage).count() / 1000. << " ms" << std::endl;
  }
}


void
OksKernel::update_repository(const std::string& param, const std::string& val, RepositoryUpdateType update_type)
{
  std::unique_lock lock(p_kernel_mutex);

  if (OksKernel::get_repository_root().empty())
    throw oks::RepositoryOperationFailed("update", "the repository-root is not set (check environment variable TDAQ_DB_REPOSITORY)");

  if (get_user_repository_root().empty())
    throw oks::RepositoryOperationFailed("update", "the user-repository-root is not set (check environment variable TDAQ_DB_USER_REPOSITORY)");

  std::string cmd("oks-update.sh");

  if (get_verbose_mode())
    cmd.append(" -v");

  cmd.append(" -u ");
  cmd.append(get_user_repository_root());

  if (update_type == OksKernel::DiscardChanges)
    cmd.append(" --force");
  else if (update_type == OksKernel::MergeChanges)
    cmd.append(" --merge");

  if (!param.empty())
    {
      cmd.append(" --");
      cmd.append(param);
      cmd.push_back(' ');
      cmd.push_back('\"');
      cmd.append(val);
      cmd.push_back('\"');
    }

  auto start_usage = std::chrono::steady_clock::now();

  if (!p_silence) {
    std::lock_guard lock(p_parallel_out_mutex);
    oks::log_timestamp() << "[OKS update] => " << cmd << std::endl;
  }

  CommandOutput cmd_out("oks-update", this, cmd);
  cmd_out.check_command_status(system(cmd.c_str()));

  p_repository_update_ts = std::time(0);

  static std::string version_prefix("update oks version ");
  std::string version = cmd_out.last_str();
  std::string::size_type pos = version.find(version_prefix);

  if(pos == 0)
    p_repository_version = version.substr(version_prefix.size());
  else
    throw oks::RepositoryOperationFailed("commit", "cannot read oks version");

  if (!p_silence)
    {
      std::lock_guard lock(p_parallel_out_mutex);
      std::cout << cmd_out.cat2str();
      oks::log_timestamp() << "[OKS update] => done in " << std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now()-start_usage).count() / 1000. << " ms" << std::endl;
    }
}

void
OksKernel::commit_repository(const std::string& comments, const std::string& credentials)
{
  std::unique_lock lock(p_kernel_mutex);

  if (OksKernel::get_repository_root().empty())
    throw oks::RepositoryOperationFailed("commit", "the repository-root is not set (check environment variable TDAQ_DB_REPOSITORY)");

  if (get_user_repository_root().empty())
    throw oks::RepositoryOperationFailed("commit", "the user-repository-root is not set (check environment variable TDAQ_DB_USER_REPOSITORY)");

  std::string cmd("oks-commit.sh");

  if (get_verbose_mode())
    cmd.append(" -v");

  cmd.append(" -u ");
  cmd.append(get_user_repository_root());

  if(comments.empty() || std::all_of(comments.begin(), comments.end(), [](char c) { return std::isspace(c); }))
    throw oks::RepositoryOperationFailed("commit", "the commit message may not be empty");

  const std::string log_file_name(oks::get_temporary_dir() + '/' + "oks-commit-msg." + std::to_string(getpid()) + ':' +  std::to_string(reinterpret_cast<std::uintptr_t>(this)) + ".txt");

  try
    {
      std::ofstream f(log_file_name);

      f.exceptions(std::ostream::failbit | std::ostream::badbit);

      if (!f)
        throw std::runtime_error("create message file failed");

      f << comments;
      f.close();
    }
  catch (const std::exception& ex)
    {
      std::ostringstream ss;
      ss << "cannot write to file \'" << log_file_name << "\' to save commit message: " << ex.what();
      throw oks::RepositoryOperationFailed("commit", ss.str());
    }

  cmd.append(" -f \'");
  cmd.append(log_file_name);
  cmd.push_back('\'');

  // if (!credentials.empty())
  //   {
  //     try
  //       {
  //         auto token = daq::tokens::verify(credentials);

  //         const std::string author = token.get_subject();

  //         std::string email;

  //         if (token.has_payload_claim("email"))
  //           email = token.get_payload_claim("email").as_string();

  //         if (email.empty())
  //           email = author + '@' + get_domain_name();

  //         System::User user(author);

  //         cmd.append(" -n \'");
  //         cmd.append(user.real_name());
  //         cmd.append("\' -e \'");
  //         cmd.append(email);
  //         cmd.push_back('\'');

  //         static std::once_flag flag;

  //         std::call_once(flag, []()
  //           {
  //             const char * opt = " -o SendEnv=OKS_COMMIT_USER";

  //             std::string val;

  //             if (const char * s = getenv("GIT_SSH_COMMAND"))
  //               {
  //                 if (strstr(s, opt) == nullptr)
  //                   val = s;
  //               }
  //             else
  //               {
  //                 val = "ssh";
  //               }

  //             if (!val.empty())
  //               {
  //                 val.append(opt);
  //                 setenv("GIT_SSH_COMMAND", val.c_str(), 1);
  //               }
  //           }
  //         );

  //         setenv("OKS_COMMIT_USER", credentials.c_str(), 1);
  //       }
  //     catch(const ers::Issue& ex)
  //       {
  //         std::ostringstream text;
  //         text << '\t' << ex;
  //         throw oks::RepositoryOperationFailed("commit", text.str());
  //       }
  //   }

  auto start_usage = std::chrono::steady_clock::now();

  if (!p_silence)
    {
      std::lock_guard lock(p_parallel_out_mutex);
      oks::log_timestamp() << "[OKS commit] => " << cmd << std::endl;
    }

  CommandOutput cmd_out("oks-commit", this, cmd);

  int status = system(cmd.c_str());

  unlink(log_file_name.c_str());

  if (!credentials.empty())
    unsetenv("OKS_COMMIT_USER");

  cmd_out.check_command_status(status);

  p_repository_update_ts = std::time(0);

  if (!p_silence)
    {
      std::lock_guard lock(p_parallel_out_mutex);
      std::cout << cmd_out.cat2str();
      oks::log_timestamp() << "[OKS commit] => done in " << std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now()-start_usage).count() / 1000. << " ms" << std::endl;
   }

  static std::string version_prefix("commit oks version ");
  std::string version = cmd_out.last_str();
  std::string::size_type pos = version.find(version_prefix);

  if(pos == 0)
    p_repository_version = version.substr(version_prefix.size());
  else
    throw oks::RepositoryOperationFailed("commit", "cannot read oks version");
}

void
OksKernel::tag_repository(const std::string& tag)
{
  std::unique_lock lock(p_kernel_mutex);

  if (OksKernel::get_repository_root().empty())
    throw oks::RepositoryOperationFailed("tag", "the repository-root is not set (check environment variable TDAQ_DB_REPOSITORY)");

  if (get_user_repository_root().empty())
    throw oks::RepositoryOperationFailed("tag", "the user-repository-root is not set (check environment variable TDAQ_DB_USER_REPOSITORY)");

  std::string cmd("oks-tag.sh");

  if (get_verbose_mode())
    cmd.append(" -v");

  cmd.append(" -u ");
  cmd.append(get_user_repository_root());

  if(tag.empty() || std::all_of(tag.begin(), tag.end(), [](char c) { return std::isspace(c); }))
    throw oks::RepositoryOperationFailed("tag", "the tag may not be empty");

  cmd.append(" -t \'");
  cmd.append(tag);
  cmd.append("\' -c \'");
  cmd.append(get_repository_version());
  cmd.push_back('\'');

  auto start_usage = std::chrono::steady_clock::now();

  if (!p_silence)
    {
      std::lock_guard lock(p_parallel_out_mutex);
      oks::log_timestamp() << "[OKS tag] => " << cmd << std::endl;
    }

  CommandOutput cmd_out("oks-tag", this, cmd);
  cmd_out.check_command_status(system(cmd.c_str()));

  if (!p_silence)
    {
      std::lock_guard lock(p_parallel_out_mutex);
      std::cout << cmd_out.cat2str();
      oks::log_timestamp() << "[OKS tag] => done in " << std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now()-start_usage).count() / 1000. << " ms" << std::endl;
   }
}

std::list<std::string>
OksKernel::get_repository_versions_diff(const std::string& sha1, const std::string& sha2)
{
  std::unique_lock lock(p_kernel_mutex);

  if (OksKernel::get_repository_root().empty())
    throw oks::RepositoryOperationFailed("diff", "the repository-root is not set (check environment variable TDAQ_DB_REPOSITORY)");

  if (get_user_repository_root().empty())
    throw oks::RepositoryOperationFailed("diff", "the user-repository-root is not set (check environment variable TDAQ_DB_USER_REPOSITORY)");

  std::string cmd("oks-diff.sh");

  if (get_verbose_mode())
    cmd.append(" -v");

  cmd.append(" -u ");
  cmd.append(get_user_repository_root());

  if(!sha1.empty() && !sha2.empty())
    {
      cmd.append(" --sha ");
      cmd.append(sha1);
      cmd.push_back(' ');
      cmd.append(sha2);
    }
  else
    {
      cmd.append(" --unmerged");
    }

  CommandOutput cmd_out("oks-diff", this, cmd);
  cmd_out.check_command_status(system(cmd.c_str()));

  std::string output = cmd_out.cat2str();

  if(!p_silence) {
    std::lock_guard lock(p_parallel_out_mutex);
    std::cout << output << "[OKS get_diff] => done" << std::endl;
  }

  std::istringstream s(output);

  std::string line;
  bool found = false;

  const char * diff_pattern = "git diff --name-only ";

  while (std::getline(s, line))
    if (line.find(diff_pattern) == 0)
      {
        found = true;
        break;
      }

  if (found == false)
    {
      std::ostringstream text;
      text << "cannot find \"" << diff_pattern << "\" pattern in output of oks-diff.sh:\n" << output;
      throw oks::RepositoryOperationFailed("get_diff", text.str());
    }

  std::list<std::string> result;

  while (std::getline(s, line))
    result.push_back(line);

  return result;
}

static bool
check_relevant(const std::set<std::string>& loaded_files, const OksRepositoryVersion& ver)
{
  for (const auto& x : ver.m_files)
    if (loaded_files.find(x) != loaded_files.end())
      return true;

  return false;
}

std::vector<OksRepositoryVersion>
OksKernel::get_repository_versions(bool skip_irrelevant, const std::string& command_line)
{
  std::string cmd("oks-log.sh");

  if (get_verbose_mode())
    cmd.append(" -v");

  cmd.append(" -u ");
  cmd.append(get_user_repository_root());

  cmd.append(command_line);

  CommandOutput cmd_out("oks-log", this, cmd);
  cmd_out.check_command_status(system(cmd.c_str()));

  std::string output = cmd_out.cat2str();

  std::istringstream s(output);

  std::string line;
  bool found = false;

  while (std::getline(s, line))
    if (line.find("git log ") == 0)
      {
        found = true;
        break;
      }

  if (found == false)
    {
      std::ostringstream text;
      text << "cannot find \"git log\" pattern in output of oks-log.sh:\n" << output;
      throw oks::RepositoryOperationFailed("log", text.str());
    }

  std::set<std::string> loaded_files;

  if (skip_irrelevant)
    {
      std::size_t len = get_user_repository_root().size()+1;

      for (const auto& x : p_schema_files)
        loaded_files.emplace(x.first->substr(len));

      for (const auto& x : p_data_files)
        loaded_files.emplace(x.first->substr(len));
    }

  std::vector<OksRepositoryVersion> vec;
  OksRepositoryVersion ver;

  found = false;

  while (std::getline(s, line))
    {
      if (found == false)
        {
          ver.clear();

          std::string::size_type idx1 = 0;
          std::string::size_type idx2 = line.find('|');

          if (idx2 != std::string::npos)
            {
              ver.m_commit_hash = line.substr(idx1, idx2);
              idx1 = idx2+1;
              idx2 = line.find('|', idx1);

              if (idx2 != std::string::npos)
                {
                  ver.m_user = line.substr(idx1, idx2-idx1);
                  idx1 = idx2+1;
                  idx2 = line.find('|', idx1);

                  if (idx2 != std::string::npos)
                    {
                      ver.m_date = std::stol(line.substr(idx1, idx2-idx1));
                      ver.m_comment = line.substr(idx2+1);
                    }
                }
            }


          if (!ver.m_comment.empty())
            found = true;
          else
            {
              std::ostringstream text;
              text << "unexpected line \"" << line << "\" in output of oks-log.sh:\n" << output;
              throw oks::RepositoryOperationFailed("log", text.str());
            }
        }
      else
        {
          if (line.empty())
            {
              found = false;

              if (skip_irrelevant && check_relevant(loaded_files, ver) == false)
                continue;

              vec.emplace_back(ver);
            }
          else
            {
              ver.m_files.push_back(line);
            }
        }
    }

  if (found)
    {
      if (skip_irrelevant == false || check_relevant(loaded_files, ver))
        vec.emplace_back(ver);
    }

  return vec;
}

std::vector<OksRepositoryVersion>
OksKernel::get_repository_versions_by_hash(bool skip_irrelevant, const std::string& since, const std::string& until)
{
  std::string command_line;

  if(!since.empty() && !until.empty())
    {
      command_line.push_back(' ');
      command_line.append(since);
      command_line.append("..");
      command_line.append(until);
    }
  else if(!since.empty())
    {
      command_line.push_back(' ');
      command_line.append(since);
      command_line.append("..origin/master");
    }
  else if(!until.empty())
    {
      command_line.append(" ..");
      command_line.append(until);
    }

  return get_repository_versions(skip_irrelevant, command_line);
}

static std::string
replace_datetime_spaces(const std::string& in)
{
  std::string out(in);
  std::replace(out.begin(), out.end(), ' ', 'T');
  return out;
}


std::vector<OksRepositoryVersion>
OksKernel::get_repository_versions_by_date(bool skip_irrelevant, const std::string& since, const std::string& until)
{
  std::string command_line;

  if(!since.empty())
    {
      command_line.append(" --since ");
      command_line.append(replace_datetime_spaces(since));
    }

  if(!until.empty())
    {
      command_line.append(" --until ");
      command_line.append(replace_datetime_spaces(until));
    }

  command_line.append(" origin/master");

  return get_repository_versions(skip_irrelevant, command_line);
}


std::string
OksKernel::read_repository_version()
{
  std::unique_lock lock(p_kernel_mutex);

  if (OksKernel::get_repository_root().empty())
    throw oks::RepositoryOperationFailed("status", "the repository-root is not set (check environment variable TDAQ_DB_REPOSITORY)");

  if (get_user_repository_root().empty())
    throw oks::RepositoryOperationFailed("status", "the user-repository-root is not set (check environment variable TDAQ_DB_USER_REPOSITORY)");

  std::string cmd("oks-version.sh");

  cmd.append(" -u ");
  cmd.append(get_user_repository_root());

  CommandOutput cmd_out("oks-status", this, cmd);
  cmd_out.check_command_status(system(cmd.c_str()));

  std::string output = cmd_out.cat2str();

  static std::string version_prefix("oks version ");
  std::string version = cmd_out.last_str();
  std::string::size_type pos = version.find(version_prefix);

  if (pos == 0)
    p_repository_version = version.substr(version_prefix.size());
  else
    {
      std::ostringstream text;
      text << "cannot read oks version from oks-version.sh output: \"" << output << '\"';
      throw oks::RepositoryOperationFailed("get version", text.str().c_str());
    }

  return p_repository_version;
}


void OksKernel::get_updated_repository_files(std::set<std::string>& updated, std::set<std::string>& added, std::set<std::string>& removed)
{
  std::unique_lock lock(p_kernel_mutex);

  if (OksKernel::get_repository_root().empty())
    throw oks::RepositoryOperationFailed("status", "the repository-root is not set (check environment variable TDAQ_DB_REPOSITORY)");

  if (get_user_repository_root().empty())
    throw oks::RepositoryOperationFailed("status", "the user-repository-root is not set (check environment variable TDAQ_DB_USER_REPOSITORY)");

  std::string cmd("oks-status.sh");

  cmd.append(" -u ");
  cmd.append(get_user_repository_root());

  CommandOutput cmd_out("oks-status", this, cmd);
  cmd_out.check_command_status(system(cmd.c_str()));

  std::string output = cmd_out.cat2str();

  if(p_verbose) {
    std::lock_guard lock(p_parallel_out_mutex);
    std::cout << output << "[OKS get_status] => done" << std::endl;
  }

  std::istringstream s(output);

  std::string line;
  bool found = false;

  const char * diff_pattern = "git status --porcelain";

  while (std::getline(s, line))
    if (line.find(diff_pattern) == 0)
      {
        found = true;
        break;
      }

  if (found == false)
    {
      std::ostringstream text;
      text << "cannot find \"" << diff_pattern << "\" pattern in output of oks-status.sh:\n" << output;
      throw oks::RepositoryOperationFailed("status", text.str());
    }

  std::list<std::string> result;

  while (std::getline(s, line))
    {
      if (line.find(" D ") == 0)
        removed.insert(line.substr(3));
      else if (line.find(" M ") == 0)
        updated.insert(line.substr(3));
      else if (line.find("?? ") == 0)
        added.insert(line.substr(3));
      else
        {
          std::ostringstream text;
          text << "unexpected output \"" << line << "\" in output of oks-status.sh:\n" << output;
          throw oks::RepositoryOperationFailed("status", text.str());
        }
    }
}


//   /* PAM authentication */

// struct creds_pam_t {
//   const char * user;
//   const char * pwd;
// };

// static int
// get_credentials_pam (int num_msg, const struct pam_message **msg, struct pam_response **resp, void *appdata_ptr)
// {
//   struct creds_pam_t * creds = (struct creds_pam_t *)appdata_ptr;
//   struct pam_response * buf = (struct pam_response *)(malloc(num_msg * sizeof (struct pam_response)));    /* released by PAM */

//   for(int k = 0; k < num_msg; ++ k) {
//     const size_t STRING_SIZE = 1024;
//     char * out;
//     int error = 0;

//     buf[k].resp = (char*)malloc(STRING_SIZE);
//     out = buf[k].resp;
//     out[0] = 0;

//     if(msg[k]->msg) {
//       if(!strcmp(msg[k]->msg, "login:") || !strcmp(msg[k]->msg, "login: ")) {
//         TLOG_DEBUG( 1 ) << "process \'login\' PAM credential of user " << creds->user ;
// 	strncpy(out, creds->user, STRING_SIZE);
//       }
//       else if(!strcmp(msg[k]->msg, "Password: ")  || !strcmp(msg[k]->msg, "LDAP Password: ")) {
//         TLOG_DEBUG( 1 ) << "process \'password\' PAM credential of user " << creds->user ;
//         strncpy (out, creds->pwd, STRING_SIZE);
//       }
//       else {
//         error = 1;
//       }
//     }

//     if(error != 0) {
//       std::cerr << "ERROR: unexpected request in process PAM credential: " << msg[k]->msg << std::endl;
//       for (int j = 0; j <= k; ++ j) {
//         free(buf[k].resp);
//       }
//       free(buf);
//       return PAM_CONV_ERR;
//     }

//     out[STRING_SIZE - 1] = 0;
//     buf[k].resp_retcode = 0;
//   }

//   *resp = buf;

//   return PAM_SUCCESS;
// }


// void
// OksKernel::validate_credentials(const char * user, const char * passwd)
// {
//   if(!user || !*user) {
//     throw oks::AuthenticationFailure("user name is not given");
//   }

//   if(!passwd || !*passwd) {
//     throw oks::AuthenticationFailure("user password is not given");
//   }

//   struct creds_pam_t creds = {user, passwd};
//   struct pam_conv conv = {get_credentials_pam, &creds};

//   const char *service = "system-auth";
//   pam_handle_t * pamh = 0;

//   int retval = pam_start(service, 0, &conv, &pamh);
//   if (retval != PAM_SUCCESS) {
//     std::ostringstream text;
//     text << "pam_start() failed for service \'" << service << "\': " << pam_strerror (pamh, retval);
//     throw oks::AuthenticationFailure(text.str());
//   }

//   retval = pam_authenticate (pamh, 0);
//   if (retval == PAM_SUCCESS) {
//     TLOG_DEBUG( 1 ) << "user " << user << " was authenticated" ;
//   }
//   else {
//     std::ostringstream text;
//     text << "failed to authenticate user " << user << ": " << pam_strerror (pamh, retval);
//     throw oks::AuthenticationFailure(text.str());
//   }

//   retval = pam_end (pamh,retval);
//   if (retval != PAM_SUCCESS) {
//      std::ostringstream text;
//     text << "pam_end() failed: " << pam_strerror (pamh, retval);
//     throw oks::AuthenticationFailure(text.str());
//   }
// }

void
OksKernel::k_check_bind_classes_status() const noexcept
{
  std::ostringstream out;

  for (auto & c : p_classes)
    c.second->check_relationships(out, true);

  p_bind_classes_status = out.str();
}


std::ostream&
oks::log_timestamp(oks::__LogSeverity__ severity)
{
  auto now(std::chrono::system_clock::now());
  auto time_since_epoch(now.time_since_epoch());
  auto seconds_since_epoch(std::chrono::duration_cast<std::chrono::seconds>(time_since_epoch));

  std::time_t now_t(std::chrono::system_clock::to_time_t(std::chrono::system_clock::time_point(seconds_since_epoch)));

  char buff[128];
  std::size_t len = std::strftime(buff, 128 - 16, "%Y-%b-%d %H:%M:%S.", std::localtime(&now_t));
  sprintf(buff+len, "%03d ", (int)(std::chrono::duration_cast<std::chrono::milliseconds>(time_since_epoch).count() - seconds_since_epoch.count()*1000));

  std::ostream& s (severity <= oks::Warning ? std::cerr : std::cout);

  s << buff << (severity == oks::Error ? "ERROR " : severity == oks::Warning ? "WARNING " : severity == oks::Debug ? "DEBUG " : "");

  return s;
}
