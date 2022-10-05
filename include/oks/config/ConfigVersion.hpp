  /**
   *  \file ConfigVersion.h This file contains ConfigVersion class describing OKS GIT repository version.
   *  \author Igor Soloviev
   *  \brief config version class
   */

#ifndef CONFIG_CONFIGVERSION_H_
#define CONFIG_CONFIGVERSION_H_

#include <ctime>
#include <string>
#include <vector>

namespace daq
{
  namespace config
  {

    /**
     *  \brief Represents configuration version.
     *
     *  The objects are created by the Configuration::get_versions() and Configuration::get_changes() methods.
     */

    class Version
    {

      friend class Configuration;
      friend class ConfigurationImpl;

    public:

      enum QueryType
      {
        query_by_date, query_by_id, query_by_tag
      };


      Version(const std::string& id, const std::string& user, std::time_t ts, const std::string& comment, const std::vector<std::string>& files) noexcept :
          m_id(id), m_user(user), m_timestamp(ts), m_comment(comment), m_files(files)
      {
        ;
      }

    public:

      /**
       *  The version unique ID is a repository hash (GIT SHA).
       *  \return version unique ID
       */
      const std::string&
      get_id() const noexcept
      {
        return m_id;
      }

      /**
       *  The user who made a commit.
       *  \return user name
       */
      const std::string&
      get_user() const noexcept
      {
        return m_user;
      }

      /**
       *  The timestamp (in seconds since Epoch) when commit was made.
       *  \return timestamp
       */
      std::time_t
      get_timestamp() const noexcept
      {
        return m_timestamp;
      }

      /**
       *  The commit comment.
       *  \return user comment
       */
      const std::string&
      get_comment() const noexcept
      {
        return m_comment;
      }

      /**
       *  The files modified in this version.
       *  \return the modified files
       */
      const std::vector<std::string>&
      get_files() const noexcept
      {
        return m_files;
      }

    private:

      std::string m_id;
      std::string m_user;
      std::time_t m_timestamp;
      std::string m_comment;
      std::vector<std::string> m_files;

    };


    std::ostream&
    operator<<(std::ostream&, const Version &);
  }
}

#endif // CONFIG_CONFIGVERSION_H_
