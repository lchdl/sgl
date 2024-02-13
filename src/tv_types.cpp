#include "tv_types.h"
#include <string.h>

void tv_time_record_start(tv_timespec* tobj)
{
    gettimeofday(&tobj->t_start, NULL);
}
tv_float tv_time_record_end(tv_timespec* tobj)
{
    gettimeofday(&tobj->t_end, NULL);
    tv_float dt_sec0 = (tobj->t_end.tv_sec - tobj->t_start.tv_sec);
    tv_float dt_sec1 = (tobj->t_end.tv_usec - tobj->t_start.tv_usec) / tv_float(1000000.0);
    return dt_sec0 + dt_sec1;
}
