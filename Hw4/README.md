# Unix Programming Homework 4 : Simple Web Server
  - Author: 0116057 曾亮齊(Liang-Chi Tseng)

## Features implemented
- [basic] GET a static object
- [basic] GET a directory
- [optional] Execute CGI programs
  - The CGI must use STDIN as input, STDOUT as output so that it can communicate with browser
  - CGI files are like static objects, but they must have extensions listed below
    - .sh
    - .php
    - .cgi
  - CGI programs must be readable and executable
  - CGI programs could be script file (with shebang #!) or binary file 
