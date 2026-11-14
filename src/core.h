#ifndef LASER_PTR_CORE_H
#define LASER_PTR_CORE_H
#include <stdint.h>
typedef struct { int active; int64_t end_ns; } AwakeTimer;
/* A zero-minute duration means indefinite; negative/overflowing inputs fail. */
int timer_start(AwakeTimer *timer, int minutes, int64_t now_ns);
void timer_stop(AwakeTimer *timer);
int timer_expired(const AwakeTimer *timer, int64_t now_ns);
int timer_remaining_minutes(const AwakeTimer *timer, int64_t now_ns);
int64_t awake_now_ns(void);
int laser_icon_write(const char *path, int active, int light_outline);
#endif
