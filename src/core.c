#define _POSIX_C_SOURCE 200809L
#include "core.h"
#include <limits.h>
#include <stdio.h>
#include <time.h>
#define MINUTE_NS INT64_C(60000000000)
int timer_start(AwakeTimer *timer, int minutes, int64_t now_ns)
{
    if (minutes < 0 || now_ns < 0 || (int64_t)minutes > INT64_MAX / MINUTE_NS) return -1;
    if (now_ns > INT64_MAX - (int64_t)minutes * MINUTE_NS) return -1;
    timer->active = 1;
    timer->end_ns = minutes ? now_ns + (int64_t)minutes * MINUTE_NS : 0;
    return 0;
}
void timer_stop(AwakeTimer *timer) { timer->active = 0; timer->end_ns = 0; }
int timer_expired(const AwakeTimer *timer, int64_t now_ns)
{ return timer->active && timer->end_ns && now_ns >= timer->end_ns; }
int timer_remaining_minutes(const AwakeTimer *timer, int64_t now_ns)
{
    if (!timer->active || now_ns < 0) return 0;
    if (!timer->end_ns) return -1;
    if (now_ns >= timer->end_ns) return 0;
    int64_t left = timer->end_ns - now_ns;
    int64_t minutes = left / MINUTE_NS + (left % MINUTE_NS != 0);
    return minutes > INT_MAX ? INT_MAX : (int)minutes;
}
int64_t awake_now_ns(void)
{
    struct timespec t;
#ifdef CLOCK_BOOTTIME
    if (!clock_gettime(CLOCK_BOOTTIME, &t)) return (int64_t)t.tv_sec * INT64_C(1000000000) + t.tv_nsec;
#endif
    if (!clock_gettime(CLOCK_MONOTONIC, &t)) return (int64_t)t.tv_sec * INT64_C(1000000000) + t.tv_nsec;
    return -1;
}
int laser_icon_write(const char *path, int active, int light_outline)
{
    FILE *f = fopen(path, "w");
    if (!f) return -1;
    fputs("<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"64\" height=\"50\" viewBox=\"0 0 28 22\">"
        "<g transform=\"translate(28 22) scale(-1 -1)\">", f);
    if (active) fputs("<path fill=\"#ff3b30\" fill-opacity=\".35\" d=\"M11 8L27 2L27 20L11 14Z\"/>"
        "<path stroke=\"#ff3b30\" stroke-width=\"1.8\" d=\"M11 11L27 11\"/>", f);
    fprintf(f,"<path fill=\"%s\" d=\"M1 6L8 6L11 9L11 13L8 16L1 16C-1 14 -1 8 1 6Z\"/>"
        "<ellipse cx=\"3.75\" cy=\"11\" rx=\"1.75\" ry=\"2\" fill=\"%s\"/></g></svg>\n",
        light_outline ? "#ffffff" : "#000000", active ? "#ff3b30" : "#8e8e93");
    int result = ferror(f) ? -1 : 0;
    if (fclose(f)) result = -1;
    return result;
}
