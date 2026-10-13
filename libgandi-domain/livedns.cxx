#include "helpers.hxx"
#include <libgandi-domain/gandi-domain.hxx>
#include <libcurlez/curlez.hxx>
#include <crails/datatree.hpp>
#include <cstdlib>
#include <iostream>

using namespace std;
using namespace Gandi;

static const string_view gandi_domain_api_url = "https://api.gandi.net/v5/livedns/domains";

static void write_to(const LiveDns::DomainRecord& record, Data item)
{
  item["rrset_name"] = record.name;
  item["rrset_type"] = record.type;
  item["rrset_ttl"]  = record.ttl;
  item["rrset_values"].from_vector(record.values);
}

static void load_from_data(LiveDns::DomainRecord& record, Data data)
{
  record.name   = data["rrset_name"].defaults_to<string>(record.name);
  record.type   = data["rrset_type"].defaults_to<string>(record.type);
  record.values = data["rrset_values"].to_vector<string>();
  record.ttl    = data["rrset_ttl"].defaults_to<unsigned int>(0);
}

LiveDns::LiveDns(const string_view token) : Api(token)
{
}

string LiveDns::DomainRecord::to_json() const
{
  DataTree item;
  write_to(*this, item);
  return item.to_json();
}

string LiveDns::Domain::to_json() const
{
  DataTree object;
  Data items = object["zone"]["items"];

  object["fqdn"] = fqdn;
  for (const DomainRecord& record : records)
  {
    DataTree item;

    write_to(record, item.as_data());
    items.push_back(item.as_data());
  }
  return object.to_json();
}

vector<string> LiveDns::accepted_record_types() const
{
  CurlReader query;
  ostringstream url;

  url << gandi_api_url << "/livedns/dns/rrtypes";
  decorate_query(query);
  query.method("GET")
       .url(url.str());
  if (query.perform() == 200)
  {
    DataTree response;

    response.from_json(string(query.response_body()));
    return response.as_data().to_vector<string>();
  }
  else
    ; // TODO report error
  return {};
}

PagedList<string> LiveDns::domain_names(unsigned int page, unsigned int per_page) const
{
  CurlReader query;
  ostringstream url;

  url << gandi_domain_api_url;
  decorate_url(url, page, per_page);
  decorate_query(query);
  query.method("GET")
       .url(url.str());
  if (query.perform() == 200)
  {
    DataTree response;
    vector<string> names;

    response.from_json(string(query.response_body()));
    for (Data entry : response.as_data())
    {
      names.push_back(entry["fqdn"].as<string>());
    }
    return {names, get_total_count_from(query)};
  }
  cerr << "Failed to list LiveDNS domains: " << query.response_body() << endl;
  return {{}, 0};
}

bool LiveDns::add_domain(const Domain& domain) const
{
  CurlReader query;
  ostringstream url;

  url << gandi_domain_api_url;
  decorate_query(query);
  query.method("POST")
       .url(url.str())
       .header("Content-Type", string_view("application/json"))
       .body(domain.to_json());
  if (is_success(query.perform()))
    return true;
  cerr << "Failed to add LiveDNS domain:" << endl << query.response_body() << endl;
  return false;
}

PagedList<LiveDns::DomainRecord> LiveDns::domain_records(const string_view domain_name, unsigned int page, unsigned int per_page) const
{
  CurlReader query;
  ostringstream url;
  int status;
  PagedList<LiveDns::DomainRecord> result{{}, 0};

  url << gandi_domain_api_url << '/' << domain_name << "/records";
  decorate_url(url, page, per_page);
  decorate_query(query);
  query.method("GET")
       .url(url.str());
  status = query.perform();
  if (status == 200)
    load_response<DomainRecord>(*this, query, result);
  return result;
}

optional<LiveDns::DomainRecord> LiveDns::domain_record(const string_view domain_name, const string_view name, const string_view type) const
{
  RecordLookup lookup = lookup_domain_record(domain_name, name, type);

  if (lookup.failed())
    cerr << "Failed to read domain record: " << lookup.status << endl << lookup.error << endl;
  return lookup.record;
}

LiveDns::RecordLookup LiveDns::lookup_domain_record(const string_view domain_name, const string_view name, const string_view type) const
{
  RecordLookup result;
  CurlReader query;
  ostringstream url;

  url << gandi_domain_api_url << '/' << domain_name << "/records/" << name << '/' << type;
  decorate_query(query);
  query.method("GET").url(url.str());
  result.status = query.perform();
  try
  {
    if (result.status == 200)
    {
      DomainRecord result{string(domain_name), string(name), string(type)};

      load_response<DomainRecord>(query, result);
    }
    else if (result.status != 404)
    {
      result.error = query.error();
      if (result.error.empty())
        result.error = string(query.response_body());
    }
  }
  catch (const std::exception& error)
  {
    result.status = 0;
    result.error = string("unexpected response: ") + error.what();
  }
  return result;
}

bool LiveDns::add_domain_record(const DomainRecord& record) const
{
  CurlReader query;
  ostringstream url;
  string body = record.to_json();

  url << gandi_domain_api_url << '/' << record.domain_name << "/records";
  decorate_query(query);
  query.method("POST")
       .url(url.str())
       .header("Content-Type", string_view("application/json"))
       .body(body);
  if (is_success(query.perform()))
    return true;
  else
    cerr << "Failed to add domain record:" << endl << query.response_body() << endl;
  return false;
}

bool LiveDns::update_domain_record(const DomainRecord& record) const
{
  CurlReader query;
  ostringstream url;
  int status;
  string body = record.to_json();

  url << gandi_domain_api_url << '/' << record.domain_name
      << "/records/"
      << record.name << '/'
      << record.type;
  decorate_query(query);
  query.url(url.str())
       .method("PUT")
       .header("Content-Type", string("application/json"))
       .body(body);
  status = query.perform();
  if (is_success(status))
    return true;
  cerr << "Failed to update domain record: " << status << endl << query.error() << query.response_body() << endl;
  return false;
}

bool LiveDns::delete_domain_record(const DomainRecord& record) const
{
  Curl query;
  ostringstream url;

  url << gandi_domain_api_url << '/' << record.domain_name
      << "/records/"
      << record.name << '/'
      << record.type;
  decorate_query(query);
  query.method("DELETE")
       .url(url.str());
  return is_success(query.perform());
}

vector<string> LiveDns::nameservers(const string_view domain_name) const
{
  CurlReader query;
  ostringstream url;

  url << gandi_domain_api_url << '/' << domain_name << "/nameservers";
  decorate_query(query);
  query.method("GET")
       .url(url.str());
  if (query.perform() == 200)
  {
    DataTree response;
    vector<string> list;

    response.from_json(string(query.response_body()));
    return response.as_data().to_vector<string>();
  }
  return {};
}

