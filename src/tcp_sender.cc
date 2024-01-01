#include "tcp_sender.hh"
#include "tcp_config.hh"

#include <iostream>
#include <random>
using namespace std;

/* TCPSender constructor (uses a random ISN if none given) */
TCPSender::TCPSender( uint64_t initial_RTO_ms, optional<Wrap32> fixed_isn )
  : isn_( fixed_isn.value_or( Wrap32 { random_device()() } ) )
  , initial_RTO_ms_( initial_RTO_ms )
  , sending_msg()
  , outstanding_msg_()
  , ack_receive_( 0 )
  , ack_send_( 0 )
  , retrans_( 0 )
  , seq_in_flight_( 0 )
  , window_size_( 0 )
  , syn_sended_( false )
  , syn_received_( false )
  , fin_sended_( false )
  , timer( Timer( initial_RTO_ms, 0 ) )
{}

uint64_t TCPSender::sequence_numbers_in_flight() const
{
  // Your code here.
  // cout << "sequence_numbers_in_flight" << endl;
  return seq_in_flight_;
}

uint64_t TCPSender::consecutive_retransmissions() const
{
  // Your code here.
  // cout << "consecutive_retransmissions" << endl;
  return { retrans_ };
}

optional<TCPSenderMessage> TCPSender::maybe_send()
{
  // Your code here.
  // cout << "maybe_send\n";
  if ( !sending_msg.empty() && syn_sended_ ) {
    TCPSenderMessage msg = move( sending_msg.front() );
    ////cout << msg.SYN << ' ' << msg.FIN << endl;
    sending_msg.pop_front();
    return { msg };
  } else
    return nullopt;
}

void TCPSender::push( Reader& outbound_stream )
{
  // Your code here.
  // cout << "push\n" ;
  if ( fin_sended_ )
    return;
  const uint16_t push_window = window_size_ ? window_size_ : 1;
  // cout << "push\n" ;
  while ( push_window > seq_in_flight_ ) {
    string s;
    TCPSenderMessage msg = { .payload = s };

    if ( !syn_sended_ ) {
      msg.SYN = true;
      syn_sended_ = true;
    }

    msg.seqno = Wrap32::wrap( ack_send_, isn_ );
    // cout << push_window << ' ' << seq_in_flight_+msg.SYN << endl;
    uint16_t ava_window = push_window - seq_in_flight_ - msg.SYN;

    uint16_t buf_size = min( (uint16_t)TCPConfig::MAX_PAYLOAD_SIZE, ava_window );

    read( outbound_stream, buf_size, s );
    // if (!s.empty()) cout << s << endl;

    if ( !fin_sended_ && outbound_stream.is_finished() && push_window > seq_in_flight_ + s.size() + msg.SYN ) {
      msg.FIN = true;
      fin_sended_ = true;
    }

    msg.payload = Buffer( move( s ) );

    uint64_t msg_size = msg.sequence_length();

    cout << "b\n";

    if ( msg_size == 0 ) {
      break;
    }

    sending_msg.emplace_back( msg );
    cout << "1\n";
    outstanding_msg_[ack_send_] = msg;
    cout << "2\n";
    seq_in_flight_ += msg_size;
    cout << "3\n";
    ack_send_ += msg_size;

    cout << "push end\n";

    if ( !timer.timer_status() ) {
      timer.start();
    }

    if ( msg.FIN ) {
      break;
    }
  }
}

TCPSenderMessage TCPSender::send_empty_message() const
{
  // Your code here.
  TCPSenderMessage msg;
  msg.seqno = Wrap32::wrap( ack_send_, isn_ );
  return { msg };
}

void TCPSender::receive( const TCPReceiverMessage& msg )
{
  // Your code here.
  // cout << "receive\n" ;
  if ( msg.ackno.has_value() ) {
    uint64_t abs_ack = msg.ackno.value().unwrap( isn_, ack_send_ );
    if ( abs_ack > ack_send_ )
      return;
    for ( auto iter = outstanding_msg_.begin(); iter != outstanding_msg_.end(); ) {
      auto& [seq, seg] = *iter;
      if ( seq + seg.sequence_length() <= abs_ack ) {
        seq_in_flight_ -= seg.sequence_length();
        if ( !syn_received_ )
          syn_received_ = true;
        iter = outstanding_msg_.erase( iter );
        timer.set_RTO( initial_RTO_ms_ );
        if ( !outstanding_msg_.empty() )
          timer.start();
        else
          timer.stop();
      } else
        break;
    }
    retrans_ = 0;
  }
  window_size_ = msg.window_size;
}

void TCPSender::tick( const size_t ms_since_last_tick )
{
  // Your code here.
  // cout << "tick\n";
  if ( timer.is_over_time( ms_since_last_tick ) && !outstanding_msg_.empty() ) {
    auto& [seq, msg] = *outstanding_msg_.begin();
    sending_msg.emplace_back( msg );
    retrans_++;
    if ( window_size_ > 0 || !syn_received_ ) {
      uint64_t new_RTO = timer.get_RTO() * 2;
      // cout << new_RTO << endl;
      timer.set_RTO( new_RTO );
    }
    timer.start();
  }
}

Timer::Timer( uint64_t RTO_set_, const size_t time_set_ ) : RTO( RTO_set_ ), time_( time_set_ ), status_( false ) {}

bool Timer::is_over_time( uint64_t ms_since_last_tick )
{
  time_ += ms_since_last_tick;
  // cout << time_ << ' ' << RTO << endl;
  return time_ >= RTO;
}