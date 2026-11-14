#include "core.h"
#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define MINUTE INT64_C(60000000000)
int main(int argc, char **argv)
{
    if (argc == 5 && !strcmp(argv[1], "icon"))
        return laser_icon_write(argv[2], atoi(argv[3]), atoi(argv[4])) ? 1 : 0;
    AwakeTimer t = {0};
    assert(!timer_expired(&t, 0)); assert(timer_remaining_minutes(&t, 0) == 0);
    assert(!timer_start(&t, 0, 42)); assert(t.active && !t.end_ns);
    assert(!timer_expired(&t, INT64_MAX)); assert(timer_remaining_minutes(&t, 100) == -1);
    const int durations[] = {30,60,120};
    for (size_t i = 0; i < sizeof durations / sizeof *durations; ++i) {
        assert(!timer_start(&t, durations[i], 17));
        assert(t.end_ns == 17 + durations[i] * MINUTE);
        assert(timer_remaining_minutes(&t, 18) == durations[i]);
        assert(timer_remaining_minutes(&t, t.end_ns-1) == 1);
        assert(!timer_expired(&t, t.end_ns-1));
        assert(timer_expired(&t, t.end_ns));
        assert(timer_remaining_minutes(&t, t.end_ns) == 0);
        assert(timer_expired(&t, t.end_ns+MINUTE));
    }
    assert(!timer_start(&t,30,100));
    assert(!timer_start(&t,60,200)); assert(t.end_ns == 200+60*MINUTE);
    AwakeTimer saved=t;
    assert(timer_start(&t,-1,0)==-1); assert(t.end_ns==saved.end_ns);
    assert(timer_start(&t,INT_MAX,0)==-1); assert(t.end_ns==saved.end_ns);
    assert(timer_start(&t,30,INT64_MAX)==-1); assert(t.end_ns==saved.end_ns);
    assert(timer_start(&t,30,-1)==-1);
    timer_stop(&t); assert(!t.active && !t.end_ns);
    assert(awake_now_ns() >= 0);
    puts("Timer checks passed: durations, indefinite, expiry, replacement, stop, invalid inputs and overflow.");
    return 0;
}
