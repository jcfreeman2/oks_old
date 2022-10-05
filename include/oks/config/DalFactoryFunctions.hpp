#ifndef DAL_FACTORY_FUNCTIONS_H
#define DAL_FACTORY_FUNCTIONS_H

#include <set>
#include <string>

#include <boost/compute/functional/identity.hpp>

class Configuration;
class ConfigObject;
class ConfigurationChange;
class DalObject;
class CacheBase;


/**
 *  \brief The factory function creates DAL object of given template class.
 *
 *  It is used by the referenced_by() method returning DAL object.
 *
 *  \param db    reference on configuration
 *  \param obj   config object
 *  \param uid   uid for generated objects
 *  \return      the DAL object
 */

typedef DalObject * (*dal_object_creator)(Configuration& db, ConfigObject& obj, const std::string& uid);


/**
 *  \brief The notification callback function which
 *  is invoked by database implementation in case of changes.
 *
 *  \warning To be used by automatically generated libraries and should not be directly used by developers.
 *
 *  \param conf            reference on configuration
 *  \param changed_class   pointer to changed class
 */

typedef void (*notify2)(Configuration & conf, const ConfigurationChange * changed_class);


/**
 *  \brief The function to update states of objects in cache.
 *
 *  \warning To be used by automatically generated libraries and should not be directly used by developers.
 *
 *  \param conf                 reference on configuration
 *  \param re_initialise_obj    reinitialize object (after abort operation)
 */

typedef void (*unread_object)(CacheBase* x);


/**
 *  \brief The function to rename object in cache.
 *
 *  \warning To be used by automatically generated libraries and should not be directly used by developers.
 *
 *  \param x         reference on configuration cache for class of object
 *  \param old_id    old object id
 *  \param new_id    new object id
 */

typedef void (*rename_object_f)(CacheBase* x, const std::string& old_id, const std::string& new_id);



struct DalFactoryFunctions
{
  notify2 m_update_fn;
  unread_object m_unread_object_fn;
  rename_object_f m_rename_object_fn;
  dal_object_creator m_creator_fn;

  std::set<std::string> m_algorithms;

  template<class T>
    DalFactoryFunctions(boost::compute::identity<T>, const std::set<std::string> algorithms);
};

#endif
