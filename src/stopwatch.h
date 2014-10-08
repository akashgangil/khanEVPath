#ifndef STOPWATCH_H
#define STOPWATCH_H

#include <sys/time.h>

struct stopwatch_t
{
    struct timeval t_start_;
    struct timeval t_stop_;
    int is_running_;
};


long double stopwatch_elapsed(struct stopwatch_t*);

void stopwatch_init(void);

void stopwatch_start(struct stopwatch_t*);

long double stopwatch_stop(struct stopwatch_t*);

struct stopwatch_t* stopwatch_create(void);

void stopwatch_destroy(struct stopwatch_t*);

#endif
