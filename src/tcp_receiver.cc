#include "tcp_receiver.hh"
#include <iostream>
using namespace std;

void TCPReceiver::receive( TCPSenderMessage message, Reassembler& reassembler, Writer& inbound_stream )
{
  // Your code here.
  (void)message;
  (void)reassembler;
  (void)inbound_stream;
  if ( message.SYN ) {
    SYN = true;
    zero_point = message.seqno;
    if ( message.FIN || message.payload.size() > 0 )
      reassembler.insert( 0, message.payload, message.FIN, inbound_stream );
  } else {
    if ( SYN ) {
      // cout << reassembler.bytes_pending() << endl;
      reassembler.insert( message.seqno.unwrap( zero_point, reassembler.bytes_pending() ) - SYN,
                          message.payload,
                          message.FIN,
                          inbound_stream );
    }
  }
  FIN = inbound_stream.is_closed();
}

TCPReceiverMessage TCPReceiver::send( const Writer& inbound_stream ) const
{
  // Your code here.
  (void)inbound_stream;
  TCPReceiverMessage res
    = { .window_size = (uint16_t)min( inbound_stream.available_capacity(), (uint64_t)UINT16_MAX ) };
  if ( SYN )
    res.ackno = Wrap32::wrap( inbound_stream.bytes_pushed() + SYN + FIN, zero_point );
  return { res };
}
