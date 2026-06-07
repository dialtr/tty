CXX=g++
CXXFLAGS=-Wall -Werror

.PHONY:
all: tty

.PHONY:
clean:
	-rm -f tty *.o

tty: tty.o
	$(CXX) $(CXXFLAGS) -o tty $^

.cc.o:
	$(CXX) $(CXXFLAGS) -c $<


