CCX=g++
CC=gcc

CCXFLAGS= -Wall -Wextra -O

LIBDIRS=-L/net/hp41/chaos/rhe6-64-icc/atl/lib -L/net/hp41/chaos/rhe6-64-icc/evpath/lib -L/net/hp41/chaos/rhe6-64-icc/ffs/lib -L/net/hp41/chaos/rhe6-64-icc/dill/lib -L/net/hp41/chaos/rhe6-64-icc/cercs_env/lib -I/net/hp41/chaos/rhe6-64-icc/atl/include -I/net/hp41/chaos/rhe6-64-icc/ffs/include -I/net/hp41/chaos/rhe6-64-icc/evpath/include -Wl,-rpath=/net/hp41/chaos/rhe6-64-icc/atl/lib -Wl,-rpath=/net/hp41/chaos/rhe6-64-icc/ffs/lib -Wl,-rpath=/net/hp41/chaos/rhe6-64-icc/dill/lib -Wl,-rpath=/net/hp41/chaos/rhe6-64-icc/cercs_env/lib -L.

LIBS=-lthr_pool -latl -levpath -lffs -ldill -lcercs_env -lpthread -lrt

all:
	$(CCX) $(LIBDIRS) -o send net_send.cpp $(LIBS)
	$(CCX) -c threadpool.c 
	$(CCX) $(LIBDIRS) net_recv.cpp -o recv threadpool.o $(LIBS)

clean:
	rm send recv *.o
