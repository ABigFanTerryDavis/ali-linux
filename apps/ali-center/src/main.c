#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ALI Center v1 - separate app, C + GTK3, XFCE native.
 * Visible-layer control panel only. No system file writes in v1
 * except launching helpers (terminal, installer, file manager).
 */

static void launch_async(const char *cmd) {
    GError *err = NULL;
    if (!g_spawn_command_line_async(cmd, &err)) {
        GtkWidget *d = gtk_message_dialog_new(NULL, GTK_DIALOG_MODAL,
            GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE,
            "Could not launch:\n%s\n%s", cmd, err ? err->message : "");
        gtk_dialog_run(GTK_DIALOG(d));
        gtk_widget_destroy(d);
        if (err) g_error_free(err);
    }
}

static void read_os_pretty(char *out, size_t n) {
    FILE *f = fopen("/etc/os-release", "r");
    snprintf(out, n, "ALI Linux 1.0.6");
    if (!f) return;
    char line[256];
    while (fgets(line, sizeof(line), f)) {
        if (strncmp(line, "PRETTY_NAME=", 12) == 0) {
            char *p = strchr(line, '"');
            char *q = p ? strchr(p + 1, '"') : NULL;
            if (p && q && (size_t)(q - p - 1) < n) {
                memcpy(out, p + 1, (size_t)(q - p - 1));
                out[q - p - 1] = '\0';
            }
            break;
        }
    }
    fclose(f);
}

/* --- callbacks --- */
static void on_update_clicked(GtkButton *b, gpointer u) {
    (void)b; (void)u;
    launch_async("x-terminal-emulator -e 'sudo apt update && sudo apt full-upgrade; echo DONE - press Enter; read x'");
}

static void on_sysinfo_clicked(GtkButton *b, gpointer u) {
    (void)b; (void)u;
    launch_async("x-terminal-emulator -e 'fastfetch 2>/dev/null || neofetch 2>/dev/null || (cat /etc/os-release; uname -a); echo; read -n1 -p \"press any key\"'");
}

static void on_wallpapers_clicked(GtkButton *b, gpointer u) {
    (void)b; (void)u;
    launch_async("thunar /usr/share/backgrounds/ali 2>/dev/null || exo-open /usr/share/backgrounds/ali");
}

static void on_terminal_clicked(GtkButton *b, gpointer u) {
    (void)b; (void)u;
    launch_async("xfce4-terminal --title='ALI Terminal'");
}

static void on_files_clicked(GtkButton *b, gpointer u) {
    (void)b; (void)u;
    launch_async("thunar");
}

static void on_browser_clicked(GtkButton *b, gpointer u) {
    (void)b; (void)u;
    launch_async("firefox-esr 2>/dev/null || firefox 2>/dev/null || exo-open https://github.com/");
}

static void on_install_clicked(GtkButton *b, gpointer u) {
    (void)b; (void)u;
    /* trixie d-i live uses debian-installer-launcher */
    int rc = system("command -v debian-installer-launcher >/dev/null 2>&1");
    if (rc == 0) { launch_async("debian-installer-launcher"); return; }
    rc = system("command -v live-installer >/dev/null 2>&1");
    if (rc == 0) { launch_async("live-installer"); return; }
    GtkWidget *d = gtk_message_dialog_new(NULL, GTK_DIALOG_MODAL,
        GTK_MESSAGE_INFO, GTK_BUTTONS_CLOSE,
        "Installer not found in live session.\nLook for 'ALI Installer' on the desktop.");
    gtk_dialog_run(GTK_DIALOG(d));
    gtk_widget_destroy(d);
}

static GtkWidget *tab_label_page(const char *title) {
    return gtk_label_new(title);
}

int main(int argc, char **argv) {
    gtk_init(&argc, &argv);

    GtkWidget *win = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(win), "ALI Center");
    gtk_window_set_default_size(GTK_WINDOW(win), 640, 440);
    gtk_window_set_position(GTK_WINDOW(win), GTK_WIN_POS_CENTER);
    g_signal_connect(win, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    GtkWidget *nb = gtk_notebook_new();
    gtk_container_add(GTK_CONTAINER(win), nb);

    /* --- System tab --- */
    {
        GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
        gtk_container_set_border_width(GTK_CONTAINER(box), 16);
        char pretty[128]; read_os_pretty(pretty, sizeof(pretty));
        char buf[256];
        snprintf(buf, sizeof(buf), "<b>%s</b>\nALI Linux - XFCE - amd64", pretty);
        GtkWidget *l = gtk_label_new(NULL);
        gtk_label_set_markup(GTK_LABEL(l), buf);
        gtk_box_pack_start(GTK_BOX(box), l, FALSE, FALSE, 0);

        GtkWidget *b1 = gtk_button_new_with_label("System Info (fastfetch)");
        g_signal_connect(b1, "clicked", G_CALLBACK(on_sysinfo_clicked), NULL);
        gtk_box_pack_start(GTK_BOX(box), b1, FALSE, FALSE, 0);

        GtkWidget *b2 = gtk_button_new_with_label("Check for Updates (apt)");
        g_signal_connect(b2, "clicked", G_CALLBACK(on_update_clicked), NULL);
        gtk_box_pack_start(GTK_BOX(box), b2, FALSE, FALSE, 0);

        GtkWidget *hint = gtk_label_new("Keyboard: Turkish (tr) + English supported.\nSwitch with XFCE Panel -> Keyboard applet.");
        gtk_box_pack_start(GTK_BOX(box), hint, FALSE, FALSE, 8);

        gtk_notebook_append_page(GTK_NOTEBOOK(nb), box, tab_label_page("System"));
    }

    /* --- Appearance tab --- */
    {
        GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
        gtk_container_set_border_width(GTK_CONTAINER(box), 16);
        GtkWidget *l = gtk_label_new("Theme: Greybird - Icons: elementary-xfce\nWallpapers: /usr/share/backgrounds/ali/");
        gtk_box_pack_start(GTK_BOX(box), l, FALSE, FALSE, 0);
        GtkWidget *b = gtk_button_new_with_label("Open Wallpapers Folder");
        g_signal_connect(b, "clicked", G_CALLBACK(on_wallpapers_clicked), NULL);
        gtk_box_pack_start(GTK_BOX(box), b, FALSE, FALSE, 0);
        GtkWidget *h = gtk_label_new("Tip: right-click Desktop -> Desktop Settings to change wallpaper.");
        gtk_box_pack_start(GTK_BOX(box), h, FALSE, FALSE, 8);
        gtk_notebook_append_page(GTK_NOTEBOOK(nb), box, tab_label_page("Appearance"));
    }

    /* --- Apps tab --- */
    {
        GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
        gtk_container_set_border_width(GTK_CONTAINER(box), 16);
        GtkWidget *b1 = gtk_button_new_with_label("Open ALI Terminal");
        g_signal_connect(b1, "clicked", G_CALLBACK(on_terminal_clicked), NULL);
        gtk_box_pack_start(GTK_BOX(box), b1, FALSE, FALSE, 0);
        GtkWidget *b2 = gtk_button_new_with_label("Open Files");
        g_signal_connect(b2, "clicked", G_CALLBACK(on_files_clicked), NULL);
        gtk_box_pack_start(GTK_BOX(box), b2, FALSE, FALSE, 0);
        GtkWidget *b3 = gtk_button_new_with_label("Open Browser");
        g_signal_connect(b3, "clicked", G_CALLBACK(on_browser_clicked), NULL);
        gtk_box_pack_start(GTK_BOX(box), b3, FALSE, FALSE, 0);
        gtk_notebook_append_page(GTK_NOTEBOOK(nb), box, tab_label_page("Apps"));
    }

    /* --- Install tab --- */
    {
        GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
        gtk_container_set_border_width(GTK_CONTAINER(box), 16);
        GtkWidget *l = gtk_label_new("Try live, then install to disk when ready.");
        gtk_box_pack_start(GTK_BOX(box), l, FALSE, FALSE, 0);
        GtkWidget *b = gtk_button_new_with_label("Install ALI to Disk");
        gtk_widget_set_size_request(b, -1, 48);
        g_signal_connect(b, "clicked", G_CALLBACK(on_install_clicked), NULL);
        gtk_box_pack_start(GTK_BOX(box), b, FALSE, FALSE, 0);
        gtk_notebook_append_page(GTK_NOTEBOOK(nb), box, tab_label_page("Install"));
    }

    /* --- About tab --- */
    {
        GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
        gtk_container_set_border_width(GTK_CONTAINER(box), 16);
        GtkWidget *l = gtk_label_new("ALI Linux 1.0.6\nXFCE - amd64.\n\nALI Center v1 - C + GTK3.");
        gtk_box_pack_start(GTK_BOX(box), l, FALSE, FALSE, 0);
        gtk_notebook_append_page(GTK_NOTEBOOK(nb), box, tab_label_page("About"));
    }

    gtk_widget_show_all(win);
    gtk_main();
    return 0;
}
