#include "domains.hxx"
#include <crails/datatree.hpp>
#include <libcurlez/curlez.hxx>

using namespace std;
using namespace Gandi;

static time_point parse_gandi_time(const string& str)
{
  if (!str.empty())
  {
    tm tm = {};
    istringstream ss(str);

    ss >> get_time(&tm, "%Y-%m-%dT%H:%M:%S"); // ISO 8601 format, like "2026-01-20T00:00:00Z"
    if (!ss.fail()) [[likely]]
    {
      time_t time_t_val = mktime(&tm);

      return chrono::system_clock::from_time_t(time_t_val);
    }
  }
  return {};
}

static void load_from_data(Domains::Domain& domain, Data data)
{
  domain.autorenew = data["autorenew"].as<bool>();
  domain.registry_created_at = parse_gandi_time(data["registry_created_at"].defaults_to<string>(""));
  domain.registry_ends_at = parse_gandi_time(data["registry_ends_at"].defaults_to<string>(""));
  domain.updated_at = parse_gandi_time(data["updated_at"].defaults_to<string>(""));
  domain.domain_owner = data["domain_owner"].as<string>();
  domain.orga_owner = data["orga_owner"].as<string>();
  domain.owner = data["owner"].as<string>();
  domain.fqdn = data["fqdn"].as<string>();
  domain.fqdn_unicode = data["fqdn_unicode"].as<string>();
  domain.href = data["href"].as<string>();
  domain.id = data["id"].as<string>();
  domain.tld = data["tld"].as<string>();
  domain.status = data["status"].to_vector<string>();
  domain.tags = data["tags"].to_vector<string>();
}

static void load_from_data(Domains::GlueRecord& record, Data data)
{
  record.fqdn = data["fqdn"].as<string>();
  record.fqdn_unicode = data["fqdn_unicode"].as<string>();
  record.href = data["href"].as<string>();
  record.ips = data["ips"].to_vector<string>();
  record.name = data["name"].as<string>();
}

static void load_from_data(Domains::LiveDnsStatus& status, Data data)
{
  status.current = data["current"].as<string>();
  status.nameservers = data["nameservers"].to_vector<string>();
  status.dnssec_available = data["dnssec_available"].defaults_to<bool>(false);
  status.livednssec_available = data["livednssec_available"].defaults_to<bool>(false);
}

template<typename OBJECT>
static void load_response(CurlReader& query, OBJECT& result)
{
  DataTree response;

  response.from_json(string(query.response_body()));
  load_from_data(result, response.as_data());
}

template<typename OBJECT>
static void load_response(const Domains& api, CurlReader& query, PagedList<OBJECT>& result)
{
  DataTree response;

  response.from_json(string(query.response_body()));
  result.count = api.get_total_count_from(query);
  for (Data entry : response.as_data())
  {
    OBJECT domain;

    load_from_data(domain, entry);
    result.list.push_back(domain);
  }
}

Domains::Domains(const string_view token) : Api(token)
{
}

PagedList<Domains::Domain> Domains::list(unsigned int page, unsigned int per_page) const
{
  PagedList<Domain> result{{}, 0};
  CurlReader query;
  ostringstream url;

  url << gandi_api_url << "/domain/domains";
  decorate_url(url, page, per_page);
  decorate_query(query);
  query.method("GET")
       .url(url.str());
  if (query.perform() == 200)
    load_response<Domain>(*this, query, result);
  return result;
}

optional<Domains::Domain> Domains::get(const string_view domain_name) const
{
  Domain domain;
  CurlReader query;
  ostringstream url;

  url << gandi_api_url << "/domain/domains/" << domain_name;
  decorate_query(query);
  query.method("GET").url(url.str());

  if (query.perform() == 200)
  {
    load_response(query, domain);
    return domain;
  }
  return {};
}

PagedList<Domains::GlueRecord> Domains::glue_records(const string_view domain_name, unsigned int page, unsigned int per_page) const
{
  PagedList<GlueRecord> result{{}, 0};
  CurlReader query;
  ostringstream url;

  url << gandi_api_url << "/domain/domains/" << domain_name << "/hosts";
  decorate_url(url, page, per_page);
  decorate_query(query);
  query.method("GET")
       .url(url.str());
  if (query.perform() == 200)
    load_response<GlueRecord>(*this, query, result);
  return result;
}

Domains::LiveDnsStatus Domains::livedns(const string_view domain_name) const
{
  LiveDnsStatus result;
  CurlReader query;
  ostringstream url;

  url << gandi_api_url << "/domain/domains/" << domain_name << "/livedns";
  decorate_query(query);
  query.method("GET")
       .url(url.str());
  if (query.perform() == 200)
    load_response<LiveDnsStatus>(query, result);
  return result;
}

Domains::Availability Domains::check_availability(const string_view domain_name) const
{
  Availability result;
  CurlReader query;
  ostringstream url;
  DataTree response;

  url << gandi_api_url << "/domain/check?name=" << domain_name << "&processes=create";
  decorate_query(query);
  query.method("GET")
       .url(url.str());
  if (query.perform() == 200)
  {
    response.from_json(string(query.response_body()));
    for (Data product : response["products"])
    {
      if (product["name"].defaults_to<string>("") == domain_name)
      {
        result.status = product["status"].defaults_to<string>("undefined");
        for (Data price : product["prices"])
        {
          result.price_before_taxes = price["price_before_taxes"].defaults_to<double>(0.0);
          result.price_after_taxes = price["price_after_taxes"].defaults_to<double>(0.0);
          result.currency = price["currency"].defaults_to<string>("EUR");
          break ;
        }
        break ;
      }
    }
  }
  else
    result.status = string(query.response_body());
  return result;
}

string Domains::Registration::to_json() const
{
  DataTree object;

  object["fqdn"] = fqdn;
  object["duration"] = duration_years;
  if (!owner_id.empty())
    object["owner"] = owner_id;
  return object.to_json();
}

Domains::RegistrationResult Domains::buy(const Registration& registration) const
{
  RegistrationResult result;
  CurlReader query;
  ostringstream url;
  string body = registration.to_json();

  url << gandi_api_url << "/domain/domains";
  decorate_query(query);
  query.method("POST")
       .url(url.str())
       .header("Content-Type", string_view("application/json"))
       .body(body);

  result.status = query.perform();
  if (!result)
    result.message = query.response_body();
  return result;
}

Domains::RegistrationResult Domains::renew(const std::string_view domain_name, int duration_years) const
{
  RegistrationResult result;
  CurlReader query;
  ostringstream url;
  DataTree body;

  body["duration"] = duration_years;
  url << gandi_api_url << "/domain/domains/" << domain_name << "/renew";
  decorate_query(query);
  query.method("POST")
       .url(url.str())
       .header("Content-Type", string_view("application/json"))
       .body(body.to_json());
  result.status = query.perform();
  if (!result)
    result.message = query.response_body();
  return result;
}

string Domains::Transfer::to_json() const
{
  DataTree object;
  object["fqdn"] = fqdn;
  object["authcode"] = auth_code;
  if (!owner_id.empty())
    object["owner"] = owner_id;
  return object.to_json();
}

Domains::RegistrationResult Domains::transfer(const Transfer& transfer) const
{
  RegistrationResult result;
  CurlReader query;
  ostringstream url;
  string body = transfer.to_json();

  url << gandi_api_url << "/domain/domains";
  decorate_query(query);
  query.method("POST")
       .url(url.str())
       .header("Content-Type", string_view("application/json"))
       .body(body);

  result.status = query.perform();
  if (!result)
    result.message = string(query.response_body());
  return result;
}
