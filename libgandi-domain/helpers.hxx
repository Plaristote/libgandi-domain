#pragma once
#include <libcurlez/curlez.hxx>
#include <crails/datatree.hpp>
#include "paged-list.hxx"
#include "api.hxx"

namespace Gandi
{
  template<typename OBJECT>
  void load_response(CurlReader& query, OBJECT& result)
  {
    DataTree response;

    response.from_json(std::string(query.response_body()));
    load_from_data(result, response.as_data());
  }

  template<typename OBJECT>
  void load_response(const Gandi::Api& api, CurlReader& query, PagedList<OBJECT>& result)
  {
    DataTree response;

    response.from_json(std::string(query.response_body()));
    result.count = api.get_total_count_from(query);
    for (Data entry : response.as_data())
    {
      OBJECT domain;

      load_from_data(domain, entry);
      result.list.push_back(domain);
    }
  }
}
