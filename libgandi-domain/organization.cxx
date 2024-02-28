#include "organization.hxx"
#include <crails/datatree.hpp>
#include <libcurlez/curlez.hxx>

using namespace std;
using namespace Gandi;

static void load_from_data(Organizations::Organization& org, Data data)
{
  org.id = data["id"].as<string>();
  org.name = data["name"].as<string>();
  org.email = data["email"].defaults_to<string>("");
  org.firstname = data["firstname"].defaults_to<string>("");
  org.lastname = data["lastname"].defaults_to<string>("");
  org.orgname = data["orgname"].defaults_to<string>("");
  org.corporate = data["corporate"].defaults_to<bool>(false);
  org.reseller = data["reseller"].defaults_to<bool>(false);
  org.siren = data["siren"].defaults_to<string>("");
  org.type = data["type"].defaults_to<string>("");
  org.vat_number = data["vat_number"].defaults_to<string>("");
}

Organizations::Organizations(const string_view token) : Api(token)
{
}

PagedList<Organizations::Organization> Organizations::list(unsigned int page, unsigned int per_page) const
{
  PagedList<Organization> result{{}, 0};
  CurlReader query;
  ostringstream url;

  url << gandi_api_url << "/organization/organizations";
  decorate_url(url, page, per_page);
  decorate_query(query);
  query.method("GET")
       .url(url.str());
  if (query.perform() == 200)
  {
    DataTree response;

    response.from_json(string(query.response_body()));
    result.count = get_total_count_from(query);
    for (Data entry : response.as_data())
    {
      Organization domain;

      load_from_data(domain, entry);
      result.list.push_back(domain);
    }
  }
  return result;
}
