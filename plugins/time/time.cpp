/**
 * @file time.cpp
 * @brief  Brief description of file.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <angort/angort.h>

using namespace angort;

%name time
%shared

static timespec progstart;

inline double time_diff(timespec start, timespec end)
{
    timespec temp;
    if ((end.tv_nsec-start.tv_nsec)<0) {
        temp.tv_sec = end.tv_sec-start.tv_sec-1;
        temp.tv_nsec = 1000000000+end.tv_nsec-start.tv_nsec;
    } else {
        temp.tv_sec = end.tv_sec-start.tv_sec;
        temp.tv_nsec = end.tv_nsec-start.tv_nsec;
    }
    
    double t = temp.tv_sec;
    double ns = temp.tv_nsec;
    t += ns*1e-9;
    return t;
}

%word now (-- float) get current time
{
    struct timespec t;
    extern struct timespec progstart;
    
    clock_gettime(CLOCK_MONOTONIC,&t);
    double diff=time_diff(progstart,t);
    a->pushFloat((float)diff);
}

%word delay (float --) wait for a number of seconds
{
    float f = a->popFloat();
    f *= 1e6f;
    usleep((int)f);
}


%init
{
    clock_gettime(CLOCK_MONOTONIC,&progstart);
    fprintf(stderr,"Initialising Time plugin, %s %s\n",__DATE__,__TIME__);
}

