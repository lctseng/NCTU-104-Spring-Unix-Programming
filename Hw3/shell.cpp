#include <iostream>
#include <string>
#include <regex>
#include <cstdlib>
#include <vector>

#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <wordexp.h>

#include "lib.h"


using namespace std;


struct CmdEntry{
  string cmd;
  string arg_line;
  string read_filename;
  string write_filename;
  int fileflag;
  CmdEntry(const string& cmd,
          const string& arg_line,
          const string& read_filename, 
          const string& write_filename, 
          int fileflag):
    cmd(cmd),
    arg_line(arg_line),
    read_filename(read_filename),
    write_filename(write_filename),
    fileflag(fileflag){}
};

struct JobEntity{
  pid_t pgid;
  int wait_count;
  JobEntity(pid_t pgid,int wait_count = 1):pgid(pgid),wait_count(wait_count){}
};

extern char **environ;
bool flag_stopped;
deque<JobEntity> stopped_jobs;


typedef void (*Sigfunc)(int);
static Sigfunc old_handler;
// Signal handler for SIGCHLD
void handle_sigchld(int sig) {
  int status;
  while (waitpid((pid_t)(-1), &status, WNOHANG)> 0) {
    // collect one status
    if(WIFSTOPPED(status)){
      // have stopped, resume to foreground
      flag_stopped = true;
    }
  }
}

void register_sigchld(){
  old_handler = signal(SIGCHLD,handle_sigchld);
}
void restore_sigchld(){
  signal(SIGCHLD,old_handler);
}

JobEntity wake_up_one_stopped_pg(){
  if(!stopped_jobs.empty()){
    JobEntity ent = stopped_jobs.front();
    stopped_jobs.pop_front();
    kill(ent.pgid * -1,SIGCONT);
    return ent;
  }
  else{
    cerr << "No current job" << endl;
    return JobEntity(-1);
  }
}

sigset_t block_signals(){
  sigset_t newmask, oldmask;
  sigemptyset(&newmask);
  sigaddset(&newmask,SIGTTOU);
  //sigaddset(&newmask,SIGCHLD);
  sigprocmask(SIG_BLOCK, &newmask, &oldmask);
  return oldmask;
  //signal(SIGTTOU,SIG_IGN);
}

void unblock_signals(sigset_t oldmask){
  sigprocmask(SIG_SETMASK, &oldmask, NULL);
  //signal(SIGTTOU,SIG_DFL);
}


void run_command(CmdEntry& cmd,UnixPipe& last_pipe, UnixPipe& next_pipe){
  // open last_pipe's read_fd for stdin if exists
  if(!last_pipe.is_read_closed()){
    fd_reopen(STDIN_FILENO,last_pipe.read_fd);
  }
  else if(!cmd.read_filename.empty()){ // last_pipe doesn't open, check if read_filename not empty?
    int new_fd = open(cmd.read_filename.c_str(),O_RDONLY);
    if(new_fd >= 0){
      fd_reopen(STDIN_FILENO,new_fd);
    }
    else{
      cerr << "Open read file error: " << cmd.read_filename << endl;
      exit(127);
    }
  }
  // open next_pipe's write_fd for stdout if exists
  if(!next_pipe.is_write_closed()){
    fd_reopen(STDOUT_FILENO,next_pipe.write_fd);
  }
  else if(!cmd.write_filename.empty()){ // next_pipe doesn't open, check if write_filename not empty?
    int new_fd = open(cmd.write_filename.c_str(),cmd.fileflag,0644);
    if(new_fd >= 0){
      fd_reopen(STDOUT_FILENO,new_fd);
    }
    else{
      cerr << "Open write file error: " << cmd.write_filename << endl;
      exit(127);
    }
  }
  last_pipe.close();
  next_pipe.close();
  exec_cmd(cmd.cmd,cmd.arg_line);
}

void run_commands(vector<CmdEntry>& cmds,bool run_as_bg){
  restore_sigchld();
  flag_stopped = false;
  sigset_t oldmask = block_signals();
#ifdef DEBUG
  cerr << "Number of cmd entries: " << cmds.size() << endl;
#endif
  vector<UnixPipe> pipes;
  pid_t pgid = 0;
  // create a fake first pipe
  pipes.emplace_back(false);
  // create n-1 real pipes 
  for(int i=0;i<cmds.size()-1;i++){
    pipes.emplace_back(true);
  }
  // create a fake last pipe
  pipes.emplace_back(false);
  // dispatch commands
  int last_idx;
  int next_idx;
  for(int i=0;i<cmds.size();i++){
    // fork it!
    pid_t pid = fork();
    if(pid > 0) // parent
    {
      // we need to know children's pgid
      if(pgid == 0){
        pgid = pid;
      }
      // set pgid for child
      setpgid(pid,pgid);
#ifdef DEBUG
      cerr << "[Parent] new pid : " << pid  << endl;
#endif
    }
    else // child
    {
      setpgid(0,pgid);
      signal(SIGTSTP,SIG_DFL);
      last_idx = i;
      next_idx = i + 1;
      // close unrelated pipe
#ifdef DEBUG
      cerr << "[Child] closing unrelated pipes" << endl;
#endif
      for(int j=0;j<pipes.size();j++){
        if(j != last_idx && j != next_idx){
          pipes[j].close();
        }
      }
      // run command!
      run_command(cmds[i],pipes[last_idx],pipes[next_idx]);
      // here means fail...
      cerr << "Error executing: " << cmds[i].cmd << endl;
      exit(127);
    }
  }
  // parent
  // close all pipe
#ifdef DEBUG
  cerr << "[Parent] closing all pipes" << endl;
#endif
  for(auto& pipe : pipes){
    pipe.close();
  }
  // if child is a foreground
  if(!run_as_bg){
    // change foreground group to child pg
    tcsetpgrp(STDIN_FILENO,pgid);
    // wait for all children
    int wait_count = cmds.size();
    while(wait_count > 0){
      int status = 0;
      pid_t pid = waitpid(pgid * -1, &status ,WUNTRACED);
      //cout << "pid = " << pid << endl;
      // collect one status
      if(WIFSTOPPED(status)){
        // have stopped, resume to foreground
        stopped_jobs.emplace_back(pgid,wait_count);
        //cout << "pid " << pid << " is stopped" << endl;
        break;
      }
      wait_count--;
    }
    // change foreground to my pgid
    tcsetpgrp(STDIN_FILENO,getpgid(0));
  }
  unblock_signals(oldmask);
  register_sigchld();
}

int main(){
  signal(SIGTSTP,SIG_IGN);
  vector<CmdEntry> cmds;
  string str;
  while(true){
    register_sigchld();
    cout << "shell-prompt$ ";
    if(!getline(cin,str)){
      cout << "EOF encountered, exiting..." << endl;
      break;
    }
    str = string_strip(str);
    smatch match;
    if(regex_search(str, regex("^\\s*exit\\s*$"))){
      break;
    }
    // fg
    else if(regex_search(str, regex("^\\s*fg\\s*$"))){
      JobEntity ent = wake_up_one_stopped_pg();
      pid_t pgid = ent.pgid;
      // change it to foreground
      restore_sigchld();
      sigset_t oldmask = block_signals();
      tcsetpgrp(STDIN_FILENO,pgid);
      int wait_count = ent.wait_count;
      while(wait_count > 0){
        int status = 0;
        pid_t pid = waitpid(pgid * -1, &status ,WUNTRACED);
        //cout << "pid = " << pid << endl;
        // collect one status
        if(WIFSTOPPED(status)){
          // have stopped, resume to foreground
          stopped_jobs.emplace_back(pgid,wait_count);
          //cout << "pid " << pid << " is stopped" << endl;
          break;
        }
        wait_count--;
      }
      // change foreground to my pgid
      tcsetpgrp(STDIN_FILENO,getpgid(0));
      unblock_signals(oldmask);
      register_sigchld();
    }
    // bg
    else if(regex_search(str, regex("^\\s*bg\\s*$"))){
      wake_up_one_stopped_pg();
    }
    // printenv
    else if(regex_search(str,match,regex("^\\s*printenv(\\s*|$)"))){
      string name = string_strip(match.suffix());
      if(!name.empty()){
        char* value = getenv(name.c_str());
        if(value){
          cout << name << '=' << value << endl;
        }
        else{
          cout << "Undefined environment variable: " << name << endl;
        }
      }
      else{ // dump all env
        char** current = environ;
        while(*current){
          cout << *current << endl;
          ++current;
        }
      }
    } 
    // set env
    else if(regex_search(str,match,regex("^\\s*export(\\s+)"))){
      string remain = match.suffix();
      if(regex_search(remain,match,regex("([^ \t\n]*)=([^ \t\n]*)"))){
        string var = match[1];
        string name = match[2];
#ifdef DEBUG
        cerr << "Set env: '" << var << "' to '" << name << "'" << endl;
#endif
        setenv(var.c_str(),name.c_str(),1);
      }
    }
    // remove env
    else if(regex_search(str,match,regex("^\\s*unset\\s+"))){
      string name = string_strip(match.suffix());
      if(!name.empty()){
        unsetenv(name.c_str());
      }
    } // set env
    else{
      cmds.clear();
      bool run_as_bg = false;
      string cut_string = str;
      string next_cut;
      while(regex_search(cut_string,match,regex("(.+?)($|(\\| )|(&))"))){
        next_cut = match.suffix();
        string src_cmd_line = string_strip(match[1]);
        string cmd_line = src_cmd_line;
        string suffix = string_strip(match[2]);

        // File name
        string read_filename;
        string write_filename;
        int fileflag = O_WRONLY | O_CREAT;
        // read
        if(regex_search(cmd_line,match,regex("(<+)\\s*([^!| \t\n]+)"))){
          read_filename = string_strip(match[2]);
          cmd_line = string_strip(match.prefix()) + " " + string(match.suffix());
        }
        // write
        if(regex_search(cmd_line,match,regex("(>+)\\s*([^!| \t\n]+)"))){
          string mode_str = string_strip(match[1]);
          if(mode_str == ">>"){
            fileflag |= O_APPEND;
          }
          else{
            fileflag |= O_TRUNC;
          }
          write_filename = string_strip(match[2]);
          cmd_line = string_strip(match.prefix()) + " " + string(match.suffix());
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
        debug("read filename:"+read_filename);
        debug("write filename:"+write_filename);
        debug("suffix:"+suffix);
        debug("---------");
#endif
        cmds.emplace_back(cmd,arg_line,read_filename,write_filename,fileflag);
        // check need bg?
        if(suffix == "&"){
          run_as_bg = true;
        }
        // prepare for next cut
        cut_string = std::move(next_cut);
      } // end for line-cutting
      // cmd vector is ready here
      if(!cmds.empty()){
        run_commands(cmds,run_as_bg);
      }
    } // end for normal cmd
  } // end for while each line
}
