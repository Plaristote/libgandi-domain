#pragma once
#include "api.hxx"
#include "paged-list.hxx"
#include <optional>

namespace Gandi
{
  class LIBGANDI_DOMAIN_SYMEXPORT Domains : public Gandi::Api
  {
  public:
    struct Domain
    {
      bool autorenew = false;
      time_point registry_created_at;
      time_point registry_ends_at;
      time_point renew_begins_at;
      time_point updated_at;
      std::string domain_owner;
      std::string orga_owner;
      std::string owner;
      std::string fqdn;
      std::string fqdn_unicode;
      std::string href;
      std::string id;
      std::string tld;
      std::vector<std::string> status;
      std::vector<std::string> tags;
    };

    struct GlueRecord
    {
      std::string fqdn;
      std::string fqdn_unicode;
      std::string href;
      std::vector<std::string> ips;
      std::string name;
    };

    struct LiveDnsStatus
    {
      std::string current;
      std::vector<std::string> nameservers;
      bool dnssec_available;
      bool livednssec_available;
    };

    struct Availability
    {
      std::string name;
      std::string status;
      double price_before_taxes = 0.0;
      double price_after_taxes = 0.0;
      std::string currency;
      operator bool() const { return status == "available"; }
    };

    struct Registration
    {
      std::string fqdn;
      int duration_years = 1;
      std::string owner_id;
      std::string to_json() const;
    };

    struct RegistrationResult
    {
      int status = 0;
      std::string message;
      operator bool() const { return status == 201 || status == 200; }
    };

    struct Transfer
    {
      std::string fqdn;
      std::string auth_code;
      std::string owner_id;
      std::string to_json() const;
    };

    struct Lookup
    {
      int status = 0;
      std::string error;
      std::optional<Domain> domain;
      operator bool() const { return status == 200 && domain.has_value(); }
      bool is_unknown() const { return status == 404 || status == 403; }
    };

    struct ListResult
    {
      int status = 0;
      std::string error;
      std::vector<Domain> domains;
      bool ok() const { return status == 200; }
      operator bool() const { return ok(); }
    };

    Domains(const std::string_view token);

    PagedList<Domain> list(unsigned int page = 0, unsigned int per_page = 0) const;
    ListResult list_all(unsigned int per_page = 100) const;
    std::optional<Domain> get(const std::string_view domain_name) const;
    Lookup lookup(const std::string_view domain_name) const;
    PagedList<GlueRecord> glue_records(const std::string_view domain_name, unsigned int page = 0, unsigned int per_page = 0) const;
    LiveDnsStatus livedns(const std::string_view) const;
    Availability check_availability(const std::string_view) const;
    RegistrationResult buy(const Registration&) const;
    RegistrationResult renew(const std::string_view, int duration_years = 1) const;
    RegistrationResult transfer(const Transfer&) const;
  };
}
