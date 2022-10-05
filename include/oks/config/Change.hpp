  /**
   *  \file Change.h This file contains Change class,
   *  that is used to describe database changes.
   *  \author Igor Soloviev
   *  \brief describe database changes
   */

#ifndef CONFIG_CONFIGURATION__CHANGE_H_
#define CONFIG_CONFIGURATION__CHANGE_H_

#include <string>
#include <vector>
#include <iostream>


  /**
   *  \brief Describes changes inside a class returned by the notification mechanism.
   *
   *  All changes received by the notification mechanism after commit
   *  of database modification are passed to subscribed client in one go.
   *  This is more preferred way than passing changes for each class or
   *  object in a separate callback. In the latter case user need
   *  to be sure at some moment that (s)he read all changes, keep
   *  changes already read, etc.
   *
   *  The changes are passed to a client as a vector of objects of this class.
   *  For each class there are three vectors of object identities:
   *  - modified objects (i.e. one or more attributes/relationships values were modified)
   *  - created objects
   *  - removed objects
   */

class ConfigurationChange {

  friend class Configuration;
  friend class ConfigurationImpl;

  public:

      /// Get name of class which objects were modified.

    const std::string& get_class_name() const {return m_class_name;}


      /// Return vector of identies of modified objects.

    const std::vector<std::string>& get_modified_objs() const {return m_modified;}


      /// Return vector of identies of created objects.

    const std::vector<std::string>& get_created_objs() const {return m_created;}


      /// Return vector of identies of removed objects.

    const std::vector<std::string>& get_removed_objs() const {return m_removed;}


     /**
      *  \brief Helper method to add object to the vector of existing changes.
      *
      *  Note that this is the only method available to create
      *  new changes description since the constructor is private.
      *
      *  The method adds object described by the 'class_name' and 'obj_name' to the changes.
      *  New ConfigurationChange object is created if required.
      *
      *  \param changes      description of existing changes
      *  \param class_name   name of the object's class
      *  \param obj_id       object's id
      *  \param action       requested action:
      *                      - '+' created
      *                      - '~' changed
      *                      - '-' removed
      */

    static void add(std::vector<ConfigurationChange *>& changes,
                    const std::string& class_name,
                    const std::string& obj_id,
                    const char action);

     /**
      *  \brief Helper method to clear vector of changes (pointers).
      *
      *  Destroy ConfigurationChange objects referenced by the container.
      *
      *  \param changes      description of existing changes
      */

    static void clear(std::vector<ConfigurationChange *>& changes);


  private:

    ConfigurationChange( const ConfigurationChange & );
    ConfigurationChange& operator= ( const ConfigurationChange & );

    ConfigurationChange(const std::string& name) : m_class_name(name) {}


  private:

    std::string m_class_name;

    std::vector<std::string> m_modified;
    std::vector<std::string> m_created;
    std::vector<std::string> m_removed;

};

  /** Operator prints out to stream details of configuration change. **/

std::ostream& operator<<(std::ostream&, const ConfigurationChange&);


  /** Operator prints out to stream details of all configuration changes. **/

std::ostream& operator<<(std::ostream&, const std::vector<ConfigurationChange *>&);

#endif // CONFIG_CONFIGURATION__CHANGE_H_
