#ifndef DATABASE_H
#define DATABASE_H

#include <stdlib.h>
#include <iostream>
#include <sstream>
#include <string>

#define REDIS_FOUND

#ifdef REDIS_FOUND
#include "redis.h"
#endif

#ifdef VOLDEMORT_FOUND
#include "voldemort.h"
#endif

#ifdef BDB_FOUND
#include "bdb.h"
#endif

#define VOLDEMORT 1
#define REDIS 2
#define BDB 3
#define DATABASE REDIS
#define BILLION 1E9

#ifdef APPLE
    #include <sys/statvfs.h>
    #include <sys/dir.h>
    #define CLOCK_REALTIME 1
#endif
extern struct timespec start, stop;
extern double time_spent;

extern int vold_calls;
extern int readdir_calls;
extern int access_calls;
extern int getattr_calls;
extern int read_calls;
extern int write_calls;
extern int create_calls;
extern int rename_calls;
extern double tot_time;
extern double vold_avg_time;
extern double readdir_avg_time;
extern double access_avg_time;
extern double getattr_avg_time;
extern double read_avg_time;
extern double write_avg_time;
extern double create_avg_time;
extern double rename_avg_time;
extern double localize_time;
extern int redis_calls;
extern double redis_avg_time;
extern int bdb_calls;
extern double bdb_avg_time;

bool init_database(int port);
std::string database_setval(std::string file_id, std::string col, std::string val);
std::string database_getval(std::string col, std::string val);
std::string database_getvals(std::string col);
void database_remove_val(std::string file, std::string col, std::string val);

#endif
