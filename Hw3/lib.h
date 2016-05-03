#ifndef __LIB_H_INCLUDED
#define __LIB_H_INCLUDED
#include <iostream>
#include <string>
#include <vector>

#include "pipe.h"

//#define DEBUG

using std::cerr;
using std::endl;
using std::string;
using std::vector;




void err_abort(const string& msg);
int fd_reopen(int old_fd,int target_fd);
// strip while spaces
string string_strip(const string& org);
// wait for children pid vector and clear them
void wait_for_children(vector<pid_t>& children);
// convert string to argv
char** convert_string_to_argv(const string& arg_list);
// release argv space
void delete_argv(char** argv);
// execute program with arg string
bool exec_cmd(const string& filename,const string& arg_list);
// pipe-related
void exit_unknown_cmd(UnixPipe& pipe);
// template function for debug
template<class T>
int debug(const T& val){
  cerr << ":::DEBUG:::" << val << endl;
}

#endif
