/* defines fundamental types */
#pragma once
#include <stdint.h> /* for int*_t, uint*_t, ... */
#include <string.h> /* for memset, memcpy, ... */
#include <sys/time.h> /* please compile on MinGW if sys/time.h is not found. */
#include <math.h>

typedef double   tv_float; /* float64 is used by default, do not change this to float32 */
typedef uint8_t  tv_u8_t;
typedef int8_t   tv_i8_t;
typedef int32_t  tv_i32_t;
typedef uint32_t tv_u32_t;
typedef uint64_t tv_u64_t;

struct tv_timespec {
    /* stores data for timing */
    timeval t_start, t_end;
};

struct tv_surface {
    /* 
    Render surface with arbitrary pixel formats,
    with data stored on RAM and can be randomly
    accessed by CPU. Surface object can also store
    loaded image texture data. 
    */
    tv_i32_t h, w;     /* size of the surface */
    void* pixels;  /* pixel data pointer */
};

void tv_time_record_start(tv_timespec* tobj);
tv_float tv_time_record_end(tv_timespec* tobj);

