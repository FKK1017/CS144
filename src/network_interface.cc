#include "network_interface.hh"

#include "arp_message.hh"
#include "ethernet_frame.hh"

using namespace std;

// ethernet_address: Ethernet (what ARP calls "hardware") address of the interface
// ip_address: IP (what ARP calls "protocol") address of the interface
NetworkInterface::NetworkInterface( const EthernetAddress& ethernet_address, const Address& ip_address )
  : ethernet_address_( ethernet_address ), ip_address_( ip_address )
{
  cerr << "DEBUG: Network interface has Ethernet address " << to_string( ethernet_address_ ) << " and IP address "
       << ip_address.ip() << "\n";
}

// dgram: the IPv4 datagram to be sent
// next_hop: the IP address of the interface to send it to (typically a router or default gateway, but
// may also be another host if directly connected to the same network as the destination)

// Note: the Address type can be converted to a uint32_t (raw 32-bit IP address) by using the
// Address::ipv4_numeric() method.
void NetworkInterface::send_datagram( const InternetDatagram& dgram, const Address& next_hop )
{
  uint32_t ip = next_hop.ipv4_numeric();
  if ( address_table_.find( ip ) != address_table_.end() ) {
    // already known
    EthernetFrame eframe;
    eframe.header.src = ethernet_address_;
    eframe.header.dst = address_table_[ip].first;
    eframe.header.type = EthernetHeader::TYPE_IPv4;
    eframe.payload = serialize( dgram );
    outbound_frames_.push( eframe );
  } else {
    // unknown, broadcast IP
    if ( arp_req_lifetime_.find( ip ) == arp_req_lifetime_.end() ) {
      // not broadcast
      ARPMessage arp;
      arp.sender_ethernet_address = ethernet_address_;
      arp.sender_ip_address = ip_address_.ipv4_numeric();
      arp.target_ip_address = ip;
      arp.opcode = arp.OPCODE_REQUEST;

      EthernetFrame eframe;
      eframe.header.src = ethernet_address_;
      eframe.header.dst = ETHERNET_BROADCAST;
      eframe.header.type = EthernetHeader::TYPE_ARP;
      eframe.payload = serialize( arp );
      outbound_frames_.push( eframe );
      arp_req_lifetime_[ip] = ARP_DEFAULT_LIFE_TIME;
    }
    waiting_IPdatagram_.emplace_back( make_pair( next_hop, dgram ) );
  }
}

// frame: the incoming Ethernet frame
optional<InternetDatagram> NetworkInterface::recv_frame( const EthernetFrame& frame )
{
  if ( frame.header.dst != ethernet_address_ && frame.header.dst != ETHERNET_BROADCAST ) {
    return nullopt;
  }

  if ( frame.header.type == EthernetHeader::TYPE_IPv4 ) {
    InternetDatagram ipv4_data;
    if ( parse( ipv4_data, frame.payload ) )
      return ipv4_data;
    else
      return nullopt;
  }

  if ( frame.header.type == EthernetHeader::TYPE_ARP ) {
    ARPMessage arp_data;
    if ( parse( arp_data, frame.payload ) ) {
      address_table_[arp_data.sender_ip_address]
        = make_pair( arp_data.sender_ethernet_address, ARP_ETH_IP_LIFE_TIME );
      if ( arp_data.opcode == arp_data.OPCODE_REQUEST
           && arp_data.target_ip_address == ip_address_.ipv4_numeric() ) {
        ARPMessage arp_reply;
        arp_reply.sender_ethernet_address = ethernet_address_;
        arp_reply.sender_ip_address = ip_address_.ipv4_numeric();
        arp_reply.target_ethernet_address = arp_data.sender_ethernet_address;
        arp_reply.target_ip_address = arp_data.sender_ip_address;
        arp_reply.opcode = arp_reply.OPCODE_REPLY;

        EthernetFrame eframe;
        eframe.header.src = ethernet_address_;
        eframe.header.dst = arp_data.sender_ethernet_address;
        eframe.header.type = EthernetHeader::TYPE_ARP;
        eframe.payload = serialize( arp_reply );
        outbound_frames_.push( eframe );
      }
      for ( auto iter = waiting_IPdatagram_.begin(); iter != waiting_IPdatagram_.end(); ) {
        const auto& [ip_address, data] = *iter;
        if ( ip_address.ipv4_numeric() == arp_data.sender_ip_address ) {
          send_datagram( data, ip_address );
          iter = waiting_IPdatagram_.erase( iter );
        } else
          iter++;
      }
      arp_req_lifetime_.erase( arp_data.sender_ip_address );
    } else
      return nullopt;
  }
  return nullopt;
}

// ms_since_last_tick: the number of milliseconds since the last call to this method
void NetworkInterface::tick( const size_t ms_since_last_tick )
{
  (void)ms_since_last_tick;
  for ( auto iter = address_table_.begin(); iter != address_table_.end(); ) {
    auto& [ip, data] = *iter;
    if ( data.second <= ms_since_last_tick ) {
      iter = address_table_.erase( iter );
    } else {
      data.second -= ms_since_last_tick;
      iter++;
    }
  }

  for ( auto iter = arp_req_lifetime_.begin(); iter != arp_req_lifetime_.end(); ) {
    auto& [ip, data] = *iter;
    if ( data <= ms_since_last_tick ) {
      iter = arp_req_lifetime_.erase( iter );
    } else {
      data -= ms_since_last_tick;
      iter++;
    }
  }
}

optional<EthernetFrame> NetworkInterface::maybe_send()
{
  if ( outbound_frames_.empty() )
    return nullopt;
  else {
    EthernetFrame eframe = outbound_frames_.front();
    outbound_frames_.pop();
    return eframe;
  }
}
