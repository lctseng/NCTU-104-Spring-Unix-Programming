#ifndef __PIPE_H_INCLUDED
#define __PIPE_H_INCLUDED
#include <string>

#include <unistd.h>

#include "fdstream.h"

//#define DEBUG

using std::string;

class UnixPipe{
public:
  UnixPipe(bool real = true);
  int write(const string& str)const;
  int writeline(const string& str)const;
  string getline()const;
  bool close_read();
  bool close_write();
  bool is_alive() const;
  bool is_write_closed()const;
  bool is_read_closed()const;
  void close();

  // member variables
  int read_fd;
  int write_fd;

};

#endif
