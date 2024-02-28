#pragma once
#include <libgandi-domain/export.hxx>
#include <string>
#include <string_view>
#include <sstream>
#include <chrono>

class Curl;
class CurlReader;

namespace Gandi
{
  typedef std::chrono::time_point<std::chrono::system_clock> time_point;

  class LIBGANDI_DOMAIN_SYMEXPORT Api
  {
    std::string token;
  public:
    Api(const std::string_view token);
    std::string authorization_header() const;

    unsigned int get_total_count_from(CurlReader&) const;
    void decorate_query(Curl&) const;
    void decorate_url(std::ostringstream&, unsigned int page, unsigned int per_page) const;
  protected:
    static const std::string_view gandi_api_url;
  };
}
