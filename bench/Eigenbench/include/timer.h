#ifndef TIMER_H
#define TIMER_H 1

#include <sys/time.h>
#include <time.h>

#define TIMER_T                         struct timespec
#define TIMER_READ(time)                clock_gettime(CLOCK_MONOTONIC, &(time))
#define TIMER_DIFF_SECONDS(start, stop) \
    (((double)(stop.tv_sec)  + (double)(stop.tv_nsec / 1e9)) - \
     ((double)(start.tv_sec) + (double)(start.tv_nsec / 1e9)))

#endif /* TIMER_H */
