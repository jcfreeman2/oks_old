/**
 * @file config/ConfigurationPointer.h
 * @author <a href="mailto:andre.dos.anjos@cern.ch">Andre Anjos</a> 
 *
 * @brief A wrapper to the Configuration class to make it boost friendly
 */

#ifndef PYTHONCONFIGURATION_H 
#define PYTHONCONFIGURATION_H

#include <config/Configuration.hpp>
#include <boost/shared_ptr.hpp>

namespace python {

  /**
   * Use this class only when you want to bridge your C++ code with python. It
   * implements some weird reference counted pointer which helps a clean API
   * for the python bindings and the DBE
   */
  class ConfigurationPointer {
  
  public:
    /// Default
    ConfigurationPointer(): m_shared(), m_pointer(0) {}

    /// Constructor when the user initializes from within python
    ConfigurationPointer(const std::string& s)
      : m_shared(new Configuration(s)), m_pointer(0) {}
    
    /// Constructor when the user initializes from C++. In this case, the pointed configuration should never be deleted.
    ConfigurationPointer(const Configuration& p)
      : m_shared(), 
        m_pointer(&const_cast<Configuration&>(p)) {}

    /// Copying, is shallow. Don't do this at home...
    ConfigurationPointer (const ConfigurationPointer& other)
      : m_shared(other.m_shared), 
        m_pointer(const_cast<ConfigurationPointer&>(other).m_pointer) {}

    /// Assignment is shallow
    ConfigurationPointer& operator= (const ConfigurationPointer& other)
    {
      m_shared = other.m_shared;
      m_pointer = const_cast<ConfigurationPointer&>(other).m_pointer;
      return *this;
    }

    /// Destructor. If started from string, destroy if the last instance, otherwise, doesn't do anything at all.
    virtual ~ConfigurationPointer () {}

    /// Let python bindings access the configuration pointer
    Configuration* operator->() { return (m_pointer)?m_pointer:m_shared.get(); }

    /// Another way to access the configuration pointer
    Configuration& operator*() { return (m_pointer)?*m_pointer:*m_shared; }

    /// Get the configuration pointer
    Configuration* get() { return (m_pointer)?m_pointer:m_shared.get(); }

  private:
    boost::shared_ptr<Configuration> m_shared; //self-initialized Configuration
    Configuration* m_pointer; //user initialized Configuration
    
  };

}

#endif /* PYTHONCONFIGURATION_H */

