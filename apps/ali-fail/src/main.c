#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ali-fail 1.5.0 - the honest blue screen. Usage: ali-fail <unit>
 * Fullscreen #0000AA, names the fallen unit, shows its last log lines,
 * offers Restart / Log / Continue-without (acknowledged, stops the loop).
 * Odysseus gets the special truth: without it, this is just Debian 13.
 */

static const char *unit = "odysseus";
static char logbuf[2048];

static const char *logfile_for(const char *u) {
    if (strcmp(u, "odysseus") == 0) return "/var/log/odysseus.log";
    if (strcmp(u, "sentinel") == 0) return "/var/log/sentinel.log";
    if (strcmp(u, "terrydavis") == 0) return "/var/log/terrydavis.log";
    if (strcmp(u, "templeos") == 0) return "/var/log/templeos.log";
    if (strcmp(u, "oracle") == 0) return "/var/log/oracle.log";
    if (strcmp(u, "abigfanterrydavis") == 0) return "/var/log/fan.log";
    if (strcmp(u, "linustorvalds") == 0) return "/var/log/linus.log";
    return NULL;
}

static void load_log(void) {
    logbuf[0] = '\0';
    const char *lf = logfile_for(unit);
    char cmd[512];
    if (lf)
        snprintf(cmd, sizeof(cmd), "tail -n 8 '%s' 2>/dev/null", lf);
    else
        snprintf(cmd, sizeof(cmd), "journalctl -u '%s' -n 8 --no-pager 2>/dev/null", unit);
    FILE *p = popen(cmd, "r");
    if (!p) return;
    size_t n = fread(logbuf, 1, sizeof(logbuf) - 1, p);
    pclose(p);
    logbuf[n] = '\0';
    if (!logbuf[0])
        snprintf(logbuf, sizeof(logbuf), "(no log lines captured)");
}

static void on_restart(GtkButton *b, gpointer w) {
    (void)b;
    char cmd[256];
    snprintf(cmd, sizeof(cmd),
        "sudo systemctl reset-failed '%s' 2>/dev/null; sudo systemctl restart '%s'",
        unit, unit);
    int rc = system(cmd);
    (void)rc;
    sleep(2);
    /* still dead? stay. alive? leave. */
    snprintf(cmd, sizeof(cmd),
        "systemctl is-active --quiet '%s'", unit);
    if (system(cmd) == 0)
        gtk_main_quit();
    else {
        GtkWidget *d = gtk_message_dialog_new(GTK_WINDOW(w), GTK_DIALOG_MODAL,
            GTK_MESSAGE_WARNING, GTK_BUTTONS_CLOSE,
            "Still dead. Check the log, then decide.");
        gtk_dialog_run(GTK_DIALOG(d));
        gtk_widget_destroy(d);
    }
}

static void on_log(GtkButton *b, gpointer u) {
    (void)b; (void)u;
    const char *lf = logfile_for(unit);
    char cmd[512];
    if (lf)
        snprintf(cmd, sizeof(cmd),
            "x-terminal-emulator -e 'tail -n 50 \"%s\"; echo; read -n1 -p \"press any key\"'", lf);
    else
        snprintf(cmd, sizeof(cmd),
            "x-terminal-emulator -e 'journalctl -u \"%s\" -n 50 --no-pager; echo; read -n1 -p \"press any key\"'", unit);
    GError *err = NULL;
    if (!g_spawn_command_line_async(cmd, &err)) {
        if (err) g_error_free(err);
    }
}

static void on_continue(GtkButton *b, gpointer u) {
    (void)b; (void)u;
    char flag[256], cmd[384];
    snprintf(flag, sizeof(flag), "/run/%s-acknowledged", unit);
    snprintf(cmd, sizeof(cmd),
        "sudo systemctl stop '%s' 2>/dev/null; sudo touch '%s' 2>/dev/null || touch '%s' 2>/dev/null",
        unit, flag, flag);
    int rc = system(cmd);
    (void)rc;
    gtk_main_quit();
}

int main(int argc, char **argv) {
    if (argc > 1 && argv[1][0]) unit = argv[1];
    gtk_init(&argc, &argv);
    load_log();

    int is_odysseus = strcmp(unit, "odysseus") == 0;

    GtkWidget *win = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    char title[128];
    snprintf(title, sizeof(title), "%s FAILED", unit);
    for (char *p = title; *p; p++) *p = (*p >= 'a' && *p <= 'z') ? *p - 32 : *p;
    gtk_window_set_title(GTK_WINDOW(win), title);
    gtk_window_fullscreen(GTK_WINDOW(win));
    g_signal_connect(win, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    GtkCssProvider *css = gtk_css_provider_new();
    gtk_css_provider_load_from_data(css,
        "window { background-color: #0000AA; }"
        "window label { color: #ffffff; }", -1, NULL);
    gtk_style_context_add_provider_for_screen(gdk_screen_get_default(),
        GTK_STYLE_PROVIDER(css), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);

    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 16);
    gtk_container_set_border_width(GTK_CONTAINER(box), 60);
    gtk_container_add(GTK_CONTAINER(win), box);

    GtkWidget *head = gtk_label_new(NULL);
    char hbuf[256];
    snprintf(hbuf, sizeof(hbuf), "<b><big><big>%s</big></big></b>", title);
    gtk_label_set_markup(GTK_LABEL(head), hbuf);
    gtk_box_pack_start(GTK_BOX(box), head, FALSE, FALSE, 0);

    GtkWidget *msg = gtk_label_new(
        is_odysseus
        ? "Without Odysseus, this machine is just Debian 13.\nEnforcer, watchdog, sea chart, memory - all stop with it."
        : "This ALI daemon keeps dying.\nThe rest of the system carries on without it.");
    gtk_box_pack_start(GTK_BOX(box), msg, FALSE, FALSE, 0);

    GtkWidget *lg = gtk_label_new(logbuf);
    gtk_label_set_selectable(GTK_LABEL(lg), TRUE);
    gtk_label_set_xalign(GTK_LABEL(lg), 0.0);
    gtk_box_pack_start(GTK_BOX(box), lg, FALSE, FALSE, 0);

    GtkWidget *row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
    GtkWidget *b1 = gtk_button_new_with_label("Restart It");
    g_signal_connect(b1, "clicked", G_CALLBACK(on_restart), win);
    gtk_box_pack_start(GTK_BOX(row), b1, TRUE, TRUE, 0);
    GtkWidget *b2 = gtk_button_new_with_label("View Full Log");
    g_signal_connect(b2, "clicked", G_CALLBACK(on_log), NULL);
    gtk_box_pack_start(GTK_BOX(row), b2, TRUE, TRUE, 0);
    GtkWidget *b3 = gtk_button_new_with_label("Continue Without It");
    g_signal_connect(b3, "clicked", G_CALLBACK(on_continue), NULL);
    gtk_box_pack_start(GTK_BOX(row), b3, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(box), row, FALSE, FALSE, 0);

    gtk_widget_show_all(win);
    gtk_main();
    return 0;
}
