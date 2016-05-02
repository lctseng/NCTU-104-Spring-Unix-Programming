#include <iostream>
#include <string>
#include <regex>
#include <cstdlib>

#include "lib.h"
using std::getline;
using std::string;
using std::cout;
using std::cin;
using std::regex;
using std::regex_match;
using std::regex_search;
using std::smatch;
using std::getenv;



int main(){
  /*
  string str;
  while(true){
    cout << "% ";
    if(!getline(cin,str)){
#ifdef DEBUG
      debug("stream closed");
#endif
      break;
    }
    str = string_strip(str);
    smatch match;
    if(regex_search(str, regex("^\\s*exit"))){
      break;
    }
    else if(regex_search(str, regex("/"))){
      cout << "/ is forbidden here!" << endl;
    }
    else if(regex_search(str,match,regex("^\\s*printenv(\\s*|$)"))){
      string name = string_strip(match.suffix());
      if(!name.empty()){
        cout << name << '=' << getenv(name.c_str()) << endl;
      }
      decrease_pipe_counter_and_close();
    }
    else if(regex_search(str,match,regex("^\\s*setenv(\\s*|$)"))){
      string remain = match.suffix();
      if(regex_search(remain,match,regex("([^ \t\n]*)\\s+([^ \t\n]*)"))){
        string var = match[1];
        string name = match[2];
        setenv(var.c_str(),name.c_str(),1);
      }
      decrease_pipe_counter_and_close();
    }
    else{
      UnixPipe next_pipe(false); 
      UnixPipe last_pipe(false); 
      bool first = true;
      bool last = false;
      UnixPipe stdout_pipe(false);
      int stdout_pipe_count = -1;
      UnixPipe stderr_pipe(false);
      int stderr_pipe_count = -1;
      string cut_string = str;
      string next_cut;
      while(regex_search(cut_string,match,regex("(.+?)($|(\\| )|(([|!]\\d+\\s*)+))"))){
        next_cut = match.suffix();
        string src_cmd_line = string_strip(match[1]);
        string cmd_line;
        string suffix = string_strip(match[2]);
        UnixPipe error_pipe;
       
        // pipe pool
        if(first){
          if(!pipe_pool.empty()){
            pipe_pool.pop_front();
            if(!pipe_pool.empty()){
              pipe_pool[0].close_write();
            }
          }
        }

        // File name
        string filename;
        const char* filemode;
        if(regex_search(src_cmd_line,match,regex("(>+)\\s*([^!| \t\n]+)"))){
          string mode_str = string_strip(match[1]);
          if(mode_str == ">>"){
            filemode = "a";
          }
          else{
            filemode = "w";
          }
          filename = string_strip(match[2]);
          cmd_line = string_strip(match.prefix()) + " " + string(match.suffix());
        }
        else{
          cmd_line = src_cmd_line;
        }
        // arguments
        string::size_type space_pos = cmd_line.find_first_of(" \t");
        string cmd;
        string arg_line;
        if(space_pos != string::npos){
          cmd = cmd_line.substr(0,space_pos);
          arg_line = string(cmd_line.substr(space_pos));   
        }
        else{
          cmd = cmd_line; 
        }
#ifdef DEBUG
        debug("cmd:"+cmd);
        debug("arg:"+arg_line);
        debug("filename:"+filename);
        debug("filemode:"+string(filemode));
        debug("suffix:"+suffix);
#endif
        if(suffix=="|"){
          last = false;
          next_pipe = UnixPipe();
        }
        else{
          last = true; 
          if(suffix.size() >= 2){
            // create numbered-pipe
            // stdout
            if(regex_search(suffix,match,regex("\\|(\\d+)"))){
              stdout_pipe_count = stoi(match[1]);
              stdout_pipe = create_numbered_pipe(stdout_pipe_count);
            }
            // stderr
            if(regex_search(suffix,match,regex("!(\\d+)"))){
              stderr_pipe_count = stoi(match[1]);
              stderr_pipe = create_numbered_pipe(stderr_pipe_count);
            }
            
          }
        }
        // fork
        pid_t pid;
        if((pid = fork())>0){
          // parent
          // close last_pipe and change it to next_pipe
          last_pipe.close();
          if(!last){
            last_pipe = next_pipe;
          }
          // read from error_pipe, if 0 readed. child is successfully exited
          error_pipe.close_write();
          string result = error_pipe.getline();
          if(!result.empty()){
            // error
            printf("Unknown command: [%s].\n",src_cmd_line.c_str());
            // restore pipe
            if(first){
              pipe_pool.push_front(UnixPipe(false));
            }
            // close any numbered-pipe if exist
            stdout_pipe.close(); 
            stdout_pipe.close();
            break;
          }
          else{
            // success
            if(first){
              first = false; 
              if(!pipe_pool.empty()){
                pipe_pool[0].close();
              }
            }
            record_pipe_info(stdout_pipe_count,stdout_pipe);
            record_pipe_info(stderr_pipe_count,stderr_pipe);
          }
          // close error pipe
          error_pipe.close_read();
          // wait for child
          waitpid(pid,nullptr,0);
        }else{
          // child
          // close last pipe writing
          last_pipe.close_write();
          // close error pipe read
          error_pipe.close_read();
          // stdout redirect
          if(!filename.empty()){
            // file
            FILE* f = fopen(filename.c_str(), filemode);
            fd_reopen(FD_STDOUT,fileno(f));
          }
          else{
            if(last){
              if(!stdout_pipe.is_write_closed()){
                // open with numbered-pipe 
                fd_reopen(FD_STDOUT,stdout_pipe.write_fd);
              }
            }
            else{
              // open with next pipe 
              fd_reopen(FD_STDOUT,next_pipe.write_fd);
            }
          }
          // stderr redirect: numbered-pipe
          if(!stderr_pipe.is_write_closed()){
            // open with numbered-pipe 
            fd_reopen(FD_STDERR,stderr_pipe.write_fd);
          }
          // first process open numbered pipe
          if(first){
            // the first process, try to open numbered pipe
            UnixPipe& n_pipe = pipe_pool[0];
            if(!n_pipe.is_read_closed()){
              n_pipe.close_write();
              fd_reopen(FD_STDIN,n_pipe.read_fd); 
            }
            // close other high-count pipe
            close_pipe_larger_than(1);
          }
          else{
            // non-first, open last pipe
            fd_reopen(FD_STDIN,last_pipe.read_fd); 
            // close all numbered-pipe
            close_pipe_larger_than(0);
          }
          // Exec
          exec_cmd(cmd,arg_line);
          // if you are here, means error occurs
          exit_unknown_cmd(error_pipe);
        }
        // prepare for next cut
        cut_string = std::move(next_cut);
      } // end for line-cutting
    } // end for normal cmd
  } // end for while 
  */
}
