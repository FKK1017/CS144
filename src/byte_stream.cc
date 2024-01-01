#include <stdexcept>

#include "byte_stream.hh"
#include <iostream>

using namespace std;

ByteStream::ByteStream( uint64_t capacity ) : capacity_( capacity ), bytestream_() {}

void Writer::push( string data )
{
  // Your code here.
  (void)data;

  if ( data.empty() )
    return;

  if ( closed_ ) {
    cerr << "Writer already closed but still push.\n";
    set_error();
    return;
  }

  if ( error_ ) {
    cerr << "ByteStream has an error but still push.\n";
    return;
  }

  uint64_t lasted_ = capacity_ - ( bytes_pushed_ - bytes_popped_ );

  if ( lasted_ >= data.size() ) {
    for ( auto x : data ) {
      bytestream_.push( std::string( 1, x ) );
      bytes_pushed_++;
    }
  } else {
    for ( uint64_t i = 0; i < lasted_; i++ ) {
      bytestream_.push( std::string( 1, data[i] ) );
      // write_pos_ ++ ;
      bytes_pushed_++;
    }
  }
}

void Writer::close()
{
  // Your code here.
  if ( closed_ ) {
    cerr << "Writer already closed but reset.\n";
    return;
  }

  closed_ = true;
  if ( bytes_pushed_ == bytes_popped_ )
    finished_ = true;
}

void Writer::set_error()
{
  // Your code here.
  error_ = true;
}

bool Writer::is_closed() const
{
  // Your code here.
  return { closed_ };
}

uint64_t Writer::available_capacity() const
{
  // Your code here.
  return { capacity_ - ( bytes_pushed_ - bytes_popped_ ) };
}

uint64_t Writer::bytes_pushed() const
{
  // Your code here.
  return { bytes_pushed_ };
}

string_view Reader::peek() const
{
  // Your code here.
  if ( bytestream_.empty() ) {
    return {};
  } else {
    return bytestream_.front();
  }
}

bool Reader::is_finished() const
{
  // Your code here.
  return { finished_ };
}

bool Reader::has_error() const
{
  // Your code here.
  return { error_ };
}

void Reader::pop( uint64_t len )
{
  // Your code here.
  (void)len;
  for ( uint64_t i = 0; i < len; i++ ) {
    bytestream_.pop();
    bytes_popped_++;
  }

  if ( closed_ && ( bytes_buffered() == 0 ) )
    finished_ = true;
}

uint64_t Reader::bytes_buffered() const
{
  // Your code here.
  return { bytes_pushed_ - bytes_popped_ };
}

uint64_t Reader::bytes_popped() const
{
  // Your code here.
  return { bytes_popped_ };
}
