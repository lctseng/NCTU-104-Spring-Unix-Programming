
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <glob.h>

#include <sstream>
#include <cstring>
#include <cstdlib>

#include "lib.h"

using std::stringstream;
using std::strncpy;

void err_abort(const string& msg){
  debug(msg);
  abort();
}

int fd_reopen(int old_fd,int target_fd){
  if(old_fd != target_fd){
    close(old_fd);
    dup2(target_fd,old_fd);
  }
  return old_fd;
}

// strip while spaces
string string_strip(const string& org){
  if(org.empty()){
    return org;
  }
  else{
    string::size_type front = org.find_first_not_of(" \t\n\r");
    if(front==string::npos){
      return "";
    }
    string::size_type end = org.find_last_not_of(" \t\n\r");
    return org.substr(front,end - front + 1);
  }
}

// wait for children pid vector and clear them
void wait_for_children(vector<pid_t>& children){
    for(const pid_t& pid : children){
      waitpid(pid,nullptr,0);
    }
    children.clear();
}


// convert string to argv
char** convert_string_to_argv(const string& arg_list){ 
  stringstream ss(arg_list);
  string arg;
  vector<string> arg_array;
  glob_t globbuf;
  int skip_convert = 1; // skip first one
  while(ss >> arg){
    // arg is a single argument
    if(skip_convert-- > 0){
      // no convert
      arg_array.push_back(arg);
    }
    else{
      // try to expand it via glob
#ifdef DEBUG
      std::cout << "Converting:" << arg << std::endl;
#endif
      glob(arg.c_str(), GLOB_NOCHECK, NULL, &globbuf);
      for(int i=0;i<globbuf.gl_pathc;i++){
#ifdef DEBUG
        std::cout << "Converted arg: " << globbuf.gl_pathv[i] << std::endl;
#endif
        arg_array.push_back(globbuf.gl_pathv[i]);
      }
    }
  }
  char** argv = new char*[arg_array.size()+1];
  for(int i=0;i<arg_array.size();i++){
    string& arg = arg_array[i];
    argv[i] = new char[arg.length()+1];
    strncpy(argv[i],arg.c_str(),arg.length()+1);
  }
  argv[arg_array.size()] = nullptr;
#ifdef DEBUG
  debug("ARGV:");
  char **ptr = argv;
  while(*ptr){
    debug(*ptr);
    ++ptr;
  }
#endif
  return argv;
}
// release argv space
void delete_argv(char** argv){
  char **ptr = argv;
  while(*ptr){
    delete[] *ptr;
    ++ptr;
  }
  delete[] argv;
    
}


// execute program with arg string
bool exec_cmd(const string& filename,const string& arg_list){
  char** argv = convert_string_to_argv(filename + " " + arg_list);
  if(execvp(filename.c_str(),argv)){
    delete_argv(argv);
    return false;
  }
  return true;
}

// pipe-related
void exit_unknown_cmd(UnixPipe& pipe){
  pipe.writeline("error");
  pipe.close_write();
  abort();
}



