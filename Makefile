CCX=g++

CCXFLAGS= -Wall -Wextra -O

LIBDIRS=-L/net/hp41/chaos/rhe6-64-icc/atl/lib -L/net/hp41/chaos/rhe6-64-icc/evpath/lib -L/net/hp41/chaos/rhe6-64-icc/ffs/lib -L/net/hp41/chaos/rhe6-64-icc/dill/lib -L/net/hp41/chaos/rhe6-64-icc/cercs_env/lib -I/net/hp41/chaos/rhe6-64-icc/atl/include -I/net/hp41/chaos/rhe6-64-icc/ffs/include -I/net/hp41/chaos/rhe6-64-icc/evpath/include -Wl,-rpath=/net/hp41/chaos/rhe6-64-icc/atl/lib -Wl,-rpath=/net/hp41/chaos/rhe6-64-icc/ffs/lib -Wl,-rpath=/net/hp41/chaos/rhe6-64-icc/dill/lib -Wl,-rpath=/net/hp41/chaos/rhe6-64-icc/cercs_env/lib

LIBS=-latl -levpath -lffs -ldill -lcercs_env -lpthread

all:
	$(CCX) $(LIBDIRS) -o recv net_recv.cpp $(LIBS)
	$(CCX) $(LIBDIRS) -o send net_send.cpp $(LIBS)

clean:
	rm send recv
