#pragma once
#include "api.hxx"
#include "paged-list.hxx"

namespace Gandi
{
  class LIBGANDI_DOMAIN_SYMEXPORT Organizations : public Gandi::Api
  {
  public:
    struct Organization
    {
      std::string id;
      std::string name;
      bool corporate;
      std::string email;
      std::string firstname;
      std::string lastname;
      std::string orgname;
      bool reseller;
      std::string siren;
      std::string type;
      std::string vat_number;
    };

    Organizations(const std::string_view);

    PagedList<Organization> list(unsigned int page = 0, unsigned int per_page = 0) const;
  };
}
