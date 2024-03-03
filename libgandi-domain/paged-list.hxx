#pragma once
#include <string>
#include <vector>

namespace Gandi
{
  template<typename TYPE = std::string>
  struct PagedList
  {
    typedef typename std::vector<TYPE> ListType;
    typedef TYPE value_type;
    ListType list;
    unsigned int count;
    operator std::vector<TYPE>() const { return list; }
    typename ListType::iterator begin() { return std::begin(list); }
    typename ListType::iterator end() { return std::end(list); }
    typename ListType::const_iterator cbegin() const { return std::cbegin(list); }
    typename ListType::const_iterator cend() const { return std::cend(list); }
  };
}
