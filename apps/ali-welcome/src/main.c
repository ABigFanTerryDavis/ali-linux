#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ALI Welcome 1.3.0 - first-run wizard, C + GTK3, bilingual by layout
 * (Turkish + English side by side, no flag needed on day one).
 * Unchecking "show on login" writes Hidden=true over its own autostart copy.
 */

static void launch_async(const char *cmd) {
    GError *err = NULL;
    if (!g_spawn_command_line_async(cmd, &err)) {
        if (err) g_error_free(err);
    }
}

static void on_wifi(GtkButton *b, gpointer u) {
    (void)b; (void)u;
    launch_async("nm-connection-editor 2>/dev/null || exo-open --launch Network 2>/dev/null || true");
}

static void on_updates(GtkButton *b, gpointer u) {
    (void)b; (void)u;
    launch_async("x-terminal-emulator -e 'sudo apt update && sudo apt full-upgrade; echo DONE - press Enter; read x'");
}

static void on_center(GtkButton *b, gpointer u) {
    (void)b; (void)u;
    launch_async("ali-center");
}

static void on_tour(GtkButton *b, gpointer u) {
    (void)b; (void)u;
    launch_async("exo-open /usr/share/ali/welcome.html 2>/dev/null || firefox-esr /usr/share/ali/welcome.html 2>/dev/null || true");
}

static void on_whatsnew(GtkButton *b, gpointer u) {
    (void)b; (void)u;
    launch_async("exo-open /usr/share/ali/welcome.html#whats-new 2>/dev/null || firefox-esr /usr/share/ali/welcome.html 2>/dev/null || true");
}

static void on_login_toggle(GtkToggleButton *t, gpointer u) {
    (void)u;
    gboolean show = gtk_toggle_button_get_active(t);
    char path[512];
    snprintf(path, sizeof(path), "%s/.config/autostart/ali-welcome.desktop", g_get_home_dir());
    FILE *f = fopen(path, "w");
    if (!f) return;
    fprintf(f, "[Desktop Entry]\nType=Application\nName=ALI Welcome\nExec=ali-welcome\nHidden=%s\n",
        show ? "false" : "true");
    fclose(f);
}

/* ALI shared look (1.3.4): /usr/share/ali/ali-style.css, silent fallback. */
static void ali_style(void) {
    GtkCssProvider *p = gtk_css_provider_new();
    if (gtk_css_provider_load_from_path(p, "/usr/share/ali/ali-style.css", NULL)) {
        gtk_style_context_add_provider_for_screen(gdk_screen_get_default(),
            GTK_STYLE_PROVIDER(p), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    }
    g_object_unref(p);
}

static GtkWidget *big_button(const char *label, GCallback cb) {
    GtkWidget *b = gtk_button_new_with_label(label);
    gtk_widget_set_size_request(b, -1, 44);
    g_signal_connect(b, "clicked", cb, NULL);
    return b;
}

int main(int argc, char **argv) {
    gtk_init(&argc, &argv);
    ali_style();

    GtkWidget *win = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(win), "ALI Linux'a hoş geldin - Welcome to ALI Linux");
    gtk_window_set_icon_name(GTK_WINDOW(win), "ali-welcome");
    gtk_window_set_default_size(GTK_WINDOW(win), 460, 420);
    gtk_window_set_position(GTK_WINDOW(win), GTK_WIN_POS_CENTER);
    g_signal_connect(win, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_container_set_border_width(GTK_CONTAINER(box), 20);
    gtk_container_add(GTK_CONTAINER(win), box);

    if (g_file_test("/usr/share/icons/hicolor/scalable/apps/ali-logo.svg", G_FILE_TEST_EXISTS)) {
        GtkWidget *logo = gtk_image_new_from_file("/usr/share/icons/hicolor/scalable/apps/ali-logo.svg");
        gtk_image_set_pixel_size(GTK_IMAGE(logo), 64);
        gtk_box_pack_start(GTK_BOX(box), logo, FALSE, FALSE, 0);
    }

    GtkWidget *head = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(head),
        "<b><big>ALI Linux'a hoş geldin</big></b>\n<b><big>Welcome to ALI Linux</big></b>\nXFCE - hızlı, Türkçe, senin. / fast, Turkish, yours.");
    gtk_box_pack_start(GTK_BOX(box), head, FALSE, FALSE, 0);

    gtk_box_pack_start(GTK_BOX(box), big_button("WiFi Ayarları / WiFi Settings", G_CALLBACK(on_wifi)), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(box), big_button("Güncellemeler / Updates", G_CALLBACK(on_updates)), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(box), big_button("ALI Center (Denetim Masası / Control Panel)", G_CALLBACK(on_center)), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(box), big_button("Tur / Tour", G_CALLBACK(on_tour)), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(box), big_button("Yenilikler / What's New", G_CALLBACK(on_whatsnew)), FALSE, FALSE, 0);

    GtkWidget *chk = gtk_check_button_new_with_label("Girişte göster / Show on login");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(chk), TRUE);
    g_signal_connect(chk, "toggled", G_CALLBACK(on_login_toggle), NULL);
    gtk_box_pack_start(GTK_BOX(box), chk, FALSE, FALSE, 8);

    GtkWidget *foot = gtk_label_new("Klavye / Keyboard: Alt+Shift ile TR-EN değişir.\nGüvenlik: Sentinel izler, Odysseus bekler. / Sentinel watches, Odysseus waits.");
    gtk_box_pack_start(GTK_BOX(box), foot, FALSE, FALSE, 0);

    gtk_widget_show_all(win);
    gtk_main();
    return 0;
}
