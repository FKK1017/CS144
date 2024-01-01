#include "wrapping_integers.hh"
#include <iostream>

using namespace std;

Wrap32 Wrap32::wrap( uint64_t n, Wrap32 zero_point )
{
  // Your code here.
  uint64_t max32 = UINT32_MAX;
  max32 += 1;
  uint64_t v = n % max32;
  return zero_point.operator+( (uint32_t)v );
}

uint64_t Wrap32::unwrap( Wrap32 zero_point, uint64_t checkpoint ) const
{
  if ( zero_point.operator==( Wrap32( 0 ) ) && checkpoint == 0 )
    return raw_value_;
  uint64_t zero = zero_point.unwrap( Wrap32( 0 ), 0 );
  uint64_t val = raw_value_ - zero;
  uint64_t max32 = UINT32_MAX;
  max32 += 1;
  if ( val > raw_value_ )
    val += max32;
  uint64_t k = checkpoint / max32;
  val += ( k * max32 );
  uint64_t valp = val + max32, valm = val - max32;
  if ( val <= checkpoint ) {
    return checkpoint - val <= valp - checkpoint ? val : valp;
  } else {
    if ( checkpoint == 0 )
      return val;
    else {
      return val - checkpoint < checkpoint - valm ? val : valm;
    }
  }
}
