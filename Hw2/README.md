# Unix Programming Homework 2 : Hijack 

## How to test the code
- Compile everything using 'make'
  - Source files are: hijack.cpp, configure.hpp
  - Please compile them in CS Linux Workstation
  - The GNU C++ compiler must support C++14 standard
- Use LD_PRELOAD to load the shared object: hijack.so
  - target program is 'wget'
  - example usage: `env LD_PRELOAD=./hijack.so wget https://www.google.com.tw`
- Special notation for read/write related function
  - There will be two line for each function call to these functions
  - The first line is shown when function is called
  - The second line is shown when the original function returnd (have ==>> after function name)
  - For example, when SSL_read is called:
  ```
  [SSL_read] read from encrypted channel: 0x163fab0, into buffer:0x153a660, max read: 8192
  [SSL_read==>>] data(655 byte): <!doctype html><html itemscope="" itemtype="http:/
  ```
  - Note that these two lines may NOT be consecutive
    - Several function calls may occur between these two lines
    - For example:
    ```
    [SSL_read] (output for calling SSL_read)
    [read] (internal function call in SSL_read)
    [read==>>] (output after internal function call returned, may be binary data)
    [read] (same as above)
    [read==>>] (same as above)
    [SSL_read==>] (output after original SSL_read returned)
    ```

## Features
- Hijack Log
  - All information will be recorded in the log file
  - The default log filename is 'hijack.log'
  - Log filename could be changed in configuration file (see below)
- Configuration File
  - There is a configuration file named 'hijack.conf'
  - For more detail, please refer to that file

## Hijacked Targets
- Generic 
  - open
  - chmod
  - unlink
  - mkdir
  - fopen
  - iconv_open
  - fdopen
  - freopen
  - mkostemp
  - read
  - write
  - fwrite
  - fread
  - getline
  - __getdelim
  - fgetc
  - _IO_getc
  - fputs
  - rename
  - ftruncate
  - utime
  - fputc
  - _IO_putc
- Socket and SSL
  - connect
  - SSL_read
  - SSL_write

