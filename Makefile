OBJDIR = build
SRCDIR = src
BINDIR = bin

SERVER_SRCS  = $(SRCDIR)/redis.cpp \
							 $(SRCDIR)/utils.cpp \
							 $(SRCDIR)/database.cpp \
							 $(SRCDIR)/log.cpp \
							 $(SRCDIR)/fileprocessor.cpp \
							 $(SRCDIR)/threadpool.c \
							 $(SRCDIR)/net_recv.cpp 

SERVER_OBJS  = $(patsubst $(SRCDIR)/%.cpp, $(OBJDIR)/%.o, $(SERVER_SRCS))

CLIENT_SRCS  = net_send.cpp
CLIENT_OBJS  = $(patsubst $(SRCDIR)/%.cpp, $(OBJDIR)/%.o, $(SRCDIR)/$(CLIENT_SRCS))

CCX = g++
CCXFLAGS = -Wall -Wextra -O

EVPATH_LIB_DIRS = -L/net/hp41/chaos/rhe6-64-icc/atl/lib -L/net/hp41/chaos/rhe6-64-icc/evpath/lib -L/net/hp41/chaos/rhe6-64-icc/ffs/lib -L/net/hp41/chaos/rhe6-64-icc/dill/lib -L/net/hp41/chaos/rhe6-64-icc/cercs_env/lib -Wl,-rpath=/net/hp41/chaos/rhe6-64-icc/atl/lib -Wl,-rpath=/net/hp41/chaos/rhe6-64-icc/ffs/lib -Wl,-rpath=/net/hp41/chaos/rhe6-64-icc/dill/lib -Wl,-rpath=/net/hp41/chaos/rhe6-64-icc/cercs_env/lib

EVPATH_INCLUDE_DIRS = -I/net/hp41/chaos/rhe6-64-icc/atl/include -I/net/hp41/chaos/rhe6-64-icc/ffs/include -I/net/hp41/chaos/rhe6-64-icc/evpath/include

REDIS_LIB_DIRS  = -L/net/hu21/agangil3/hiredis -Wl,-rpath=/net/hu21/agangil3/hiredis 
REDIS_INCLUDE_DIRS=-I/net/hu21/agangil3/hiredis

PYTHON_INCLUDE_DIRS = -I/usr/include/python2.6 
PYTHON_LIB_DIRS = -L/usr/lib64/python -Wl,-rpath=/usr/lib64

EVPATH_LIBS  = -latl -levpath -lffs -ldill -lcercs_env -lrt
PYTHON_LIBS  = -lpython2.6
REDIS_LIBS   = -lhiredis
PTHREAD_LIBS = -lpthread

SERVER = net_recv
CLIENT = net_send

all: builddir $(SERVER) $(CLIENT)
	
builddir:
	mkdir -p $(OBJDIR)

$(SERVER): $(SERVER_OBJS)
	$(CCX) $(SERVER_OBJS) $(EVPATH_LIB_DIRS) $(REDIS_LIB_DIRS) $(PYTHON_LIB_DIRS) -o $@ $(EVPATH_LIBS) $(PYTHON_LIBS) $(REDIS_LIBS) $(PTHREAD_LIBS)

$(CLIENT): $(CLIENT_OBJS)
	$(CCX) $(CLIENT_OBJS) $(EVPATH_LIB_DIRS) -o $@ $(EVPATH_LIBS)

$(OBJDIR)/%.o: $(SRCDIR)/%.cpp
	$(CCX) $(EVPATH_INCLUDE_DIRS) $(PYTHON_INCLUDE_DIRS) $(REDIS_INCLUDE_DIRS) $(OPTS) -c $< -o $@

clean:
	rm $(SERVER) $(CLIENT) $(OBJDIR) -Rf

