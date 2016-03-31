#include <iostream>
#include <cstdarg>
#include <cstdio>
#include <cstring>

#include <dlfcn.h>
#include <sys/stat.h>
#include <unistd.h>
#include <iconv.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <openssl/ssl.h>

#include "configure.hpp"


using namespace std;


// generic file and read/write
typedef int(*open_func_t)(const char *,int);
typedef int (*chmod_func_t)(const char *, mode_t);
typedef int (*unlink_func_t)(const char *);
typedef int (*mkdir_func_t)(const char *, mode_t);
typedef FILE *(*fopen_func_t)(const char *, const char *);
typedef iconv_t (*iconv_open_func_t)(const char *, const char *);
typedef FILE *(*fdopen_func_t)(int fd, const char *mode);
typedef FILE *(*freopen_func_t)(const char *path, const char *mode, FILE *stream);
typedef int (*mkostemp_func_t)(char *_template, int flags);
typedef ssize_t (*read_func_t)(int fildes, void *buf, size_t nbyte);
typedef ssize_t (*write_func_t)(int fildes, const void *buf, size_t nbyte);
typedef size_t (*fwrite_func_t)(const void *ptr, size_t size, size_t nmemb, FILE *stream);
typedef size_t (*fread_func_t)(void *ptr, size_t size, size_t nmemb, FILE *stream);
typedef ssize_t (*getline_func_t)(char **lineptr, size_t *n, FILE *stream);
typedef ssize_t (*__getdelim_func_t)(char **lineptr, size_t *n, int delim, FILE *stream);
typedef int (*fgetc_func_t)(FILE *stream);
typedef int (*_IO_getc_func_t)(FILE *stream);
typedef int (*fputs_func_t)(const char *s, FILE *stream);
typedef int (*rename_func_t)(const char *old_name, const char *new_name);
typedef int (*ftruncate_func_t)(int fildes, off_t length);
typedef int (*utime_func_t)(const char *path, const struct utimbuf *times);
typedef int (*fputc_func_t)(int c, FILE *stream);
typedef int (*_IO_putc_func_t)(int c, FILE *stream);

// socket related
typedef int (*connect_func_t)(int socket, const struct sockaddr *address,socklen_t address_len);
typedef int (*SSL_read_func_t)(SSL *ssl, void *buf, int num);
typedef int (*SSL_write_func_t)(SSL *ssl, const void *buf, int num);

// function pointers
static open_func_t old_open;
static chmod_func_t old_chmod;
static unlink_func_t old_unlink;
static mkdir_func_t old_mkdir;
static fopen_func_t old_fopen;
static iconv_open_func_t old_iconv_open;
static fdopen_func_t old_fdopen;
static freopen_func_t old_freopen;
static mkostemp_func_t old_mkostemp;
static read_func_t old_read;
static write_func_t old_write;
static fwrite_func_t old_fwrite;
static fread_func_t old_fread;
static getline_func_t old_getline;
static __getdelim_func_t old___getdelim;
static fgetc_func_t old_fgetc;
static _IO_getc_func_t old__IO_getc;
static fputs_func_t old_fputs;
static rename_func_t old_rename;
static ftruncate_func_t old_ftruncate;
static utime_func_t old_utime;
static fputc_func_t old_fputc;
static _IO_putc_func_t old__IO_putc;

static connect_func_t old_connect;
static SSL_read_func_t old_SSL_read;
static SSL_write_func_t old_SSL_write;

// hijack: others
static bool hijack_initialized = false;
static FILE* fp_log;
static bool in_hijack;
static bool already_msg = false;


// time
static char time_buffer[26];

// a template function that expand all functions
template<typename Return_t = int,Return_t ret_value = -1,typename Function_t, typename... Args>
Return_t try_function(Function_t ptr,Args... args){
  in_hijack = false;
  if(ptr){
    return ptr(args...);
  } 
  else{
    return Return_t(ret_value);
  }
}


char* time_string(){
  time_t timer;
  struct tm* tm_info;
  time(&timer);
  tm_info = localtime(&timer);
  strftime(time_buffer, 26, "%Y:%m:%d %H:%M:%S", tm_info);
  return time_buffer;
}

void vlog(const char* prefix,const char* format,va_list ap){
  if(fp_log){
    fprintf(fp_log,"[%s](%s) ",prefix,time_string());
    vfprintf(fp_log, format, ap );
  }
}
void log(const char* prefix,const char* format, ... ){
  if(fp_log){
    fprintf(fp_log,"[%s](%s) ",prefix,time_string());
    va_list args;
    va_start( args, format );
    vfprintf(fp_log, format, args);
    va_end( args );
  }
}

// wrapping printf
int msg( const char* format, ... ) {
  if(!already_msg){
    already_msg = true;
    va_list args;
    int ret;
    printf(COLOR_YELLOW "[JACK] ");
    // to screen
    va_start( args, format );
    ret = vprintf( format, args );
    va_end( args );
    // to log
    va_start( args, format );
    vlog("JACK",format,args);
    va_end( args );
    printf(COLOR_RESET);
    fflush(stdout);
    already_msg = false;
    return ret;
  }
  else{
    //try_function<ssize_t,-1>(old_write,1,"Already msg",strlen("Already msg"));
    // No mesg here, prevent from recursive
    return -1;
  }
}

void *dlsym_log(void *handle, const char *symbol){
  void* ret = dlsym(handle,symbol);
  if(ret){
    log("INFO","Function loaded: %s\n",symbol);
  }
  else{
    log("WARN","Unable to lod function: %s\n",symbol);
  }
  return ret;
}

// initailize hijack
void hijack_initialize(){
  if(!hijack_initialized){
    // data init
    hijack_initialized = true;
    // load function pointer
    void* handle;
    
    // load open/write from libc
    handle = dlopen("libc.so.6", RTLD_LAZY);
    if(handle){
      // open
      old_open = (open_func_t)dlsym_log(handle, "open");
      // fopen
      old_fopen = (fopen_func_t)dlsym_log(handle, "fopen");
      // write
      old_write = (write_func_t)dlsym_log(handle, "write");
      // fwrite
      old_fwrite = (fwrite_func_t)dlsym_log(handle, "fwrite");
      // read
      old_read = (read_func_t)dlsym_log(handle, "read");
      // fread
      old_fread = (fread_func_t)dlsym_log(handle, "fread");
    }
    // config init
    already_msg = true; // prevent extra output from configuration initialization
    setup_config();
    already_msg = false;
    // prepare log file
    fp_log = fopen(config.logname,"w");
    log("INFO","Hijack initialized\n");
    // load others from libc
    if(handle){
      // chmod
      old_chmod = (chmod_func_t)dlsym_log(handle, "chmod");
      // unlink
      old_unlink = (unlink_func_t)dlsym_log(handle, "unlink");
      // mkdir
      old_mkdir = (mkdir_func_t)dlsym_log(handle, "mkdir");
      // iconv_open
      old_iconv_open = (iconv_open_func_t)dlsym_log(handle, "iconv_open");
      // fdopen
      old_fdopen = (fdopen_func_t)dlsym_log(handle, "fdopen");
      // freopen
      old_freopen = (freopen_func_t)dlsym_log(handle, "freopen");
      // getline
      old_getline = (getline_func_t)dlsym_log(handle, "getline");
      // __getdelim
      old___getdelim = (__getdelim_func_t)dlsym_log(handle, "__getdelim");
      // fgetc
      old_fgetc = (fgetc_func_t)dlsym_log(handle, "fgetc");
      // _IO_getc
      old__IO_getc = (_IO_getc_func_t)dlsym_log(handle, "_IO_getc");
      // fputs
      old_fputs = (fputs_func_t)dlsym_log(handle, "fputs");
      // rename
      old_rename = (rename_func_t)dlsym_log(handle, "rename");
      // ftruncate
      old_ftruncate = (ftruncate_func_t)dlsym_log(handle, "ftruncate");
      // utime
      old_utime = (utime_func_t)dlsym_log(handle, "utime");
      // connect
      old_connect = (connect_func_t)dlsym_log(handle, "connect");
      // fputc
      old_fputc = (fputc_func_t)dlsym_log(handle, "fputc"); 
      // _IO_putc
      old__IO_putc = (_IO_putc_func_t)dlsym_log(handle, "_IO_putc"); 
    }
    // load from libssl
    handle = dlopen("libssl.so.1.0.0", RTLD_LAZY);
    if(handle){
      // SSL_read
      old_SSL_read = (SSL_read_func_t)dlsym_log(handle, "SSL_read");
      // SSL_write
      old_SSL_write = (SSL_write_func_t)dlsym_log(handle, "SSL_write");
    }
  }
  in_hijack = true;
}

extern "C" {
  // open
  int open(const char *path, int oflag){
    hijack_initialize();
    msg("[open] Open file: %s, oflag: %d\n",path,oflag);
    return try_function(old_open,path,oflag);
  }
  // chmod
  int chmod(const char *path, mode_t mode){
    hijack_initialize();
    msg("[chmod] Change file: %s, mode: %o\n",path,mode);
    return try_function(old_chmod,path,mode);
  }
  // unlink
  int unlink(const char *path){
    hijack_initialize();
    msg("[unlink] Unlink file: %s\n",path);
    return try_function(old_unlink,path);
  }
  // mkdir
  int mkdir(const char *path, mode_t mode){
    hijack_initialize();
    msg("[mkdir] Create directory: %s, mode: %o\n",path,mode);
    return try_function(old_mkdir,path,mode);
  }
   
  // fopen
  FILE *fopen(const char *path, const char *mode){
    hijack_initialize();
    msg("[fopen] Open file: %s, mode: %s\n",path,mode);
    return try_function<FILE*,nullptr>(old_fopen,path,mode);
  }
  

  // iconv_open
  iconv_t iconv_open(const char *tocode, const char *fromcode){
    hijack_initialize();
    msg("[iconv_open] character set conversion: from %s to %s\n",fromcode,tocode);
    return try_function<iconv_t, iconv_t(0)>(old_iconv_open,tocode,fromcode);
  }
  
  // fdopen
  FILE *fdopen(int fd, const char *mode){
    hijack_initialize();
    msg("[fdopen] Open file descriptor: %d, mode: %s\n",fd,mode);
    return try_function<FILE*,nullptr>(old_fdopen,fd,mode);
  }
  // freopen
  FILE *freopen(const char *path, const char *mode, FILE *stream){
    hijack_initialize();
    msg("[freopen] Re-open file: %s, mode: %s, from FILE stream: %p\n",path,mode,stream);
    return try_function<FILE*,nullptr>(old_freopen,path,mode,stream);
  }
  // mkostemp
  int mkostemp(char *_template, int flags){
    hijack_initialize();
    msg("[mkostemp] create template file with template: %s, flags: %d\n",_template,flags);
    return try_function(old_mkostemp,_template,flags);
    
  }
  // read
  ssize_t read(int fildes, void *buf, size_t nbyte){
    hijack_initialize();
    msg("[read] read from fd: %d, into buffer:%p, max read: %u\n",fildes,buf,nbyte);
    ssize_t ret = try_function<ssize_t,-1>(old_read,fildes,buf,nbyte);
    if(ret > 0){
      if (config.max_read_dump_size >= 0){ // dump data
        int max_dump;
        if(config.max_read_dump_size == 0){
          max_dump = ret;
        }
        else{
          max_dump = min(ret,config.max_read_dump_size);
        }
        char* databuf = new char[max_dump + 1];
        memcpy(databuf,buf,max_dump);
        databuf[max_dump] = '\0';
        msg("[read==>>] data(%d byte): %s\n", ret,databuf);
        delete[] databuf;
      }
      else{ // no dump
        msg("[read==>>] data(%d byte)\n", ret);
      }
    }
    return ret;
  }
  // write
  ssize_t write(int fildes, const void *buf, size_t nbyte){
    hijack_initialize();
    msg("[write] write to fd: %d, from buffer:%p, max write: %u\n",fildes,buf,nbyte);
    ssize_t ret = try_function<ssize_t,-1>(old_write,fildes,buf,nbyte);
    if(ret > 0){
      if (config.max_write_dump_size >= 0){ // dump data
        int max_dump;
        if(config.max_write_dump_size == 0){
          max_dump = ret;
        }
        else{
          max_dump = min(ret,config.max_write_dump_size);
        }
        char* databuf = new char[max_dump + 1];
        memcpy(databuf,buf,max_dump);
        databuf[max_dump] = '\0';
        msg("[write==>>] data(%d byte): %s\n", ret,databuf);
        delete[] databuf;
      }
      else{ // no dump
        msg("[write==>>] data(%d byte)\n", ret);
      }
    }
    return ret;
  }
  // fwrite
  size_t fwrite(const void *ptr, size_t size, size_t nmemb, FILE *stream){
    hijack_initialize();
    msg("[fwrite] write to FILE stream: %p, from buffer:%p, max write: %u * %u \n",stream,ptr,size,nmemb);
    size_t ret_obj = try_function<size_t,0>(old_fwrite,ptr,size,nmemb,stream);
    ssize_t ret = ret_obj * size;
    if(ret > 0){
      if (config.max_write_dump_size >= 0){ // dump data
        int max_dump;
        if(config.max_write_dump_size == 0){
          max_dump = ret;
        }
        else{
          max_dump = min(ret,config.max_write_dump_size);
        }
        char* databuf = new char[max_dump + 1];
        memcpy(databuf,ptr,max_dump);
        databuf[max_dump] = '\0';
        msg("[fwrite==>>] data(%d byte): %s\n", ret,databuf);
        delete[] databuf;
      }
      else{ // no dump
        msg("[fwrite==>>] data(%d byte)\n", ret);
      }
    }
    return ret;
    
  }
  // fread
  size_t fread(void *ptr, size_t size, size_t nmemb, FILE *stream){
    hijack_initialize();
    msg("[fread] read from FILE stream: %p, into buffer:%p, max read: %u * %u\n",stream,ptr,size,nmemb);
    size_t ret_obj = try_function<size_t,0>(old_fread,ptr,size,nmemb,stream);
    ssize_t ret = ret_obj * size;
    if(ret > 0){
      if (config.max_read_dump_size >= 0){ // dump data
        int max_dump;
        if(config.max_read_dump_size == 0){
          max_dump = ret;
        }
        else{
          max_dump = min(ret,config.max_read_dump_size);
        }
        char* databuf = new char[max_dump + 1];
        memcpy(databuf,ptr,max_dump);
        databuf[max_dump] = '\0';
        msg("[fread==>>] data(%d byte): %s\n", ret,databuf);
        delete[] databuf;
      }
      else{ // no dump
        msg("[fread==>>] data(%d byte)\n", ret);
      }
    }
    return ret;
  
  }
  // getline
  ssize_t getline(char **lineptr, size_t *n, FILE *stream){
    hijack_initialize();
    msg("[getline] Read line from FILE stream: %p, into buffer: %p (size = %d)\n",stream,*lineptr,*n);
    ssize_t ret = try_function<ssize_t,-1>(old_getline,lineptr,n,stream);
    if(ret >= 0){
      msg("[getline==>>] data(%d byte): %s", ret,*lineptr);
    }
    return ret;
    
  }
  // __getdelim
  ssize_t __getdelim(char **lineptr, size_t *n, int delim, FILE *stream){
    hijack_initialize();
    //msg("[__getdelim] Read (delimiter:\\%d) from FILE stream: %p, into buffer: %p (size = %d)\n",delim,stream,*lineptr,*n);
    ssize_t ret = try_function<ssize_t,-1>(old___getdelim,lineptr,n,delim,stream);
    if(ret >= 0){
      //msg("[__getdelim==>>] data(%d byte): %s", ret,*lineptr);
    }
    return ret;
    
  }
  //fgetc
  int fgetc(FILE *stream){
    hijack_initialize();
    int c =  try_function<int,EOF>(old_fgetc,stream);
    msg("[fgetc] Read char from FILE stream: %p, character: %d(%c)\n",stream,c,c);
    return c;
  }
  //_IO_getc
  int _IO_getc(FILE *stream){
    hijack_initialize();
    int c =  try_function<int,EOF>(old__IO_getc,stream);
    msg("[_IO_getc] Read char from FILE stream: %p, character: %d(%c)\n",stream,c,c);
    return c;
  }
  // fputs
  int fputs(const char *s, FILE *stream){
    hijack_initialize();
    msg("[fputs] Write to FILE stream: %p, data: %s\n",stream,s);
    return try_function<int,EOF>(old_fputs,s,stream);
  }
  // rename
  int rename(const char *old_name, const char *new_name){
    hijack_initialize();
    msg("[rename] Rename file %s to %s\n",old_name,new_name);
    return try_function(old_rename,old_name,new_name);
  }
  // ftruncate
  int ftruncate(int fildes, off_t length){
    hijack_initialize();
    msg("[ftruncate] Truncate file descriptor %d, length: %u\n",fildes,length);
    return try_function(old_ftruncate,fildes,length);
  }
  // utime
  int utime(const char *path, const struct utimbuf *times){
    hijack_initialize();
    msg("[utime] set utime for file %s, using utimbuf at: %p\n",path,times);
    return try_function(old_utime,path,times);
  }
  // fputc
  int fputc(int c, FILE *stream){
    hijack_initialize();
    msg("[fputc] Write to FILE stream: %p, characeter: %d(%c)\n",stream,c,c);
    return try_function<int,EOF>(old_fputc,c,stream);
  }
  // _IO_putc
  int _IO_putc(int c, FILE *stream){
    hijack_initialize();
    msg("[_IO_putc] Write to FILE stream: %p, characeter: %d(%c)\n",stream,c,c);
    return try_function<int,EOF>(old__IO_putc,c,stream);
  }

  // connect
  int connect(int socket, const struct sockaddr *address, socklen_t address_len){
    hijack_initialize();
    if(address_len == sizeof(sockaddr_in)){
      const struct sockaddr_in* in_addr = (const struct sockaddr_in*)address;
      // show address
      char ip[20];
      inet_ntop(AF_INET,&in_addr->sin_addr,ip,sizeof(ip));
      msg("[connect] FD :%d, ip address: %s\n",socket,ip);
    }
    return try_function(old_connect,socket,address,address_len);
  }
  // SSL_read
  int SSL_read(SSL *ssl, void *buf, int num){
    hijack_initialize();
    msg("[SSL_read] read from encrypted channel: %p, into buffer:%p, max read: %u\n",ssl,buf,num);
    ssize_t ret = try_function(old_SSL_read,ssl,buf,num);
    if(ret > 0){
      if (config.max_read_dump_size >= 0){ // dump data
        int max_dump;
        if(config.max_read_dump_size == 0){
          max_dump = ret;
        }
        else{
          max_dump = min(ret,config.max_read_dump_size);
        }
        char* databuf = new char[max_dump + 1];
        memcpy(databuf,buf,max_dump);
        databuf[max_dump] = '\0';
        msg("[SSL_read==>>] data(%d byte): %s\n", ret,databuf);
        delete[] databuf;
      }
      else{ // no dump
        msg("[SSL_read==>>] data(%d byte)\n", ret);
      }
    }
    return ret; 
  }
  // SSL write
  int SSL_write(SSL *ssl, const void *buf, int num){
    hijack_initialize();
    msg("[SSL_write] write to encrypted channel: %p, from buffer:%p, max write: %u\n",ssl,buf,num);
    ssize_t ret = try_function(old_SSL_write,ssl,buf,num);
    if(ret > 0){
      if (config.max_write_dump_size >= 0){ // dump data
        int max_dump;
        if(config.max_write_dump_size == 0){
          max_dump = ret;
        }
        else{
          max_dump = min(ret,config.max_write_dump_size);
        }
        char* databuf = new char[max_dump + 1];
        memcpy(databuf,buf,max_dump);
        databuf[max_dump] = '\0';
        msg("[SSL_write==>>] data(%d byte): %s\n", ret,databuf);
        delete[] databuf;
      }
      else{ // no dump
        msg("[SSL_write==>>] data(%d byte)\n", ret);
      }
    }
    return ret;
    
  }
}
