#include "inhibit.h"
#include <gio/gunixfdlist.h>
#include <unistd.h>

void inhibitors_release(Inhibitors *locks)
{
    if (!locks) return;
    if (locks->sleep_fd >= 0) close(locks->sleep_fd);
    if (locks->has_cookie)
        g_dbus_connection_call(locks->session, locks->screen_name, locks->screen_path,
            locks->screen_interface, locks->release_method, g_variant_new("(u)", locks->cookie),
            NULL, G_DBUS_CALL_FLAGS_NO_AUTO_START, 3000, NULL, NULL, NULL);
    g_clear_object(&locks->session);
    g_free(locks);
}
Inhibitors *inhibitors_acquire(GError **error)
{
    Inhibitors *locks = g_new0(Inhibitors, 1);
    locks->sleep_fd = -1;
    locks->session = g_bus_get_sync(G_BUS_TYPE_SESSION, NULL, error);
    if (!locks->session) goto fail;
    const char *names[] = {"org.freedesktop.ScreenSaver", "org.freedesktop.ScreenSaver", "org.gnome.SessionManager", "org.mate.SessionManager"};
    const char *paths[] = {"/org/freedesktop/ScreenSaver", "/ScreenSaver", "/org/gnome/SessionManager", "/org/mate/SessionManager"};
    for (int i = 0; i < 4; ++i) {
        GError *screen_error = NULL;
        GVariant *args = i < 2 ? g_variant_new("(ss)", "laser-ptr", "Keep the display awake") :
            g_variant_new("(susu)", "laser-ptr", 0u, "Keep the display awake", 8u);
        GVariant *reply = g_dbus_connection_call_sync(locks->session, names[i], paths[i], names[i],
            "Inhibit", args, G_VARIANT_TYPE("(u)"), G_DBUS_CALL_FLAGS_NONE, 3000, NULL, &screen_error);
        if (reply) {
            g_variant_get(reply, "(u)", &locks->cookie);
            locks->has_cookie = TRUE; locks->screen_name = names[i]; locks->screen_path = paths[i];
            locks->screen_interface = names[i]; locks->release_method = i < 2 ? "UnInhibit" : "Uninhibit";
            g_variant_unref(reply); g_clear_error(&screen_error); break;
        }
        g_clear_error(&screen_error);
    }
    if (!locks->has_cookie) {
        g_set_error_literal(error, G_IO_ERROR, G_IO_ERROR_NOT_SUPPORTED,
            "No supported desktop screen-idle inhibitor is available. laser-ptr was not activated.");
        goto fail;
    }
    GDBusConnection *system = g_bus_get_sync(G_BUS_TYPE_SYSTEM, NULL, error);
    if (!system) goto fail;
    GUnixFDList *fds = NULL;
    GVariant *reply = g_dbus_connection_call_with_unix_fd_list_sync(system,
        "org.freedesktop.login1", "/org/freedesktop/login1", "org.freedesktop.login1.Manager", "Inhibit",
        g_variant_new("(ssss)", "idle:sleep", "laser-ptr", "Keep the computer awake", "block"),
        G_VARIANT_TYPE("(h)"), G_DBUS_CALL_FLAGS_ALLOW_INTERACTIVE_AUTHORIZATION,
        120000, NULL, &fds, NULL, error);
    g_object_unref(system);
    if (!reply) { g_clear_object(&fds); goto fail; }
    gint32 index;
    g_variant_get(reply, "(h)", &index);
    g_variant_unref(reply);
    if (!fds) {
        g_set_error_literal(error, G_IO_ERROR, G_IO_ERROR_FAILED, "logind did not return an inhibitor descriptor.");
        goto fail;
    }
    locks->sleep_fd = g_unix_fd_list_get(fds, index, error);
    g_object_unref(fds);
    if (locks->sleep_fd < 0) goto fail;
    return locks;
fail:
    inhibitors_release(locks);
    return NULL;
}
