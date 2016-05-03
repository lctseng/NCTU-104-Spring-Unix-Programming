#include <iostream>
#include "pipe.h"

using std::cout;
using std::getline;
using std::endl;

UnixPipe::UnixPipe(bool real){
  if(!real){
    read_fd = -1;
    write_fd = -1;
  }
  else{
    int fd[2];
    pipe(fd);
    read_fd = fd[0];
    write_fd = fd[1];
#ifdef DEBUG
    cout << "Creating New Pipe, read = " << read_fd << ", write = " << write_fd << endl;
#endif
  }
} 
inline bool UnixPipe::is_write_closed()const{
  return write_fd < 0;
}
inline bool UnixPipe::is_read_closed()const{
  return read_fd < 0;
}
inline bool UnixPipe::is_alive() const{
  return !is_write_closed() || !is_read_closed();
}
int UnixPipe::write(const string& str)const{
  if(!is_write_closed()){
    return ::write(write_fd,str.c_str(),str.length()+1);
  }
  else{
    throw "Write end is closed!";
    return 0;
  }
}
int UnixPipe::writeline(const string& str)const{
  return write(str+"\r\n");
}
string UnixPipe::getline()const{
  if(!is_read_closed()){
    string str;
    ifdstream is(read_fd);
    ::getline(is,str);
    return str;
  }
  else{
    throw "Read end is closed!";
    return "";
  }
}
bool UnixPipe::close_read(){
  if(!is_read_closed()){
    ::close(read_fd);
    read_fd = -1;
    return true;
  }
  return false;
}
bool UnixPipe::close_write(){
  if(!is_write_closed()){
    ::close(write_fd);
    write_fd = -1;
    return true;
  }
  return false;
}
void UnixPipe::close(){
#ifdef DEBUG
    cout << "Closing Pipe, read = " << read_fd << ", write = " << write_fd << endl;
#endif
  close_read();
  close_write();
}

