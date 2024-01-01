#include "router.hh"

#include <iostream>
#include <limits>

using namespace std;

// route_prefix: The "up-to-32-bit" IPv4 address prefix to match the datagram's destination address against
// prefix_length: For this route to be applicable, how many high-order (most-significant) bits of
//    the route_prefix will need to match the corresponding bits of the datagram's destination address?
// next_hop: The IP address of the next hop. Will be empty if the network is directly attached to the router (in
//    which case, the next hop address should be the datagram's final destination).
// interface_num: The index of the interface to send the datagram out on.
void Router::add_route( const uint32_t route_prefix,
                        const uint8_t prefix_length,
                        const optional<Address> next_hop,
                        const size_t interface_num )
{
  cerr << "DEBUG: adding route " << Address::from_ipv4_numeric( route_prefix ).ip() << "/"
       << static_cast<int>( prefix_length ) << " => " << ( next_hop.has_value() ? next_hop->ip() : "(direct)" )
       << " on interface " << interface_num << "\n";

  (void)route_prefix;
  (void)prefix_length;
  (void)next_hop;
  (void)interface_num;
  route_table_.emplace_back(route_datagram {route_prefix, prefix_length, next_hop, interface_num});
}

void Router::route() {
  for (auto &interface_temp:interfaces_) {
    while (optional<InternetDatagram> dg = interface_temp.maybe_receive()) {
      if (dg.has_value()) {
        const uint32_t ipdst = dg.value().header.dst;
        auto largest_prefix_match_route = route_table_.end();
        for (auto r=route_table_.begin(); r!=route_table_.end(); r++) {
          if (r->prefix_length==0 || 
          (r->route_prefix ^ ipdst) >> (static_cast<uint8_t>(32) - r->prefix_length) == 0) {
            if (largest_prefix_match_route==route_table_.end() || largest_prefix_match_route->prefix_length < r->prefix_length) {
              largest_prefix_match_route = r;
            }
          }
        }

        uint8_t &dg_ttl = dg.value().header.ttl;
        if (largest_prefix_match_route!=route_table_.end() && dg_ttl>1) {
          dg_ttl--;
          dg.value().header.compute_checksum();
          const Address n_hop = (largest_prefix_match_route->next_hop.has_value() ? largest_prefix_match_route->next_hop.value()
          : Address::from_ipv4_numeric(dg.value().header.dst));
          interface(largest_prefix_match_route->interface_num).send_datagram(dg.value(), n_hop);
        }
      }
    }
  }
}
