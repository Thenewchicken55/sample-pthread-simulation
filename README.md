# pthread Simulation

This repository contains a sample multithreaded program with proper critical section handeling for practice

## Setup

To run the program, use the command
``` bash
g++ -std=c++11 -pthread -o main main.cpp
./main
```

To run the program line by line using GDB ([GNU Project Debugger](https://sourceware.org/gdb/))
``` bash
g++ -std=c++11 -pthread -g -o main main.cpp
gdb main
```

## Todo

Create a makerun file
