  /**
   *  \file Errors.h This file contains exception classes,
   *  which can be thrown by methods of config packages.
   *  \author Igor Soloviev
   *  \brief exceptions of methods of config packages
   */

#ifndef CONFIG_ERRORS_H_
#define CONFIG_ERRORS_H_

#include <ers/Issue.hpp>


namespace daq {

    /**
     * \class daq::config::Exception
     * \brief Base class for all \b config exceptions
     *
     *	This exception normally should be caught when any config method is used.
\code   
try {
  Configuration db(...);
  ... // any user code working with db
}
catch (daq::config::Exception & ex) {
    // throw some user-defined exception in case of config exception
  throw ers::error(user::exception(ERS_HERE, "config database problem", ex));
}
\endcode
     */

  ERS_DECLARE_ISSUE_HPP( config, Exception, , )


    /**
     * \class daq::config::Generic
     * \brief Generic configuration exception.
     *
     *	It reports most of the config problems, such as:
     *  - bad database,
     *  - wrong parameter,
     *  - plug-in specific problems
     */

  ERS_DECLARE_ISSUE_BASE_HPP(
    config,
    Generic,
    config::Exception,
    ,
    ,
    ((const char*)what)
  )


    /**
     * \class daq::config::NotFound
     * \brief Try to access non-existent object or class.
     *
     *	It is thrown if a config object accessed by ID is not found,
     *  or a class accessed by name is not found.
     */

  ERS_DECLARE_ISSUE_BASE_HPP(
    config,
    NotFound,
    config::Exception,
    ,
    ,
    ((const char*)type)
    ((const char*)data)
  )

    /**
     * \class daq::config::DeletedObject
     * \brief Try to access deleted DAL object.
     *
     *	It is thrown when a deleted DAL template object is accessed.
     *  Note, an object can be via notification mechanism or by the
     *  user's code modifying database.
     */

  ERS_DECLARE_ISSUE_BASE_HPP(
    config,
    DeletedObject,
    config::Exception,
    ,
    ,
    ((const char*)class_name)
    ((const char*)object_id)
  )

}

#endif
