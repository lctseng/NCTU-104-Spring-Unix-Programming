#ifndef __CONFIGURE_HPP_INCLUDED__
#define __CONFIGURE_HPP_INCLUDED__

#include <cstring>
#include <regex>
#include <fstream>
using namespace std;

#define COLOR_RED  "\033[31m"
#define COLOR_YELLOW  "\033[33m"
#define COLOR_CYAN  "\033[36m"
#define COLOR_RESET "\033[0m"

#define DEFAULT_HIJACK_LOG "hijack.log"
#define CONF_FILENAME "hijack.conf"

#define CONFIG_STRING_LENGTH 255

struct config_set {
  char logname[CONFIG_STRING_LENGTH];
  ssize_t max_read_dump_size; 
  ssize_t max_write_dump_size; 
  
} config;

enum conf_type{
  CONF_INT,
  CONF_STRING,
} ;
struct config_entry {
  string name;
  conf_type type;
  void *data;       
};

static config_entry config_data[] =
  {
    { "logname", CONF_STRING, config.logname },
    { "max_read_dump_size", CONF_INT, &config.max_read_dump_size },
    { "max_write_dump_size", CONF_INT, &config.max_write_dump_size }
  };

void set_default_config(){
  config.max_read_dump_size = 50;
  config.max_write_dump_size = 50;
  strncpy(config.logname,DEFAULT_HIJACK_LOG,min((size_t)CONFIG_STRING_LENGTH,strlen(DEFAULT_HIJACK_LOG)));
}

void parse_config(){
  ifstream fin(CONF_FILENAME);
  string str;
  smatch match;
  cout << COLOR_RED;
  while(getline(fin,str)){
    if(regex_search(str, regex("^\\s+#"))){ // comment line
      continue;
    }
    else if(regex_search(str, match, regex("(.+)=(.*)"))){
      string name = match[1]; 
      string value = match[2]; 
      // find from config_data
      for( auto& entry : config_data){
        if(name == entry.name){
          cout << "[CONFIG] '" << name << "' will be set to '" << value << "'" <<endl;
          // convert data
          switch(entry.type){
            case CONF_STRING:
              strncpy((char*)entry.data,value.c_str(),min((size_t)CONFIG_STRING_LENGTH,value.length()+1));
              break;
            case CONF_INT:
              *((int*)entry.data) = stoi(value);
              break;
          }
        } 
      }
    }
  }
  cout << COLOR_RESET;
  cout.flush();

}


void setup_config(){
  set_default_config(); 
  parse_config();
}

#endif



