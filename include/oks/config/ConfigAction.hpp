#ifndef CONFIG_ACTION_H_
#define CONFIG_ACTION_H_

#include <string>

#include <config/Change.hpp>

class ConfigObject;

  /** An action is called when configuration is unload or system notification callback is coming. */

class ConfigAction {

  public:

    /// Call action on database changes
    virtual void notify( std::vector<ConfigurationChange *>& changes ) noexcept = 0;

    /// Call action on database file(s) load
    virtual void load( ) noexcept = 0;

    /// Call action on database file(s) unload
    virtual void unload( ) noexcept = 0;

    /// Call action on database object modification by user's code
    virtual void update( const ConfigObject& obj, const std::string& name) noexcept = 0;

};

#endif // CONFIG_DAL_ACTION_H_
