#include "domains.hxx"
#include "helpers.hxx"
#include <crails/datatree.hpp>
#include <libcurlez/curlez.hxx>
#include <ctime>
#include <iomanip>

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
      return chrono::system_clock::from_time_t(timegm(&tm));
  }
  return {};
}

static time_point load_date(Data data, const string& key)
{
  Data dates = data["dates"];

  if (dates.exists() && dates[key].exists())
    return parse_gandi_time(dates[key].defaults_to<string>(""));
  return parse_gandi_time(data[key].defaults_to<string>(""));
}

static bool load_autorenew(Data data)
{
  Data autorenew = data["autorenew"];
  bool result = false;

  if (autorenew.exists())
  {
    if (autorenew["enabled"].exists())
      autorenew = autorenew["enabled"];
    try { result = autorenew.defaults_to<bool>(false); }
    catch (const std::exception&) {}
  }
  return result;
}

static void load_from_data(Domains::Domain& domain, Data data)
{
  domain.autorenew = load_autorenew(data);
  domain.registry_created_at = load_date(data, "registry_created_at");
  domain.registry_ends_at = load_date(data, "registry_ends_at");
  domain.renew_begins_at = load_date(data, "renew_begins_at");
  domain.updated_at = load_date(data, "updated_at");
  domain.domain_owner = data["domain_owner"].defaults_to<string>("");
  domain.orga_owner = data["orga_owner"].defaults_to<string>("");
  domain.owner = data["owner"].defaults_to<string>("");
  domain.fqdn = data["fqdn"].defaults_to<string>("");
  domain.fqdn_unicode = data["fqdn_unicode"].defaults_to<string>("");
  domain.href = data["href"].defaults_to<string>("");
  domain.id = data["id"].defaults_to<string>("");
  domain.tld = data["tld"].defaults_to<string>("");
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

Domains::ListResult Domains::list_all(unsigned int per_page) const
{
  ListResult result;
  const unsigned int max_pages = 100; // Can't loop forever

  if (per_page == 0)
    per_page = 100;
  for (unsigned int page = 1 ; page <= max_pages ; ++page)
  {
    CurlReader query;
    ostringstream url;
    PagedList<Domain> paged{{}, 0};
    bool is_last_page = false;

    url << gandi_api_url << "/domain/domains";
    decorate_url(url, page, per_page);
    decorate_query(query);
    query.method("GET").url(url.str());
    result.status = query.perform();
    if (result.status == 200)
    {
      try
      {
        load_response<Domain>(*this, query, paged);
        is_last_page = paged.list.size() < per_page || (paged.count > 0 && result.domains.size() >= paged.count);
        result.domains.insert(result.domains.end(), paged.list.begin(), paged.list.end());
      }
      catch (const std::exception& error)
      {
        result.status = 0;
        result.error = string("unexpected response: ") + error.what();
        is_last_page = true;
      }
      if (is_last_page)
        break ;
    }
    else
    {
      result.error = query.error();
      if (result.error.empty())
        result.error = string(query.response_body());
      break ;
    }
  }
  return result;
}

optional<Domains::Domain> Domains::get(const string_view domain_name) const
{
  return lookup(domain_name).domain;
}

Domains::Lookup Domains::lookup(const string_view domain_name) const
{
  Lookup result;
  CurlReader query;
  ostringstream url;

  url << gandi_api_url << "/domain/domains/" << domain_name;
  decorate_query(query);
  query.method("GET").url(url.str());

  result.status = query.perform();
  if (result.status == 200)
  {
    try
    {
      Domain domain;

      load_response(query, domain);
      result.domain = domain;
    }
    catch (const std::exception& error)
    {
      result.error = string("unexpected response: ") + error.what();
    }
  }
  else
  {
    result.error = query.error();
    if (result.error.empty())
      result.error = string(query.response_body());
  }
  return result;
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
