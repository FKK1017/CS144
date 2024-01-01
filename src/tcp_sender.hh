#pragma once

#include "byte_stream.hh"
#include "tcp_receiver_message.hh"
#include "tcp_sender_message.hh"

#include <deque>
#include <map>

class Timer
{
  uint64_t RTO;
  uint64_t time_;
  bool status_;

public:
  Timer( uint64_t RTO_set_, const size_t time_set_ );

  void set_RTO( uint64_t new_RTO ) { RTO = new_RTO; }
  uint64_t get_RTO() { return RTO; }

  void start()
  {
    time_ = 0;
    status_ = true;
  }
  void stop() { status_ = false; }

  bool timer_status() { return status_; }

  uint64_t get_time() { return time_; }

  bool is_over_time( uint64_t ms_since_last_tick );
};

class TCPSender
{
  Wrap32 isn_;
  uint64_t initial_RTO_ms_;

  std::deque<TCPSenderMessage> sending_msg;              // to send
  std::map<uint64_t, TCPSenderMessage> outstanding_msg_; // to confirm ack and retransmit
  // std::deque<TCPSenderMessage> not_ack_msg;
  //  last ack have reveived
  uint64_t ack_receive_;
  // next ack should send
  uint64_t ack_send_;

  uint64_t retrans_;
  uint64_t seq_in_flight_;

  uint16_t window_size_;

  bool syn_sended_;
  bool syn_received_;
  bool fin_sended_;

  Timer timer;

  // void make_msg(Wrap32 seq, bool SYN, Buffer payload, bool FIN);

public:
  /* Construct TCP sender with given default Retransmission Timeout and possible ISN */
  TCPSender( uint64_t initial_RTO_ms, std::optional<Wrap32> fixed_isn );

  /* Push bytes from the outbound stream */
  void push( Reader& outbound_stream );

  /* Send a TCPSenderMessage if needed (or empty optional otherwise) */
  std::optional<TCPSenderMessage> maybe_send();

  /* Generate an empty TCPSenderMessage */
  TCPSenderMessage send_empty_message() const;

  /* Receive an act on a TCPReceiverMessage from the peer's receiver */
  void receive( const TCPReceiverMessage& msg );

  /* Time has passed by the given # of milliseconds since the last time the tick() method was called. */
  void tick( const size_t ms_since_last_tick );

  /* Accessors for use in testing */
  uint64_t sequence_numbers_in_flight() const;  // How many sequence numbers are outstanding?
  uint64_t consecutive_retransmissions() const; // How many consecutive *re*transmissions have happened?
};
