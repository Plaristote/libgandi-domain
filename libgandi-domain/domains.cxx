#include "domains.hxx"
#include <crails/datatree.hpp>
#include <libcurlez/curlez.hxx>

using namespace std;
using namespace Gandi;

static void load_from_data(Domains::Domain& domain, Data data)
{
  domain.autorenew = data["autorenew"].as<bool>();
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
