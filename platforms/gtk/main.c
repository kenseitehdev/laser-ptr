#include <gtk/gtk.h>
#include <gio/gio.h>
#ifdef HAVE_AYATANA_APPINDICATOR
#include <libayatana-appindicator/app-indicator.h>
#endif

#define APP_NAME "laser-ptr"
#define SCREENSAVER_PATH "/org/freedesktop/ScreenSaver"
#define SCREENSAVER_IFACE "org.freedesktop.ScreenSaver"
#define SCREENSAVER_SERVICE "org.freedesktop.ScreenSaver"
#define GNOME_SERVICE "org.gnome.SessionManager"
#define GNOME_PATH "/org/gnome/SessionManager"
#define GNOME_IFACE "org.gnome.SessionManager"

typedef struct {
#ifdef HAVE_AYATANA_APPINDICATOR
    AppIndicator *indicator;
#endif
    guint timer_id;
    guint cookie;
    gboolean active;
    gboolean gnome_inhibitor;
} App;

static App app;
static GtkWidget *menu_toggle;
static GtkWidget *menu_status;
static GtkWidget *fallback_window;
static GtkWidget *fallback_status;
static GtkWidget *fallback_toggle;
static gboolean timer_expired(gpointer unused);
static void deactivate(void);

static gboolean set_inhibited(gboolean enable) {
    GError *error = NULL;
    GDBusConnection *bus = g_bus_get_sync(G_BUS_TYPE_SESSION, NULL, &error);
    if (!bus) {
        g_warning("Could not connect to desktop session bus: %s", error->message);
        g_clear_error(&error);
        return FALSE;
    }

    if (enable) {
        GVariant *reply = g_dbus_connection_call_sync(
            bus, SCREENSAVER_SERVICE, SCREENSAVER_PATH, SCREENSAVER_IFACE,
            "Inhibit", g_variant_new("(ss)", APP_NAME, "Keep the computer awake"),
            G_VARIANT_TYPE("(u)"), G_DBUS_CALL_FLAGS_NONE, 5000, NULL, &error);
        if (reply) {
            g_variant_get(reply, "(u)", &app.cookie);
            g_variant_unref(reply);
            app.gnome_inhibitor = FALSE;
        } else {
            g_clear_error(&error);
            reply = g_dbus_connection_call_sync(
                bus, GNOME_SERVICE, GNOME_PATH, GNOME_IFACE, "Inhibit",
                g_variant_new("(susu)", APP_NAME, 0u, "Keep the computer awake", 12u),
                G_VARIANT_TYPE("(u)"), G_DBUS_CALL_FLAGS_NONE, 5000, NULL, &error);
            if (!reply) {
                g_warning("Desktop did not grant sleep inhibition: %s", error->message);
                g_clear_error(&error);
                g_object_unref(bus);
                return FALSE;
            }
            g_variant_get(reply, "(u)", &app.cookie);
            g_variant_unref(reply);
            app.gnome_inhibitor = TRUE;
        }
    } else if (app.cookie) {
        GVariant *reply = g_dbus_connection_call_sync(
            bus, app.gnome_inhibitor ? GNOME_SERVICE : SCREENSAVER_SERVICE,
            app.gnome_inhibitor ? GNOME_PATH : SCREENSAVER_PATH,
            app.gnome_inhibitor ? GNOME_IFACE : SCREENSAVER_IFACE,
            app.gnome_inhibitor ? "Uninhibit" : "UnInhibit",
            g_variant_new("(u)", app.cookie), G_VARIANT_TYPE("()"),
            G_DBUS_CALL_FLAGS_NONE, 5000, NULL, &error);
        if (!reply) {
            g_warning("Could not release sleep inhibition: %s", error->message);
            g_clear_error(&error);
            g_object_unref(bus);
            return FALSE;
        }
        g_variant_unref(reply);
        app.cookie = 0;
        app.gnome_inhibitor = FALSE;
    }
    g_object_unref(bus);
    return TRUE;
}


static void refresh_ui(void) {
    const char *state = app.active ? "Awake mode is on" : "Awake mode is off";
    if (menu_toggle)
        gtk_menu_item_set_label(GTK_MENU_ITEM(menu_toggle), app.active ? "Turn Off" : "Keep Awake");
    if (menu_status)
        gtk_menu_item_set_label(GTK_MENU_ITEM(menu_status), state);
    if (fallback_status)
        gtk_label_set_text(GTK_LABEL(fallback_status), state);
    if (fallback_toggle)
        gtk_button_set_label(GTK_BUTTON(fallback_toggle), app.active ? "Turn Off" : "Keep Awake");
#ifdef HAVE_AYATANA_APPINDICATOR
    if (app.indicator) {
        const char *icon_name = app.active ? "media-playback-start" : "media-playback-pause";
        app_indicator_set_icon_full(app.indicator, icon_name, app.active ? "Awake mode on" : "Awake mode off");
        app_indicator_set_title(app.indicator, app.active ? "laser-ptr · On" : "laser-ptr · Off");
    }
#endif
}

static void deactivate(void) {
    if (!app.active) return;
    if (!set_inhibited(FALSE)) {
        if (!app.timer_id) app.timer_id = g_timeout_add_seconds(30, timer_expired, NULL);
        refresh_ui();
        return;
    }
    if (app.timer_id) {
        g_source_remove(app.timer_id);
        app.timer_id = 0;
    }
    app.active = FALSE;
    refresh_ui();
}

static gboolean timer_expired(gpointer unused) {
    (void)unused;
    app.timer_id = 0;
    deactivate();
    return G_SOURCE_REMOVE;
}

static void activate_for(guint seconds) {
    deactivate();
    if (app.active || !set_inhibited(TRUE)) return;
    app.active = TRUE;
    if (seconds) app.timer_id = g_timeout_add_seconds(seconds, timer_expired, NULL);
    refresh_ui();
}

static void menu_toggle_clicked(GtkMenuItem *item, gpointer unused) {
    (void)item; (void)unused;
    if (app.active) deactivate(); else activate_for(0);
}

static void menu_duration_clicked(GtkMenuItem *item, gpointer unused) {
    (void)unused;
    guint minutes = GPOINTER_TO_UINT(g_object_get_data(G_OBJECT(item), "minutes"));
    activate_for(minutes * 60);
}

static GtkWidget *duration_menu_item(const char *label, guint minutes) {
    GtkWidget *item = gtk_menu_item_new_with_label(label);
    g_object_set_data(G_OBJECT(item), "minutes", GUINT_TO_POINTER(minutes));
    g_signal_connect(item, "activate", G_CALLBACK(menu_duration_clicked), NULL);
    return item;
}

static void quit_app(void) {
    deactivate();
    gtk_main_quit();
}

static void menu_quit_clicked(GtkMenuItem *item, gpointer unused) {
    (void)item; (void)unused;
    quit_app();
}

static GtkWidget *make_menu(void) {
    GtkWidget *menu = gtk_menu_new();
    menu_toggle = gtk_menu_item_new_with_label("Keep Awake");
    g_signal_connect(menu_toggle, "activate", G_CALLBACK(menu_toggle_clicked), NULL);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_toggle);
    GtkWidget *duration = gtk_menu_item_new_with_label("Keep Awake For");
    GtkWidget *submenu = gtk_menu_new();
    gtk_menu_shell_append(GTK_MENU_SHELL(submenu), duration_menu_item("30 Minutes", 30));
    gtk_menu_shell_append(GTK_MENU_SHELL(submenu), duration_menu_item("1 Hour", 60));
    gtk_menu_shell_append(GTK_MENU_SHELL(submenu), duration_menu_item("2 Hours", 120));
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(duration), submenu);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), duration);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), gtk_separator_menu_item_new());
    menu_status = gtk_menu_item_new_with_label("Awake mode is off");
    gtk_widget_set_sensitive(menu_status, FALSE);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_status);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), gtk_separator_menu_item_new());
    GtkWidget *quit = gtk_menu_item_new_with_label("Quit laser-ptr");
    g_signal_connect(quit, "activate", G_CALLBACK(menu_quit_clicked), NULL);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), quit);
    gtk_widget_show_all(menu);
    return menu;
}

static void fallback_toggle_clicked(GtkButton *button, gpointer unused) {
    (void)button; (void)unused;
    if (app.active) deactivate(); else activate_for(0);
}

static void fallback_duration_clicked(GtkButton *button, gpointer data) {
    (void)button;
    activate_for(GPOINTER_TO_UINT(data) * 60);
}

static void fallback_quit_clicked(GtkButton *button, gpointer unused) {
    (void)button; (void)unused;
    quit_app();
}

static gboolean fallback_delete(GtkWidget *widget, GdkEvent *event, gpointer unused) {
    (void)widget; (void)event; (void)unused;
    quit_app();
    return TRUE;
}

static void add_duration_button(GtkWidget *box, const char *label, guint minutes) {
    GtkWidget *button = gtk_button_new_with_label(label);
    g_signal_connect(button, "clicked", G_CALLBACK(fallback_duration_clicked), GUINT_TO_POINTER(minutes));
    gtk_box_pack_start(GTK_BOX(box), button, FALSE, FALSE, 0);
}

static void show_fallback_window(void) {
    fallback_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(fallback_window), "laser-ptr");
    gtk_window_set_default_size(GTK_WINDOW(fallback_window), 260, 220);
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
    gtk_container_set_border_width(GTK_CONTAINER(box), 12);
    gtk_container_add(GTK_CONTAINER(fallback_window), box);
    GtkWidget *title = gtk_label_new("laser-ptr · Keep the computer awake");
    fallback_status = gtk_label_new("Awake mode is off");
    fallback_toggle = gtk_button_new_with_label("Keep Awake");
    g_signal_connect(fallback_toggle, "clicked", G_CALLBACK(fallback_toggle_clicked), NULL);
    gtk_box_pack_start(GTK_BOX(box), title, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(box), fallback_status, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(box), fallback_toggle, FALSE, FALSE, 0);
    add_duration_button(box, "30 Minutes", 30);
    add_duration_button(box, "1 Hour", 60);
    add_duration_button(box, "2 Hours", 120);
    GtkWidget *quit = gtk_button_new_with_label("Quit");
    g_signal_connect(quit, "clicked", G_CALLBACK(fallback_quit_clicked), NULL);
    gtk_box_pack_start(GTK_BOX(box), quit, FALSE, FALSE, 0);
    g_signal_connect(fallback_window, "delete-event", G_CALLBACK(fallback_delete), NULL);
    gtk_widget_show_all(fallback_window);
}

#ifdef HAVE_AYATANA_APPINDICATOR
static gboolean show_fallback_if_unconnected(gpointer unused) {
    (void)unused;
    gboolean connected = FALSE;
    g_object_get(app.indicator, "connected", &connected, NULL);
    if (!connected && !fallback_window) show_fallback_window();
    return G_SOURCE_REMOVE;
}

static void indicator_connection_changed(AppIndicator *indicator, gboolean connected, gpointer unused) {
    (void)indicator; (void)unused;
    if (!connected && !fallback_window) {
        show_fallback_window();
    } else if (connected && fallback_window) {
        gtk_widget_destroy(fallback_window);
        fallback_window = NULL;
        fallback_status = NULL;
        fallback_toggle = NULL;
    }
}
#endif

int main(int argc, char **argv) {
    gtk_init(&argc, &argv);
    GtkWidget *menu = make_menu();
#ifdef HAVE_AYATANA_APPINDICATOR
    app.indicator = app_indicator_new(APP_NAME, "media-playback-pause",
                                     APP_INDICATOR_CATEGORY_APPLICATION_STATUS);
    app_indicator_set_status(app.indicator, APP_INDICATOR_STATUS_ACTIVE);
    app_indicator_set_menu(app.indicator, GTK_MENU(menu));
    g_signal_connect(app.indicator, "connection-changed",
                     G_CALLBACK(indicator_connection_changed), NULL);
    g_timeout_add(1500, show_fallback_if_unconnected, NULL);
#else
    gtk_widget_destroy(menu);
    show_fallback_window();
#endif
    refresh_ui();
    gtk_main();
    return 0;
}
