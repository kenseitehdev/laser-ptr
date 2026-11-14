#include <gtk/gtk.h>
#include <libayatana-appindicator/app-indicator.h>
#include <glib-unix.h>
#include <glib/gstdio.h>
#include <signal.h>
#include "core.h"
#include "inhibit.h"

static AppIndicator *indicator;
static GtkWidget *toggle, *status_item, *durations;
static AwakeTimer timer;
static Inhibitors *locks;
static char *icon_dir;
static gboolean busy, light_outline = TRUE;
static guint generation;

typedef struct { int minutes; guint generation; } Request;
static void show_error(const char *message)
{
    GtkWidget *dialog = gtk_message_dialog_new(NULL, GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR,
        GTK_BUTTONS_CLOSE, "%s", message);
    gtk_dialog_run(GTK_DIALOG(dialog)); gtk_widget_destroy(dialog);
}
static void update(void)
{
    gtk_menu_item_set_label(GTK_MENU_ITEM(toggle), timer.active ? "Turn Off" : "Keep Computer Awake");
    gtk_widget_set_sensitive(toggle, !busy);
    gtk_widget_set_sensitive(durations, !busy);
    char *text;
    if (busy) text = g_strdup("Requesting keep-awake inhibitors…");
    else if (!timer.active) text = g_strdup("Off");
    else {
        int remaining = timer_remaining_minutes(&timer, awake_now_ns());
        text = remaining < 0 ? g_strdup("On · indefinitely") : g_strdup_printf("On · about %d min left", remaining);
    }
    gtk_menu_item_set_label(GTK_MENU_ITEM(status_item), text); g_free(text);
    app_indicator_set_icon_full(indicator, timer.active ? "laser-on" : "laser-off",
        timer.active ? "laser-ptr active" : "laser-ptr off");
}
static void deactivate(void)
{
    ++generation;
    timer_stop(&timer); inhibitors_release(locks); locks = NULL;
    update();
}
static void acquire_worker(GTask *task, gpointer source, gpointer data, GCancellable *cancel)
{
    (void)source; (void)data; (void)cancel;
    GError *error = NULL;
    Inhibitors *result = inhibitors_acquire(&error);
    if (result) g_task_return_pointer(task, result, (GDestroyNotify)inhibitors_release);
    else g_task_return_error(task, error);
}
static void acquired(GObject *source, GAsyncResult *result, gpointer data)
{
    (void)source;
    Request *request = data;
    GError *error = NULL;
    Inhibitors *candidate = g_task_propagate_pointer(G_TASK(result), &error);
    busy = FALSE;
    if (request->generation != generation) {
        inhibitors_release(candidate); g_clear_error(&error); g_free(request); update(); return;
    }
    if (!candidate) {
        update(); show_error(error ? error->message : "Could not acquire keep-awake inhibitors.");
        g_clear_error(&error); g_free(request); return;
    }
    int64_t now = awake_now_ns();
    if (timer_start(&timer, request->minutes, now)) {
        inhibitors_release(candidate); update(); show_error("Could not start the keep-awake timer.");
    } else {
        inhibitors_release(locks); locks = candidate; update();
    }
    g_free(request);
}
static void activate(int minutes)
{
    if (busy) return;
    /* Keep existing locks until their replacements have been acquired. */
    Request *request = g_new(Request, 1);
    request->minutes = minutes; request->generation = ++generation;
    busy = TRUE; update();
    GTask *task = g_task_new(NULL, NULL, acquired, request);
    g_task_run_in_thread(task, acquire_worker); g_object_unref(task);
}
static void toggle_clicked(GtkMenuItem *item, gpointer data)
{ (void)item; (void)data; if (timer.active) deactivate(); else activate(0); }
static void duration_clicked(GtkMenuItem *item, gpointer data)
{ (void)item; activate(GPOINTER_TO_INT(data)); }
static void quit_clicked(GtkMenuItem *item, gpointer data)
{ (void)item; (void)data; deactivate(); gtk_main_quit(); }
static gboolean signal_quit(gpointer data)
{ (void)data; deactivate(); gtk_main_quit(); return G_SOURCE_REMOVE; }
static gboolean tick(gpointer data)
{
    (void)data;
    int64_t now = awake_now_ns();
    if (timer.active && (now < 0 || timer_expired(&timer, now))) deactivate();
    else update();
    return G_SOURCE_CONTINUE;
}
static void owner_changed(GDBusConnection *bus, const char *sender, const char *path,
    const char *iface, const char *signal, GVariant *parameters, gpointer data)
{
    (void)bus; (void)sender; (void)path; (void)iface; (void)signal; (void)data;
    const char *name, *old_owner, *new_owner;
    g_variant_get(parameters, "(&s&s&s)", &name, &old_owner, &new_owner);
    if (!*old_owner || !g_strcmp0(old_owner, new_owner)) return;
    gboolean affected = locks && (!g_strcmp0(name, locks->screen_name) || !g_strcmp0(name, "org.freedesktop.login1"));
    if (busy && (!g_strcmp0(name, "org.freedesktop.ScreenSaver") || !g_strcmp0(name, "org.gnome.SessionManager") ||
        !g_strcmp0(name, "org.mate.SessionManager") || !g_strcmp0(name, "org.freedesktop.login1"))) {
        ++generation;
    }
    if (affected) {
        if (!g_strcmp0(name, locks->screen_name)) locks->has_cookie = FALSE;
        deactivate(); show_error("A power-management service restarted or stopped. Keep-awake mode was turned off; activate it again to reacquire the inhibitors."); }
}
static guint watch_bus(GDBusConnection *bus)
{
    return g_dbus_connection_signal_subscribe(bus, "org.freedesktop.DBus", "org.freedesktop.DBus",
        "NameOwnerChanged", "/org/freedesktop/DBus", NULL, G_DBUS_SIGNAL_FLAGS_NONE, owner_changed, NULL, NULL);
}
int main(int argc, char **argv)
{
    for (int i = 1; i < argc; ++i) {
        if (!g_strcmp0(argv[i], "--dark-outline")) light_outline = FALSE;
        else if (!g_strcmp0(argv[i], "--help")) {
            g_print("laser-ptr [--dark-outline]\nA Linux tray keep-awake utility.\n"); return 0;
        } else { g_printerr("Unknown option: %s\n", argv[i]); return 2; }
    }
    if (!gtk_init_check(NULL, NULL)) { g_printerr("laser-ptr needs a graphical desktop session.\n"); return 1; }
    GError *error = NULL;
    icon_dir = g_dir_make_tmp("laser-ptr-icons-XXXXXX", &error);
    if (!icon_dir) { g_printerr("%s\n", error->message); g_clear_error(&error); return 1; }
    for (int i = 0; i < 2; ++i) {
        char *path = g_build_filename(icon_dir, i ? "laser-on.svg" : "laser-off.svg", NULL);
        int result = laser_icon_write(path, i, light_outline); g_free(path);
        if (result) { g_printerr("Could not create the laser icon.\n"); return 1; }
    }
    indicator = app_indicator_new("laser-ptr", "laser-off", APP_INDICATOR_CATEGORY_HARDWARE);
    app_indicator_set_icon_theme_path(indicator, icon_dir);
    app_indicator_set_title(indicator, "laser-ptr");
    GtkWidget *menu = gtk_menu_new();
    toggle = gtk_menu_item_new_with_label("Keep Computer Awake");
    g_signal_connect(toggle, "activate", G_CALLBACK(toggle_clicked), NULL);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), toggle);
    durations = gtk_menu_item_new_with_label("Keep Awake For");
    GtkWidget *submenu = gtk_menu_new();
    const char *labels[] = {"Indefinitely", "30 Minutes", "1 Hour", "2 Hours"};
    const int minutes[] = {0, 30, 60, 120};
    for (int i = 0; i < 4; ++i) {
        GtkWidget *item = gtk_menu_item_new_with_label(labels[i]);
        g_signal_connect(item, "activate", G_CALLBACK(duration_clicked), GINT_TO_POINTER(minutes[i]));
        gtk_menu_shell_append(GTK_MENU_SHELL(submenu), item);
    }
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(durations), submenu);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), durations);
    status_item = gtk_menu_item_new_with_label("Off"); gtk_widget_set_sensitive(status_item, FALSE);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), status_item);
    GtkWidget *quit = gtk_menu_item_new_with_label("Quit laser-ptr");
    g_signal_connect(quit, "activate", G_CALLBACK(quit_clicked), NULL);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), quit);
    gtk_widget_show_all(menu); app_indicator_set_menu(indicator, GTK_MENU(menu));
    app_indicator_set_status(indicator, APP_INDICATOR_STATUS_ACTIVE);
    GDBusConnection *session = g_bus_get_sync(G_BUS_TYPE_SESSION, NULL, NULL);
    GDBusConnection *system = g_bus_get_sync(G_BUS_TYPE_SYSTEM, NULL, NULL);
    guint session_watch = session ? watch_bus(session) : 0;
    guint system_watch = system ? watch_bus(system) : 0;
    guint timer_id = g_timeout_add_seconds(1, tick, NULL);
    g_unix_signal_add(SIGTERM, signal_quit, NULL); g_unix_signal_add(SIGINT, signal_quit, NULL);
    update(); gtk_main();
    inhibitors_release(locks); locks = NULL;
    if (session_watch) g_dbus_connection_signal_unsubscribe(session, session_watch);
    if (system_watch) g_dbus_connection_signal_unsubscribe(system, system_watch);
    g_clear_object(&session); g_clear_object(&system); g_source_remove(timer_id);
    g_object_unref(indicator); gtk_widget_destroy(menu);
    GDir *dir = g_dir_open(icon_dir, 0, NULL);
    if (dir) { const char *name; while ((name = g_dir_read_name(dir))) {
        char *path = g_build_filename(icon_dir, name, NULL); g_remove(path); g_free(path);
    } g_dir_close(dir); }
    g_rmdir(icon_dir); g_free(icon_dir);
    return 0;
}
