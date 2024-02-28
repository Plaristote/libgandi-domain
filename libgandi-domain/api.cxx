#include "api.hxx"
#include <libcurlez/curlez.hxx>
#include <cstdlib>

using namespace std;

const string_view Gandi::Api::gandi_api_url = "https://api.gandi.net/v5";

Gandi::Api::Api(const string_view token) : token(token)
{
}

string Gandi::Api::authorization_header() const
{
  return "Bearer " + token;
}

void Gandi::Api::decorate_query(Curl& query) const
{
  query.header("Authorization", authorization_header());
  //query.option(CURLOPT_VERBOSE, 1L);
}

void Gandi::Api::decorate_url(ostringstream& stream, unsigned int page, unsigned int per_page) const
{
  if (per_page > 0)
  {
    stream << "?page=" << page << "&per_page=" << per_page;
  }
}

unsigned int Gandi::Api::get_total_count_from(CurlReader& query) const
{
  return static_cast<unsigned int>(
    std::atoi(query.response_header("Total-Count").c_str())
  );
}
