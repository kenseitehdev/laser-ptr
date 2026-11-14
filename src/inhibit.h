#ifndef LASER_PTR_INHIBIT_H
#define LASER_PTR_INHIBIT_H
#include <gio/gio.h>
typedef struct {
    GDBusConnection *session;
    int sleep_fd;
    guint32 cookie;
    gboolean has_cookie;
    const char *screen_name, *screen_path, *screen_interface, *release_method;
} Inhibitors;
/* Blocking acquisition belongs on a worker thread. Both locks are required. */
Inhibitors *inhibitors_acquire(GError **error);
void inhibitors_release(Inhibitors *locks);
#endif
