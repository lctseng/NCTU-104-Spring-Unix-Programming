GCC=gcc
GXX=g++
CFLAGS= -I. -Wall -O2

hw1: job.o main.o
	$(GCC) -o $@ $^ $(CFLAGS)
%.c:
	$(GCC) -c $< $(CFLAGS)
