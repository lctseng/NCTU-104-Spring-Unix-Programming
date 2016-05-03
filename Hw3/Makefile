CXX = g++
CFLAGS = --std=c++14
shell: shell.o pipe.o fdstream.o lib.o
	$(CXX) $(CFLAGS) -o $@ $^
shell.o: shell.cpp
	$(CXX) $(CFLAGS) -c $< -o $@
%.o: %.cpp %.h lib.h
	$(CXX) $(CFLAGS) -c $< -o $@
clean:
	rm *.o
	rm shell
