#include "fdstream.h"

ifdstream::ifdstream(int fd):
  buf(fd,std::ios::in),
  basic_istream<char>(&buf)
{
}
ofdstream::ofdstream(int fd):
  buf(fd,std::ios::out),
  basic_ostream<char>(&buf)
{
}
