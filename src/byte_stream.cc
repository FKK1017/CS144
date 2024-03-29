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

  auto len = min(available_capacity(), data.size());
  if (len == 0) {
    return;
  } else if (len < data.size()) {
    data.resize(len);
  }

  bytestream_.push(move(data));
  if (bytestream_.size() == 1) {
    bytestream_view = bytestream_.front();
  }
  bytes_pushed_ += len;
}

void Writer::close()
{
  // Your code here.
  if ( closed_ ) {
    cerr << "Writer already closed but reset.\n";
    return;
  }

  closed_ = true;
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
  return { capacity_ - reader().bytes_buffered() };
}

uint64_t Writer::bytes_pushed() const
{
  // Your code here.
  return { bytes_pushed_ };
}

string_view Reader::peek() const
{
  // Your code here.
  return bytestream_view;
}

bool Reader::is_finished() const
{
  // Your code here.
  return { writer().is_closed() && bytes_buffered()==0};
}

bool Reader::has_error() const
{
  // Your code here.
  return { error_ };
}

void Reader::pop( uint64_t len )
{
  // Your code here.
  // (void)len;
  // for ( uint64_t i = 0; i < len; i++ ) {
  //   bytestream_.pop();
  //   bytes_popped_++;
  // }

  // if ( closed_ && ( bytes_buffered() == 0 ) )
  //   finished_ = true;
  if (len > bytes_buffered()) {
    return;
  }

  bytes_popped_ += len;

  while (len > 0) {
    if (len >= bytestream_view.size()) {
      len -= bytestream_view.size();
      bytestream_.pop();
      bytestream_view = bytestream_.front();
    } else {
      bytestream_view.remove_prefix(len);
      len = 0;
    }
  }
}

uint64_t Reader::bytes_buffered() const
{
  // Your code here.
  return { writer().bytes_pushed() - bytes_popped_ };
}

uint64_t Reader::bytes_popped() const
{
  // Your code here.
  return { bytes_popped_ };
}
