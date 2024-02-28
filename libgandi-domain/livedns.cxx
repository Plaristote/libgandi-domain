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
  item["ttl"]        = record.ttl;
  item["rrset_values"].from_vector(record.values);
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
  query.method("GET")
       .url(url.str())
       .header("Authorization", authorization_header());
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
  query.method("GET")
       .url(url.str())
       .header("Authorization", authorization_header());
  if (query.perform() == 200)
  {
    DataTree response;
    vector<string> names;

    cout << "son of a whore: " << query.response_body() << ',' << get_total_count_from(query) << endl;
    response.from_json(string(query.response_body()));
    for (Data entry : response.as_data())
    {
      names.push_back(entry["fqdn"].as<string>());
    }
    return {names, get_total_count_from(query)};
  }
  cerr << "son of a bitch: " << query.response_body() << endl;
  return {{}, 0};
}

bool LiveDns::add_domain(const Domain& domain) const
{
  CurlReader query;
  ostringstream url;

  url << gandi_domain_api_url;
  query.method("POST")
       .url(url.str())
       .header("Authorization", authorization_header())
       .body(domain.to_json());
  if (query.perform() == 200)
  {
    return true;
  }
  else
    ; // TODO
  return false;
}

PagedList<LiveDns::DomainRecord> LiveDns::domain_records(const string_view domain_name, unsigned int page, unsigned int per_page) const
{
  CurlReader query;
  ostringstream url;
  int status;

  url << gandi_domain_api_url << '/' << domain_name << "/records";
  decorate_url(url, page, per_page);
  query.method("GET")
       .url(url.str())
       .header("Authorization", authorization_header());
  status = query.perform();
  if (status == 200)
  {
    DataTree response;
    vector<DomainRecord> records;

    response.from_json(string(query.response_body()));
    for (Data entry : response.as_data())
    {
      records.push_back({
        string(domain_name),
        entry["rrset_name"].as<string>(),
        entry["rrset_type"].as<string>(),
        entry["rrset_values"].to_vector<string>(),
        entry["rrset_ttl"].as<unsigned int>()
      });
    }
    return {records, get_total_count_from(query)};
  }
  return {{}, 0};
}

bool LiveDns::add_domain_record(const DomainRecord& record) const
{
  CurlReader query;
  ostringstream url;
  string body = record.to_json();

  url << gandi_domain_api_url << '/' << record.domain_name << "/records";
  query.method("POST")
       .url(url.str())
       .header("Authorization", authorization_header())
       .header("Content-Type", string_view("application/json"))
       .body(body);
  if (query.perform() == 200)
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
  if (status == 200)
    return true;
  cout << "Failed to update domain record: " << status << endl << query.error() << query.response_body() << endl;
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
  query.method("DELETE")
       .url(url.str())
       .header("Authorization", authorization_header());
  return query.perform() == 200;
}

vector<string> LiveDns::nameservers(const string_view domain_name) const
{
  CurlReader query;
  ostringstream url;

  url << gandi_domain_api_url << '/' << domain_name << "nameservers";
  query.method("GET")
       .url(url.str())
       .header("Authorization", authorization_header());
  if (query.perform() == 200)
  {
    DataTree response;
    vector<string> list;

    response.from_json(string(query.response_body()));
    return response.as_data().to_vector<string>();
  }
  return {};
}

