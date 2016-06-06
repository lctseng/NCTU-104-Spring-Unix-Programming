#include <iostream>
#include <fstream>
#include <cstring>
#include <regex>
#include <map>
#include <algorithm>

#include "unistd.h"
#include "sys/wait.h"
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>


#include "sys/socket.h"
#include "arpa/inet.h"
#include "netinet/in.h"

using namespace std;

#define LINE_BUF_SIZE 2048

FILE* fp_client;
int fd_client;

unsigned short bindPort;

struct {
  sockaddr_in addr;
  string req_line;
  string verb;
  string uri;
  string path; // uri before ?
  string query; // uri after ?
  map<string,string> headerInfo;
  string body;
  
} http_info;

map<string,string> mimeMap;

void initMIME();
int socketInit();
int handleClient();
void handleRequest();
void handleStaticFile(const string&, struct stat&);
void handleStaticNormalFile(const string&, struct stat&, const string&);
void handleStaticExecutableFile(const string&);
void handleDirectory(const string&);
void handleDirectoryWithRedirect(const string&);
void handleDirectoryWithIndex(const string&);
void handleDirectoryWithoutIndex(const string&);
void sendServerError();
int detachFork();
void print_http_info();
void send_common_header(int code);
string string_strip(const string&);
string get_mime_type_by_extension(const string& extension);
string get_extension(const string& filename);
bool is_extension_executable(const string& ext);
bool is_file_executable(const string& ext);
bool exec_cmd(const string& filename,const string& arg_list);

int main(int argc,char** argv){
  int fd_self;
  int val, pid;
  socklen_t socklen;
  if(argc < 3){
    cout << "./webserver <portNum> \"/path/to/your/webserver/docroot\"" << endl;
    exit(-1);
  }
  initMIME();
  bindPort = atoi(argv[1]);
  chdir(argv[2]);
  fd_self = socketInit();
  val = 1;

  while(true){
    socklen = sizeof(http_info.addr);
    bzero(&http_info.addr,sizeof(http_info.addr));
    if((fd_client = accept(fd_self, (sockaddr*)&http_info.addr, &socklen)) < 0 ){
      perror("accept");
      exit(-1);
    }
    pid = detachFork(); // must success, exit internally when error
    if(pid > 0){
      // parent
      close(fd_client);
    }
    else{
      // detached child
      close(fd_self);
      // make fd as FILE*
      fp_client = fdopen(fd_client,"r+");
      setvbuf(fp_client, NULL, _IONBF, 0);
      handleClient();
      exit(0);

    }
  }
}
// fork 2 times
int detachFork(){
  int pid1, pid2;
  if((pid1 = fork()) < 0){
    perror("fork 1");
    exit(-1);
  }
  else if(pid1 == 0){
    // child 1
    if((pid2 = fork())<0){
      perror("fork 2");
      exit(-1);
    }
    else if(pid2 == 0){
      // child 2
      // return as child
      return pid2;
    }
    else{
      // parent 2
      // exit immediately
      exit(0);
    }
  }
  else{
    // parent 1
    // return as parent
    waitpid(pid1,NULL,0);
    return pid1;
  }
}

int socketInit(){
    sockaddr_in addr_self;
    int fd_self;
    int val;

    addr_self.sin_family = PF_INET;
    addr_self.sin_port = htons(bindPort);
    addr_self.sin_addr.s_addr = INADDR_ANY;

    fd_self = socket(AF_INET,SOCK_STREAM,0);
    if(setsockopt(fd_self ,SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val)) < 0){
      perror("se1tsockopt");
      exit(-1);
    }

    if(fd_self < 0){
        cout << "create socket failed." << endl;
        exit(1);
    }

    if(bind(fd_self,(sockaddr*)&addr_self,sizeof(addr_self))<0){
        cout << "bind failed." << endl;
        exit(1);
    }
    if(listen(fd_self,10)<0){
        cout << "listen failed" << endl;
        exit(1);
    }

    return fd_self;
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
void send_common_header(int code){
  switch(code){
    case 200:
      fprintf(fp_client,"HTTP/1.1 200 OK\r\n");
      break;
    case 301:
      fprintf(fp_client,"HTTP/1.1 301 Moved Permanently\r\n");
      break;
    case 403:
      fprintf(fp_client,"HTTP/1.1 403 Forbidden\r\n");
      break;
    case 404:
      fprintf(fp_client,"HTTP/1.1 404 Not found\r\n");
      break;
  }
  fprintf(fp_client,"Connection: close\r\n");
}
// print http info
void print_http_info(){
  fprintf(fp_client,"HTTP/1.1 200 OK\r\n");
  fprintf(fp_client,"Connection: close\r\n");
  fprintf(fp_client,"Content-Type: text/plain\r\n");
  fprintf(fp_client,"\r\n");
  fprintf(fp_client,"Http verb: %s\r\n", http_info.verb.c_str());
  fprintf(fp_client,"Http path: %s\r\n", http_info.path.c_str());
  fprintf(fp_client,"Http query: %s\r\n", http_info.query.c_str());
  fprintf(fp_client,"Other Headers:\r\n");
  for(const auto& pair : http_info.headerInfo){
    fprintf(fp_client,"%s: %s\r\n", pair.first.c_str(), pair.second.c_str());
  }
}
void initMIME(){
  mimeMap.insert(make_pair("txt","text/plain"));
  mimeMap.insert(make_pair("html","text/html"));
  mimeMap.insert(make_pair("htm","text/html"));
  mimeMap.insert(make_pair("htm","text/html"));
  mimeMap.insert(make_pair("gif","image/gif"));
  mimeMap.insert(make_pair("jpg","image/jpeg"));
  mimeMap.insert(make_pair("png","image/png"));
  mimeMap.insert(make_pair("bmp","image/x-ms-bmp"));
  mimeMap.insert(make_pair("doc","application/msword"));
  mimeMap.insert(make_pair("pdf","application/pdf"));
  mimeMap.insert(make_pair("mp4","video/mp4"));
  mimeMap.insert(make_pair("swf","application/x-shockwave-flash"));
  mimeMap.insert(make_pair("swfl","application/x-shockwave-flash"));
  mimeMap.insert(make_pair("ogg","audio/ogg"));
  mimeMap.insert(make_pair("bz2","application/x-bzip2"));
  mimeMap.insert(make_pair("gz","application/x-gzip"));
}
string get_extension(const string& filename){
  smatch match;
  if(regex_search(filename,match,regex("\\.(\\w+)$"))){
    return match[1];
  }
  else{
    // cannot find extension
    return "";
  }
}
string get_mime_type_by_extension(const string& extension){
  string ext = extension;
  transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
  cout << "Finding MIME for extension:" << ext << endl;
  auto it = mimeMap.find(ext);
  if(it != mimeMap.end()){
    cout << "->found mime:" << it->second << endl;
    return it->second;
  }
  else{
    cout << "->not found" << endl;
    return "text/plain";
  }
}
bool is_extension_executable(const string& ext){
  return ext == "sh" || ext == "php"; 
}
bool is_file_executable(const string& filename){
  do{
    // check access
    if(access(filename.c_str(),R_OK | X_OK) != 0){
      break;
    }
    return true;
  }while(false);
  // fallback: false
  return false;
}
int handleClient(){
  char buf[LINE_BUF_SIZE];
  smatch match;
  // get http req
  fgets(buf,LINE_BUF_SIZE,fp_client);
  http_info.req_line = string(buf);
  // parse req line
  if(regex_search(http_info.req_line,match,regex("(\\S+) (\\S+)"))){
    http_info.verb = match[1];
    http_info.uri = match[2];
    if(regex_search(http_info.uri,match,regex("\\?"))){
      http_info.path = match.prefix();
      http_info.query = match.suffix();
    }
    else{
      http_info.path = http_info.uri;
    }
  }
  // get other http header
  while(fgets(buf,LINE_BUF_SIZE,fp_client)){
    string line(buf);
    if(line == "\r\n"){
      break;
    }
    else if(regex_search(line, match, regex("(\\S+): (.*)\r\n"))){
      http_info.headerInfo.insert(make_pair(match[1],match[2]));
    }
  }
  handleRequest();
  /*
  if(http_info.verb == "GET"){
    handleGET();
  }
  else if(http_info.verb == "POST"){
    handlePOST();
  }
  */
  // print debug info
  //print_http_info();
  return 0;
}
void handleStaticNormalFile(const string& file_path, struct stat& statbuf, const string& extension){
  send_common_header(200);
  fprintf(fp_client,"Content-Type: %s\r\n",get_mime_type_by_extension(extension).c_str());
  fprintf(fp_client,"Content-Length: %d\r\n",statbuf.st_size);
  fprintf(fp_client,"\r\n");
  // read bytes and send to client
  char buf[LINE_BUF_SIZE];
  int remain = statbuf.st_size;
  ifstream fin(file_path, ifstream::binary); 
  while(remain > 0){
    int n = fin.readsome(buf,min(remain,LINE_BUF_SIZE));
    fwrite(buf,1,n,fp_client);
    remain -= n;
  }
}
void handleStaticExecutableFile(const string& file_path){
  // clear env
  clearenv();
  // basic http env
  setenv("CONTENT_LENGTH",http_info.headerInfo["Content-Length"].c_str(),1);
  setenv("CONTENT_TYPE",http_info.headerInfo["Content-Type"].c_str(),1);
  setenv("REQUEST_URI",http_info.uri.c_str(),1);
  //setenv("REQUEST_METHOD",http_info.verb.c_str(),1);
  setenv("REQUEST_METHOD","GET",1);
  setenv("SCRIPT_NAME",http_info.path.c_str(),1);
  setenv("QUERY_STRING",http_info.query.c_str(),1);
  setenv("GATEWAY_INTERFACE","CGI/1.1",1);
  setenv("PATH","/bin:/usr/bin:/usr/local/bin",1);
  // address
  char addr_buf[20] = {0};
  inet_ntop(AF_INET,&http_info.addr.sin_addr,addr_buf,sizeof(addr_buf));
  setenv("REMOTE_ADDR",addr_buf,1);
  // port
  char port_buf[6] = {0};
  snprintf(port_buf,5,"%u",ntohs(http_info.addr.sin_port));
  setenv("REMOTE_PORT",port_buf,1);
  // reopen fd
  cout << "Executing script:" << file_path << endl;
  dup2(fd_client,STDOUT_FILENO);
  if(http_info.verb == "POST"){
    //dup2(fd_client,STDIN_FILENO);
    //close(STDIN_FILENO);
  }
  else{
    //close(STDIN_FILENO);
  }
  send_common_header(200);
  //close(fd_client);
  if(!exec_cmd(file_path,"")){
    sendServerError();
    fprintf(fp_client,"Cannot exec CGI: %s\r\n",file_path.c_str());
  }
}
void handleStaticFile(const string& file_path, struct stat& statbuf){
  // get extension
  string extension = get_extension(file_path);  
  if(is_extension_executable(extension) && is_file_executable(file_path) ){
    //a valid cgi
    handleStaticExecutableFile(file_path);
  }
  else{
    handleStaticNormalFile(file_path, statbuf, extension);
  }
}
void handleDirectoryWithRedirect(const string& file_path){
  // check has '/' at the end?
  smatch match;
  if(regex_search(file_path,match,regex("/$"))){
    handleDirectory(file_path);
  }
  else{
    // no slash
    cout << "Redirect from :" << file_path << endl;
    send_common_header(301);
    fprintf(fp_client,"Location: http://%s%s/\r\n",http_info.headerInfo["Host"].c_str(),http_info.uri.c_str());
    fprintf(fp_client,"\r\n");
  }
}
void handleDirectory(const string& file_path){
  cout << "Open directory for :" << file_path << endl;
  // scan for index.html
  DIR* dir = opendir(file_path.c_str());
  struct dirent entry;
  struct dirent* result;
  bool hasIndex = false;
  while(true){
    if( readdir_r(dir, &entry, &result ) == 0 ){
      if(result == nullptr){
        break;
      }
      if(strcmp(entry.d_name,"index.html")==0 ){
        hasIndex = true;
        break;
      }
    }
    else{
      perror("readdir_r");
      break;
    }
  }
  if(hasIndex){
    handleDirectoryWithIndex(file_path);
  }
  else{
    handleDirectoryWithoutIndex(file_path);
  }
}
void handleDirectoryWithIndex(const string& dir_path){
  string file_path = dir_path + "index.html";
  struct stat stat_result;
  if(stat(file_path.c_str(),&stat_result) == 0){
    // check index.html is accessible 
    if(access(file_path.c_str(),R_OK) == 0){
      handleStaticFile(file_path,stat_result);
    }
    else{
      send_common_header(403);
      fprintf(fp_client,"Content-Type: text/plain\r\n");
      fprintf(fp_client,"\r\n");
      fprintf(fp_client,"default index not readable for %s\r\n",file_path.c_str());
    }
  }
  else{
    send_common_header(500);
    fprintf(fp_client,"Content-Type: text/plain\r\n");
    fprintf(fp_client,"\r\n");
    fprintf(fp_client,"Error code = %d\r\n",errno);
    perror("stat");
  }
  
}
void handleDirectoryWithoutIndex(const string& file_path){
  send_common_header(200);
  fprintf(fp_client,"Content-Type: text/html\r\n");
  fprintf(fp_client,"\r\n");
  DIR* dir = opendir(file_path.c_str());
  struct dirent entry;
  struct dirent* result;
  fprintf(fp_client,"<h1>Directroy list for %s</h1>\r\n",http_info.path.c_str());
  while(true){
    if( readdir_r(dir, &entry, &result ) == 0 ){
      if(result == nullptr){
        break;
      }
      fprintf(fp_client,"<li><a href=%s%s>%s</a></li><br />\r\n",http_info.path.c_str(),entry.d_name,entry.d_name);
    }
    else{
      perror("readdir_r");
      break;
    }
  }
  closedir(dir);
  
}
void handleRequest(){
  // check if we can access that file
  string file_path = string(".") + http_info.path;
  if(access(file_path.c_str(),R_OK) == 0){
    // check directory or not
    struct stat stat_result;
    if(stat(file_path.c_str(),&stat_result) == 0){
      if (S_ISDIR(stat_result.st_mode)){
        handleDirectoryWithRedirect(file_path);
      }
      else{
        handleStaticFile(file_path,stat_result);
      }
    }
    else{
      send_common_header(500);
      fprintf(fp_client,"Content-Type: text/plain\r\n");
      fprintf(fp_client,"\r\n");
      fprintf(fp_client,"Error code = %d\r\n",errno);
      perror("stat");
    }
  }
  else{
    if(errno == EACCES ){
      // permission
      // if it's a directroy, use normal error code
      struct stat stat_result;
      if(stat(file_path.c_str(),&stat_result) == 0){
        if (S_ISDIR(stat_result.st_mode)){
          // denied directory, use normal 403
          send_common_header(403);
        }
        else{
          // denied static file, use 404
          send_common_header(404);
        }
      }
      else{
        // stat error, use 404 by default
        send_common_header(404);
      }
      fprintf(fp_client,"Content-Type: text/plain\r\n");
      fprintf(fp_client,"\r\n");
      fprintf(fp_client,"Access denied for %s\r\n",file_path.c_str());
    }
    else if(errno == ENOTDIR || errno == ENOENT){
      // not found
      send_common_header(403);
      fprintf(fp_client,"Content-Type: text/plain\r\n");
      fprintf(fp_client,"\r\n");
      fprintf(fp_client,"Not found for %s\r\n",file_path.c_str());
    }
    else{
      // other error
      sendServerError();
      perror("stat");
    }
  
  }
  
}
void sendServerError(){
  send_common_header(500);
  fprintf(fp_client,"Content-Type: text/plain\r\n");
  fprintf(fp_client,"\r\n");
  fprintf(fp_client,"Internal Server Error, Error code = %d\r\n",errno);
}
// convert string to argv
char** convert_string_to_argv(const string& arg_list){ 
  stringstream ss(arg_list);
  string arg;
  vector<string> arg_array;
  while(ss >> arg){
    arg_array.push_back(arg);
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

