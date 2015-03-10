CC = g++

all:
	$(CC) -c cfgparser.cpp
	$(CC) -c configwrapper.cpp
	$(CC) -c test-cfgparser.cpp
	$(CC) -o test test-cfgparser.o cfgparser.o configwrapper.o

clean:
	rm test *.o
