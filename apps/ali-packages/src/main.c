#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ALI Packages 1.4.7 - apt face, C + GTK3. Search, details, install/remove
 * in a terminal (honest privileges), ALI family pinned, updates header.
 * --tr flag or ALI_LANG=tr for the chrome (package data stays upstream).
 */

static int LANG_TR = 0;
static const char *T(const char *en, const char *tr) { return LANG_TR ? tr : en; }

/* ALI shared look (1.3.4): /usr/share/ali/ali-style.css, silent fallback. */
static void ali_style(void) {
    GtkCssProvider *p = gtk_css_provider_new();
    if (gtk_css_provider_load_from_path(p, "/usr/share/ali/ali-style.css", NULL)) {
        gtk_style_context_add_provider_for_screen(gdk_screen_get_default(),
            GTK_STYLE_PROVIDER(p), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    }
    g_object_unref(p);
}

static GHashTable *installed = NULL;
static GtkWidget *result_list = NULL;
static GtkWidget *detail_label = NULL;
static GtkWidget *status_label = NULL;

static void launch_async(const char *cmd) {
    GError *err = NULL;
    if (!g_spawn_command_line_async(cmd, &err)) {
        if (err) g_error_free(err);
    }
}

static char *run_capture(const char *cmd) {
    FILE *p = popen(cmd, "r");
    if (!p) return NULL;
    static char buf[4096];
    size_t n = fread(buf, 1, sizeof(buf) - 1, p);
    pclose(p);
    buf[n] = '\0';
    return buf[0] ? buf : NULL;
}

static void load_installed(void) {
    installed = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, NULL);
    FILE *p = popen("dpkg-query -W -f='${Package}\\n' 2>/dev/null", "r");
    if (!p) return;
    char line[256];
    while (fgets(line, sizeof(line), p)) {
        char *nl = strchr(line, '\n');
        if (nl) *nl = '\0';
        if (line[0]) g_hash_table_add(installed, g_strdup(line));
    }
    pclose(p);
}

static int is_installed(const char *pkg) {
    return installed && g_hash_table_contains(installed, pkg);
}

static const char *family[][2] = {
    {"ali-center", "ALI Center - control panel"},
    {"ali-notepad", "ALI Notepad - tabbed editor"},
    {"ali-hymns", "ALI Hymns - chiptunes"},
    {"ali-ltask", "ALI Task Manager - PyQt6"},
    {"ali-welcome", "ALI Welcome - first-run wizard"},
    {"ali-shrine", "ALI Shrine - idle oracle"},
    {NULL, NULL},
};

static void clear_list(void) {
    GList *kids = gtk_container_get_children(GTK_CONTAINER(result_list));
    for (GList *c = kids; c; c = c->next) gtk_widget_destroy(GTK_WIDGET(c->data));
    g_list_free(kids);
}

static void on_row_clicked(GtkButton *b, gpointer u) {
    (void)u;
    char *pkg = (char *)g_object_get_data(G_OBJECT(b), "ali-pkg");
    if (!pkg) return;
    char cmd[512];
    snprintf(cmd, sizeof(cmd), "apt-cache show %s 2>/dev/null | head -12", pkg);
    char *out = run_capture(cmd);
    gtk_label_set_text(GTK_LABEL(detail_label), out ? out : pkg);
}

static void show_package(const char *pkg, const char *desc) {
    char row[512];
    snprintf(row, sizeof(row), "%s  %s  %s",
        is_installed(pkg) ? "[+]" : "[ ]", pkg, desc ? desc : "");
    GtkWidget *b = gtk_button_new_with_label(row);
    gtk_button_set_alignment(GTK_BUTTON(b), 0.0, 0.5);
    g_object_set_data_full(G_OBJECT(b), "ali-pkg", g_strdup(pkg), g_free);
    g_signal_connect(b, "clicked", G_CALLBACK(on_row_clicked), NULL);
    gtk_box_pack_start(GTK_BOX(result_list), b, FALSE, FALSE, 0);
    gtk_widget_show(b);
}

static void show_family(void) {
    clear_list();
    for (int i = 0; family[i][0]; i++)
        show_package(family[i][0], family[i][1]);
    gtk_label_set_text(GTK_LABEL(detail_label),
        T("The ALI family, pinned. Search above for the other 60,000.",
          "ALI ailesi, sabitli. Diğer 60.000 için yukarıda ara."));
}

static void on_search(GtkButton *b, gpointer u) {
    (void)b;
    const char *q = gtk_entry_get_text(GTK_ENTRY(u));
    if (!q || !*q) { show_family(); return; }
    /* names-only, capped: fast enough to feel instant */
    char cmd[512];
    snprintf(cmd, sizeof(cmd),
        "apt-cache search --names-only '%s' 2>/dev/null | head -100", q);
    FILE *p = popen(cmd, "r");
    if (!p) return;
    clear_list();
    char line[512];
    int n = 0;
    while (fgets(line, sizeof(line), p)) {
        char *sep = strstr(line, " - ");
        if (!sep) continue;
        *sep = '\0';
        char *nl = strchr(sep + 3, '\n');
        if (nl) *nl = '\0';
        if (!line[0]) continue;
        show_package(line, sep + 3);
        if (++n >= 100) break;
    }
    pclose(p);
    char st[128];
    snprintf(st, sizeof(st), "%s: %d", T("Found", "Bulundu"), n);
    gtk_label_set_text(GTK_LABEL(status_label), st);
}

static void on_install(GtkButton *b, gpointer u) {
    (void)b; (void)u;
    GtkWidget *d = gtk_dialog_new_with_buttons(T("Install", "Kur"),
        NULL, GTK_DIALOG_MODAL, T("_Cancel", "_Vazgeç"), GTK_RESPONSE_CANCEL,
        T("_Install", "_Kur"), GTK_RESPONSE_ACCEPT, NULL);
    GtkWidget *e = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(e), T("package name", "paket adı"));
    gtk_container_add(GTK_CONTAINER(gtk_dialog_get_content_area(GTK_DIALOG(d))), e);
    gtk_widget_show_all(d);
    if (gtk_dialog_run(GTK_DIALOG(d)) == GTK_RESPONSE_ACCEPT) {
        const char *pkg = gtk_entry_get_text(GTK_ENTRY(e));
        if (pkg && *pkg && strchr(pkg, ' ') == NULL && strchr(pkg, ';') == NULL) {
            char cmd[384];
            snprintf(cmd, sizeof(cmd),
                "x-terminal-emulator -e 'sudo apt install -y %s; echo; read -n1 -p \"done\"'", pkg);
            launch_async(cmd);
        }
    }
    gtk_widget_destroy(d);
}

static void on_remove(GtkButton *b, gpointer u) {
    (void)b; (void)u;
    GtkWidget *d = gtk_dialog_new_with_buttons(T("Remove", "Kaldır"),
        NULL, GTK_DIALOG_MODAL, T("_Cancel", "_Vazgeç"), GTK_RESPONSE_CANCEL,
        T("_Remove", "_Kaldır"), GTK_RESPONSE_ACCEPT, NULL);
    GtkWidget *e = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(e), T("package name", "paket adı"));
    gtk_container_add(GTK_CONTAINER(gtk_dialog_get_content_area(GTK_DIALOG(d))), e);
    gtk_widget_show_all(d);
    if (gtk_dialog_run(GTK_DIALOG(d)) == GTK_RESPONSE_ACCEPT) {
        const char *pkg = gtk_entry_get_text(GTK_ENTRY(e));
        if (pkg && *pkg && strchr(pkg, ' ') == NULL && strchr(pkg, ';') == NULL) {
            char cmd[384];
            snprintf(cmd, sizeof(cmd),
                "x-terminal-emulator -e 'sudo apt remove -y %s; echo; read -n1 -p \"done\"'", pkg);
            launch_async(cmd);
        }
    }
    gtk_widget_destroy(d);
}

static void on_upgrade(GtkButton *b, gpointer u) {
    (void)b; (void)u;
    launch_async("x-terminal-emulator -e 'sudo apt update && sudo apt full-upgrade; echo DONE - press Enter; read x'");
}

int main(int argc, char **argv) {
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--tr") == 0) { LANG_TR = 1; break; }
    }
    if (!LANG_TR) {
        const char *env = getenv("ALI_LANG");
        if (env && (strcmp(env, "tr") == 0 || strcmp(env, "TR") == 0)) LANG_TR = 1;
    }
    gtk_init(&argc, &argv);
    ali_style();
    load_installed();

    GtkWidget *win = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(win), T("ALI Packages", "ALI Paketler"));
    gtk_window_set_icon_name(GTK_WINDOW(win), "system-software-install");
    gtk_window_set_default_size(GTK_WINDOW(win), 720, 520);
    gtk_window_set_position(GTK_WINDOW(win), GTK_WIN_POS_CENTER);
    g_signal_connect(win, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
    gtk_container_set_border_width(GTK_CONTAINER(box), 12);
    gtk_container_add(GTK_CONTAINER(win), box);

    /* updates header */
    {
        char *n = run_capture("cat /run/ali-updates 2>/dev/null");
        char hdr[192];
        snprintf(hdr, sizeof(hdr), "%s: %s",
            T("Pending updates", "Bekleyen güncellemeler"), (n && *n) ? n : "?");
        GtkWidget *hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
        GtkWidget *hl = gtk_label_new(hdr);
        gtk_box_pack_start(GTK_BOX(hbox), hl, TRUE, TRUE, 0);
        GtkWidget *hb = gtk_button_new_with_label(T("Upgrade Now", "Şimdi Yükselt"));
        g_signal_connect(hb, "clicked", G_CALLBACK(on_upgrade), NULL);
        gtk_box_pack_start(GTK_BOX(hbox), hb, FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(box), hbox, FALSE, FALSE, 0);
    }

    /* search row */
    GtkWidget *srow = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    GtkWidget *search = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(search),
        T("Search packages… (empty = ALI family)", "Paket ara… (boş = ALI ailesi)"));
    g_signal_connect(search, "activate", G_CALLBACK(on_search), search);
    gtk_box_pack_start(GTK_BOX(srow), search, TRUE, TRUE, 0);
    GtkWidget *sb = gtk_button_new_with_label(T("Search", "Ara"));
    g_signal_connect(sb, "clicked", G_CALLBACK(on_search), search);
    gtk_box_pack_start(GTK_BOX(srow), sb, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(box), srow, FALSE, FALSE, 0);

    /* results */
    GtkWidget *scroll = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll),
        GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    result_list = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
    gtk_container_add(GTK_CONTAINER(scroll), result_list);
    gtk_box_pack_start(GTK_BOX(box), scroll, TRUE, TRUE, 0);

    /* details */
    detail_label = gtk_label_new("");
    gtk_label_set_selectable(GTK_LABEL(detail_label), TRUE);
    gtk_label_set_xalign(GTK_LABEL(detail_label), 0.0);
    gtk_box_pack_start(GTK_BOX(box), detail_label, FALSE, FALSE, 0);

    /* actions */
    GtkWidget *arow = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    GtkWidget *bi = gtk_button_new_with_label(T("Install…", "Kur…"));
    g_signal_connect(bi, "clicked", G_CALLBACK(on_install), NULL);
    gtk_box_pack_start(GTK_BOX(arow), bi, TRUE, TRUE, 0);
    GtkWidget *br = gtk_button_new_with_label(T("Remove…", "Kaldır…"));
    g_signal_connect(br, "clicked", G_CALLBACK(on_remove), NULL);
    gtk_box_pack_start(GTK_BOX(arow), br, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(box), arow, FALSE, FALSE, 0);

    status_label = gtk_label_new("");
    gtk_box_pack_start(GTK_BOX(box), status_label, FALSE, FALSE, 0);

    show_family();

    gtk_widget_show_all(win);
    gtk_main();
    return 0;
}
