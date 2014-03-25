CCX=g++
CC=gcc

CCXFLAGS= -Wall -Wextra -O

LIBDIRS=-L/net/hp41/chaos/rhe6-64-icc/atl/lib -L/net/hp41/chaos/rhe6-64-icc/evpath/lib -L/net/hp41/chaos/rhe6-64-icc/ffs/lib -L/net/hp41/chaos/rhe6-64-icc/dill/lib -L/net/hp41/chaos/rhe6-64-icc/cercs_env/lib -I/net/hp41/chaos/rhe6-64-icc/atl/include -I/net/hp41/chaos/rhe6-64-icc/ffs/include -I/net/hp41/chaos/rhe6-64-icc/evpath/include -Wl,-rpath=/net/hp41/chaos/rhe6-64-icc/atl/lib -Wl,-rpath=/net/hp41/chaos/rhe6-64-icc/ffs/lib -Wl,-rpath=/net/hp41/chaos/rhe6-64-icc/dill/lib -Wl,-rpath=/net/hp41/chaos/rhe6-64-icc/cercs_env/lib -L. -I/net/hu21/agangil3/hiredis -L/net/hu21/agangil3/hiredis -Wl,-rpath=/net/hu21/agangil3/hiredis -I/usr/include/python2.6 -L/usr/lib64/python -Wl,-rpath=/usr/lib64

LIBS=-latl -levpath -lffs -ldill -lcercs_env -lpthread -lrt -lhiredis -lpython2.6

all:
	$(CCX) -c redis.cpp
	$(CCX) -c utils.cpp 
	$(CCX) -c database.cpp
	$(CCX) -c log.cpp
	$(CCX) -c -I/usr/include/python2.6 fileprocessor.cpp 
	$(CCX) $(LIBDIRS) -o send net_send.cpp $(LIBS)
	$(CCX) -c threadpool.c 
	$(CCX) $(LIBDIRS) net_recv.cpp -o recv threadpool.o database.o utils.o redis.o log.o fileprocessor.o $(LIBS)

clean:
	rm send recv *.o
