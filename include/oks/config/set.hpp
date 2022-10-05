#ifndef CONFIG_SET_H_
#define CONFIG_SET_H_

#include "config/string_ptr.hpp"
#include <unordered_set>

namespace config
{
  typedef std::unordered_set<std::string> set;

  // compare string pointers (not values!)
  typedef std::unordered_set<const std::string *, string_ptr_hash> fset;
}

#endif // CONFIG_SET_H_
