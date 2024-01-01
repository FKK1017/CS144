#include "reassembler.hh"

#include <algorithm>
#include <iostream>

using namespace std;

Reassembler::Reassembler() : _reassembler(), _is_data() {}

void Reassembler::insert( uint64_t first_index, string data, bool is_last_substring, Writer& output )
{
  auto _ava_cap = output.available_capacity();

  if ( _reassembler.size() == 0 ) {
    _reassembler.resize( _ava_cap );
    _is_data.resize( _ava_cap );
    _is_data.reserve( _ava_cap );
  }
  if ( _reassembler.size() < first_index + data.size() ) {
    // cout << 2 << endl;
    _reassembler.resize( first_index + data.size() );
    // cout << _is_data.size() << ' ' << first_index + data.size() << endl;
    _is_data.resize( first_index + data.size() + 1 );
    //_is_data.reserve( first_index + data.size() );
  }

  for ( uint64_t i = 0; i < data.size(); i++ ) {
    if ( i + first_index >= _sended_index + _ava_cap ) {
      break;
    } else {
      _reassembler[i + first_index] = data[i];
      if ( !_is_data[i + first_index] ) {
        _is_data[i + first_index] = true;
        _bytes_pending++;
      }
    }
  }

  std::string _sender;

  if ( is_last_substring ) {
    bool _f = true;
    for ( uint64_t i = first_index; i < data.size(); i++ ) {
      if ( _reassembler[i] != data[i - first_index] ) {
        _f = false;
        break;
      }
    }
    _last_got = _f;
  }

  while ( ( _is_data[_sended_index] ) && ( _sended_index < _reassembler.size() ) ) {
    _sender += _reassembler[_sended_index];
    _sended_index++;
    _bytes_pending--;
  }

  output.push( _sender );

  if ( _last_got && ( _bytes_pending == 0 ) )
    output.close();
}

uint64_t Reassembler::bytes_pending() const
{
  // Your code here.
  return { _bytes_pending };
}
