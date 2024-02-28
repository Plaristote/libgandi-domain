#pragma once
#include <string>
#include <vector>

namespace Gandi
{
  template<typename TYPE = std::string>
  struct PagedList
  {
    typedef TYPE value_type;
    std::vector<TYPE> list;
    unsigned int count;
    operator std::vector<TYPE>() const { return list; }
    std::vector<TYPE>::iterator begin() { return std::begin(list); }
    std::vector<TYPE>::iterator end() { return std::end(list); }
    std::vector<TYPE>::const_iterator cbegin() const { return std::cbegin(list); }
    std::vector<TYPE>::const_iterator cend() const { return std::cend(list); }
  };
}
