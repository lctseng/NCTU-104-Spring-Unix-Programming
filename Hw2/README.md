# Unix Programming Homework 2 : Hijack 

## How to test the code
- Compile everything using 'make'
  - Source files are: hijack.cpp, configure.hpp
  - Please compile them in CS Linux Workstation
  - The GNU C++ compiler must support C++14 standard
- Use LD_PRELOAD to load the shared object: hijack.so
  - target program is 'wget'
  - example usage: `env LD_PRELOAD=./hijack.so wget https://www.google.com.tw`

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

