#pragma once
#include "api.hxx"
#include "paged-list.hxx"

namespace Gandi
{
  class LIBGANDI_DOMAIN_SYMEXPORT Domains : public Gandi::Api
  {
  public:
    struct Domain
    {
      bool autorenew;
      time_point registry_created_at;
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

    Domains(const std::string_view token);

    PagedList<Domain> list(unsigned int page = 0, unsigned int per_page = 0) const;
    PagedList<GlueRecord> glue_records(const std::string_view domain_name, unsigned int page = 0, unsigned int per_page = 0) const;

    LiveDnsStatus livedns(const std::string_view) const;
  };
}
