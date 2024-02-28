#include <sstream>
#include <stdexcept>

#include <libgandi-domain/version.hxx>
#include <libgandi-domain/gandi-domain.hxx>
#include <iostream>

#undef NDEBUG
#include <cassert>

int main ()
{
  using namespace std;
  const string_view token = "";

  {
    Gandi::Organizations organizations(token);
    Gandi::LiveDns livedns(token);
    livedns.add_domain_record({
      "planed.es",
      "universal",
      "A",
      vector<string>{"135.125.11.181"},
      10800
    });
  }

  return 0;

  {
    Gandi::Organizations organizations(token);
    Gandi::Domains gandi(token);
    Gandi::LiveDns livedns(token);

    cout << "Organization list:" << endl;
    for (const auto& org : organizations.list())
    {
      cout << " - " << org.id << ' ' << org.name << endl;
    }

    cout << "Domain list:" << endl;
    for (const auto& domain : gandi.list())
    {
      auto livedns_status = gandi.livedns(domain.fqdn);

      cout << "DOMAIN " << domain.fqdn << endl;
      cout << "LiveDns status: " << livedns_status.current << endl;
      cout << "LiveDns nameservers: " << endl;
      for (const auto& name : livedns_status.nameservers)
        cout << " --> " << name << endl;
      if (livedns_status.current == "livedns")
      {
        cout << "LiveDns Records:" << endl;
        for (const auto& record : livedns.domain_records(domain.fqdn))
          cout << " --> " << record.name << endl;
      }
      cout << "Glue Records:" << endl;
      for (const auto& record : gandi.glue_records(domain.fqdn))
      {
        cout << " --> " << record.name << endl;
      }
      cout << endl;
    }
    cout << endl;

    cout << "LiveDNS domain names:" << endl;
    for (const auto& name : livedns.domain_names())
      cout << " - " << name << endl;


    bool result = livedns.add_domain_record({
      "planed.es",
      "tototest",
      "A",
      vector<string>{"152.228.165.141"},
      1300
    });
    std::cout << "Adding domain record: " << result << endl;
  }

  return 0;
  {
    Gandi::LiveDns gandi("1df932be83a6e9f6e2083d872f66aa7a51c712b0");

    cout << "Accepted record types:" << endl;
    for (const string& type : gandi.accepted_record_types())
      cout << " - " << type << endl;
    cout << endl;

    cout << "Domains:" << endl;
    for (const string& name : gandi.domain_names())
      cout << " - " << name << endl;
    cout << endl;

    cout << "Domain records:" << endl;
    for (const auto& record : gandi.domain_records("planed.es"))
      cout << " - " << record.name << ' ' << record.type << endl;
    cout << endl;
  }

  return 0;
}
