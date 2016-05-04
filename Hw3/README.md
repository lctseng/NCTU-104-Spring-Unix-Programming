# Unix Programming Homework 3 : Simple Shell
  - Author: 0116057 曾亮齊(Liang-Chi Tseng)

## Features implemented
- All basic requirements
  - Execute a single command
  - Properly block or unblock signals
    - SIGINT and SIGQUIT will not effect running shell if there are running foreground commands
    - If no foreground commands are running, the shell will receive SIGINT and SIGQUIT
  - Replace standard input/output of a process using the redirection operators
    - use << or < to redirect standard input
    - use >> or > to redirect standard output
      - >> : append on existing file
      - >  : truncate existing file
  - Setup foreground process group and background process groups
  - Create pipeline for commands separated by the pipe operator (|), and put the commands into the same process group.
- All optional requirements listed on course website
  - Manipulate environment variables
    - List all environment variables
    ```
    printenv
    ```
    - List specific environment variable
    ```
    printenv PATH
    ```
    - Set or modify environment variables
    ```
    export PATH=.:/usr/bin
    ```
    - Remove environment variables
    ```
    unset PATH
    ```
  - Expand of the * and ? operators
    - Implemented by using glob(3)
    - Example Usages
    ```
    shell-prompt$ ls *
    shell-prompt$ ls lib.?
    ```
  - Job control: support process suspension using Ctrl-Z, and fg and bg command.
    - Ctrl-Z: stop a foreground process group and put it into background
    - fg: bring a stopped  background process group to foreground and continue execution
    - bg: continue execution of a stopped backgound process group
    - cmd &: execution the group of commands in background

