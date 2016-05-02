CXX = g++
CFLAGS = --std=c++14
shell: shell.o pipe.o fdstream.o lib.o
	$(CXX) $(CFLAGS) -o $@ $^
%.o: %.cpp %.h
	$(CXX) $(CFLAGS) -c $<
