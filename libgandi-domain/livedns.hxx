#pragma once
#include "api.hxx"
#include "paged-list.hxx"
#include <vector>

namespace Gandi
{
  class LIBGANDI_DOMAIN_SYMEXPORT LiveDns : public Gandi::Api
  {
  public:
    struct DomainRecord
    {
      std::string domain_name;
      std::string name;
      std::string type;
      std::vector<std::string> values;
      unsigned int ttl;
      std::string to_json() const;
    };

    struct Domain
    {
      std::string fqdn;
      std::vector<DomainRecord> records;
      std::string to_json() const;
      operator std::string() const { return fqdn; }
    };

    LiveDns(const std::string_view token);

    std::vector<std::string> accepted_record_types() const;
    PagedList<std::string> domain_names(unsigned int page = 0, unsigned int per_page = 0) const;
    bool add_domain(const Domain&) const;
    PagedList<DomainRecord> domain_records(const Domain& domain, unsigned int page = 0, unsigned int per_page = 0) const { return domain_records(domain.fqdn, page, per_page); }
    PagedList<DomainRecord> domain_records(const std::string_view domain_name, unsigned int page = 0, unsigned int per_page = 0) const;
    bool add_domain_record(const DomainRecord&) const;
    bool update_domain_record(const DomainRecord&) const;
    bool delete_domain_record(const DomainRecord&) const;
    std::vector<std::string> nameservers(const Domain& domain) const { return nameservers(domain.fqdn); }
    std::vector<std::string> nameservers(const std::string_view domain_name) const;
  };
}
