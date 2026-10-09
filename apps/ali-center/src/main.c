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

/* Turkish pass (1.2.5): --tr flag or ALI_LANG=tr. Proper nouns stay. */
static int LANG_TR = 0;
static const char *T(const char *en, const char *tr) { return LANG_TR ? tr : en; }
static char *run_capture(const char *cmd);

static void read_os_pretty(char *out, size_t n) {
    FILE *f = fopen("/etc/os-release", "r");
    snprintf(out, n, "ALI Linux 1.4.8");
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

static void on_perf_clicked(GtkButton *b, gpointer u) {
    (void)b; (void)u;
    launch_async("x-terminal-emulator -e 'sudo ali-perf; echo; read -n1 -p \"press any key\"'");
}

static void on_normal_clicked(GtkButton *b, gpointer u) {
    (void)b; (void)u;
    launch_async("x-terminal-emulator -e 'sudo ali-normal; echo; read -n1 -p \"press any key\"'");
}

static void on_mirror_clicked(GtkButton *b, gpointer u) {
    (void)b; (void)u;
    launch_async("x-terminal-emulator -e 'sudo ali-mirrors; echo; read -n1 -p \"press any key\"'");
}

static void on_sysinfo_clicked(GtkButton *b, gpointer u) {
    (void)b; (void)u;
    launch_async("x-terminal-emulator -e 'fastfetch 2>/dev/null || neofetch 2>/dev/null || (cat /etc/os-release; uname -a); echo; read -n1 -p \"press any key\"'");
}

static void on_updates_refresh(GtkButton *b, gpointer u) {
    (void)b;
    char *n = run_capture("cat /run/ali-updates 2>/dev/null");
    char *day = run_capture("cat /run/odysseus-apt-day 2>/dev/null");
    char buf[256];
    snprintf(buf, sizeof(buf), "%s: %s\n%s: %s",
        T("Pending updates", "Bekleyen güncellemeler"), (n && *n) ? n : "?",
        T("Last checked", "Son denetim"), (day && *day) ? day : T("never", "hiç"));
    gtk_label_set_text(GTK_LABEL(u), buf);
}

static void on_updates_list(GtkButton *b, gpointer u) {
    (void)b; (void)u;
    launch_async("x-terminal-emulator -e 'apt list --upgradable 2>/dev/null | head -30; echo; read -n1 -p \"press any key\"'");
}

static void on_upgrade_clicked(GtkButton *b, gpointer u) {
    (void)b; (void)u;
    launch_async("x-terminal-emulator -e 'sudo apt update && sudo apt full-upgrade; echo DONE - press Enter; read x'");
}

static void on_wallpapers_clicked(GtkButton *b, gpointer u) {
    (void)b; (void)u;
    launch_async("thunar /usr/share/backgrounds/ali 2>/dev/null || exo-open /usr/share/backgrounds/ali");
}

static void on_night_clicked(GtkButton *b, gpointer u) {
    const char *mode = (const char *)u;
    char cmd[256];
    if (strcmp(mode, "auto") == 0)
        snprintf(cmd, sizeof(cmd), "sudo rm -f /var/lib/terrydavis/night.force");
    else
        snprintf(cmd, sizeof(cmd), "echo %s | sudo tee /var/lib/terrydavis/night.force >/dev/null", mode);
    int rc = system(cmd);
    (void)rc;
    launch_async("x-terminal-emulator -e 'sleep 16; echo night updated; read -n1 -p \"press any key\"'");
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

static void on_shot_clicked(GtkButton *b, gpointer u) {
    (void)b; (void)u;
    launch_async("xfce4-screenshooter 2>/dev/null || scrot 2>/dev/null || true");
}

static void on_transparent_clicked(GtkButton *b, gpointer u) {
    const char *mode = (const char *)u;
    char path[512];
    snprintf(path, sizeof(path), "%s/.config/xfce4/terminal/terminalrc", g_get_home_dir());
    (void)b;
    if (strcmp(mode, "on") == 0) {
        char cmd[640];
        snprintf(cmd, sizeof(cmd),
            "grep -q '^BackgroundMode=' '%s' 2>/dev/null && sed -i 's/^BackgroundMode=.*/BackgroundMode=TERMINAL_BACKGROUND_TRANSPARENT/' '%s' || echo 'BackgroundMode=TERMINAL_BACKGROUND_TRANSPARENT' >> '%s'; "
            "grep -q '^BackgroundDarkness=' '%s' 2>/dev/null && sed -i 's/^BackgroundDarkness=.*/BackgroundDarkness=0.85/' '%s' || echo 'BackgroundDarkness=0.85' >> '%s'",
            path, path, path, path, path, path);
        int rc = system(cmd);
        (void)rc;
    } else {
        char cmd[320];
        snprintf(cmd, sizeof(cmd),
            "sed -i 's/^BackgroundMode=.*/BackgroundMode=TERMINAL_BACKGROUND_SOLID/' '%s' 2>/dev/null || true", path);
        int rc = system(cmd);
        (void)rc;
    }
    GtkWidget *d = gtk_message_dialog_new(NULL, GTK_DIALOG_MODAL,
        GTK_MESSAGE_INFO, GTK_BUTTONS_CLOSE, "%s",
        T("Applies to terminals you open next.", "Sonra açacağın uçbirimlerde geçerli."));
    gtk_dialog_run(GTK_DIALOG(d));
    gtk_widget_destroy(d);
}

static void on_disk_clicked(GtkButton *b, gpointer u) {
    (void)b; (void)u;
    launch_async("x-terminal-emulator -e 'ncdu / 2>/dev/null || du -sh /* 2>/dev/null; echo; read -n1 -p \"press any key\"'");
}

static char game_buf[256];
static const char *game_status_text(void) {
    char *mode = run_capture("cat /var/lib/terrydavis/gamemode 2>/dev/null");
    char *force = run_capture("cat /var/lib/terrydavis/gamemode.force 2>/dev/null");
    snprintf(game_buf, sizeof(game_buf), "%s: %s%s%s",
        T("Game mode", "Oyun modu"),
        (mode && strcmp(mode, "on") == 0) ? T("ON", "AÇIK") : T("off", "kapalı"),
        (force && *force) ? " (" : "",
        (force && *force) ? (strcmp(force, "on") == 0 ? T("forced)", "zorla)") : T("held off)", "tutuluyor)")) : "");
    return game_buf;
}

static void on_game_clicked(GtkButton *b, gpointer u) {
    (void)b;
    /* cycle: auto -> force on -> force off -> auto */
    char *cur = run_capture("cat /var/lib/terrydavis/gamemode.force 2>/dev/null");
    const char *next = "on";
    const char *msg = NULL;
    if (!cur || !*cur) { next = "on"; msg = T("Game mode: forced ON", "Oyun modu: zorla AÇIK"); }
    else if (strcmp(cur, "on") == 0) { next = "off"; msg = T("Game mode: forced OFF", "Oyun modu: zorla KAPALI"); }
    else { next = ""; msg = T("Game mode: auto", "Oyun modu: otomatik"); }
    char cmd[256];
    if (*next)
        snprintf(cmd, sizeof(cmd), "echo %s | sudo tee /var/lib/terrydavis/gamemode.force >/dev/null", next);
    else
        snprintf(cmd, sizeof(cmd), "sudo rm -f /var/lib/terrydavis/gamemode.force");
    int rc = system(cmd);
    (void)rc;
    GtkWidget *d = gtk_message_dialog_new(NULL, GTK_DIALOG_MODAL,
        GTK_MESSAGE_INFO, GTK_BUTTONS_CLOSE, "%s", msg);
    gtk_dialog_run(GTK_DIALOG(d));
    gtk_widget_destroy(d);
    gtk_label_set_text(GTK_LABEL(u), game_status_text());
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
        "%s", T("Installer not found in live session.\nLook for 'ALI Installer' on the desktop.",
                "Kurulum bulunamadı.\nMasaüstündeki 'ALI Installer' simgesine bak."));
    gtk_dialog_run(GTK_DIALOG(d));
    gtk_widget_destroy(d);
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

static void stack_page(GtkWidget *stack, GtkWidget *box, const char *name, const char *title) {
    gtk_stack_add_named(GTK_STACK(stack), box, name);
    gtk_container_child_set(GTK_CONTAINER(stack), box, "title", title, NULL);
}

/* Read whole file into a malloc'd buffer (caller frees), NULL if missing. */
static char *read_file_all(const char *path) {
    FILE *f = fopen(path, "r");
    if (!f) return NULL;
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (sz < 0) sz = 0;
    if (sz > 65536) sz = 65536;
    char *buf = malloc((size_t)sz + 1);
    if (!buf) { fclose(f); return NULL; }
    size_t rd = fread(buf, 1, (size_t)sz, f);
    fclose(f);
    buf[rd] = '\0';
    return buf;
}

/* Last n lines of a text buffer (no copy past returned static). */
static char status_buf[4096];
static const char *build_status_text(void) {
    GString *s = g_string_new(NULL);
    char *st = read_file_all("/run/odysseus-status");
    if (st) {
        g_string_append_printf(s, "Odysseus: running\n%s\n", st);
        free(st);
    } else {
        g_string_append(s, "Odysseus: not running\n(starts at boot on ALI Linux)\n");
    }
    char *up = read_file_all("/run/ali-updates");
    if (up) {
        g_string_append_printf(s, "Pending updates: %s", up);
        if (up[strlen(up) ? strlen(up) - 1 : 0] != '\n') g_string_append_c(s, '\n');
        free(up);
    }
    char *lg = read_file_all("/var/log/odysseus.log");
    if (lg) {
        /* keep last 8 lines */
        int total = 0;
        for (char *p = lg; *p; p++) if (*p == '\n') total++;
        char *p = lg;
        int skip = total > 8 ? total - 8 : 0;
        while (skip > 0 && *p) { if (*p == '\n') skip--; p++; }
        g_string_append_printf(s, "\n--- log (tail) ---\n%s", p);
        free(lg);
    }
    snprintf(status_buf, sizeof(status_buf), "%s", s->str);
    g_string_free(s, TRUE);
    return status_buf;
}

static void on_status_refresh(GtkButton *b, gpointer u) {
    (void)b;
    gtk_label_set_text(GTK_LABEL(u), build_status_text());
}

static void on_bootreport_clicked(GtkButton *b, gpointer u) {
    (void)b; (void)u;
    launch_async("x-terminal-emulator -e 'systemd-analyze 2>/dev/null; echo; systemd-analyze blame 2>/dev/null | head -15; echo; read -n1 -p \"press any key\"'");
}

/* --- Sentinel (IDS alerts + IPS blocks) --- */
static char sentinel_buf[4096];
static const char *build_security_text(void) {
    GString *s = g_string_new(NULL);
    /* Oracle first (unified verdicts+lots), raw Sentinel feed as fallback */
    char *feed = read_file_all("/run/ali-oracle");
    if (!feed) feed = read_file_all("/run/ali-security");
    if (feed) {
        g_string_append(s, T("Sentinel: running (IDS watch + IPS exterminate)\n\n",
            "Sentinel: çalışıyor (IDS izleme + IPS engelleme)\n\n"));
        /* last 15 lines of feed */
        int total = 0;
        for (char *p = feed; *p; p++) if (*p == '\n') total++;
        char *p = feed;
        int skip = total > 15 ? total - 15 : 0;
        while (skip > 0 && *p) { if (*p == '\n') skip--; p++; }
        g_string_append(s, p);
        free(feed);
    } else {
        g_string_append(s, T("Sentinel: not running\n(starts at boot on ALI Linux 1.2.0+)\n",
            "Sentinel: çalışmıyor\n(ALI Linux 1.2.0+ sürümünde açılışta başlar)\n"));
    }
    g_string_append(s, T("\nManage from terminal: ali-sentinel status|events|blocked|unblock <ip>|learn|check|kill <pid>",
        "\nUçbirimden yönet: ali-sentinel status|events|blocked|unblock <ip>|learn|check|kill <pid>"));
    snprintf(sentinel_buf, sizeof(sentinel_buf), "%s", s->str);
    g_string_free(s, TRUE);
    return sentinel_buf;
}

static void on_security_refresh(GtkButton *b, gpointer u) {
    (void)b;
    gtk_label_set_text(GTK_LABEL(u), build_security_text());
}

static void on_scan_clicked(GtkButton *b, gpointer u) {
    (void)b; (void)u;
    launch_async("x-terminal-emulator -e 'sudo sentinel --once; echo; echo --- feed ---; tail -n 10 /run/ali-security; echo; read -n1 -p \"press any key\"'");
}

/* --- Firewall controls (1.3.3): ufw status + unblock + allow-port --- */
static void on_unblock_clicked(GtkButton *b, gpointer u) {
    (void)b; (void)u;
    GtkWidget *d = gtk_dialog_new_with_buttons(T("Unblock IP", "Engeli Kaldır"),
        NULL, GTK_DIALOG_MODAL, T("_Cancel", "_Vazgeç"), GTK_RESPONSE_CANCEL,
        T("_Unblock", "_Kaldır"), GTK_RESPONSE_ACCEPT, NULL);
    GtkWidget *e = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(e), "1.2.3.4");
    gtk_container_add(GTK_CONTAINER(gtk_dialog_get_content_area(GTK_DIALOG(d))), e);
    gtk_widget_show_all(d);
    if (gtk_dialog_run(GTK_DIALOG(d)) == GTK_RESPONSE_ACCEPT) {
        const char *ip = gtk_entry_get_text(GTK_ENTRY(e));
        if (ip && *ip) {
            char cmd[256];
            snprintf(cmd, sizeof(cmd),
                "x-terminal-emulator -e 'sudo ufw delete deny from %s; echo; read -n1 -p \"done\"'", ip);
            launch_async(cmd);
        }
    }
    gtk_widget_destroy(d);
}

static void on_allow_clicked(GtkButton *b, gpointer u) {
    (void)b; (void)u;
    GtkWidget *d = gtk_dialog_new_with_buttons(T("Allow Port", "Porta İzin Ver"),
        NULL, GTK_DIALOG_MODAL, T("_Cancel", "_Vazgeç"), GTK_RESPONSE_CANCEL,
        T("_Allow", "_İzin Ver"), GTK_RESPONSE_ACCEPT, NULL);
    GtkWidget *e = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(e), "22");
    gtk_container_add(GTK_CONTAINER(gtk_dialog_get_content_area(GTK_DIALOG(d))), e);
    gtk_widget_show_all(d);
    if (gtk_dialog_run(GTK_DIALOG(d)) == GTK_RESPONSE_ACCEPT) {
        const char *port = gtk_entry_get_text(GTK_ENTRY(e));
        if (port && atoi(port) > 0) {
            char cmd[256];
            snprintf(cmd, sizeof(cmd),
                "x-terminal-emulator -e 'sudo ufw allow %d; echo; read -n1 -p \"done\"'", atoi(port));
            launch_async(cmd);
        }
    }
    gtk_widget_destroy(d);
}

/* --- Terry's Dice in the Security tab (1.3.0) --- */
static char *run_capture(const char *cmd) {
    FILE *p = popen(cmd, "r");
    if (!p) return NULL;
    static char buf[256];
    size_t n = fread(buf, 1, sizeof(buf) - 1, p);
    pclose(p);
    buf[n] = '\0';
    char *nl = strchr(buf, '\n');
    if (nl) *nl = '\0';
    return buf[0] ? buf : NULL;
}

static void on_passgen_clicked(GtkButton *b, gpointer u) {
    (void)b; (void)u;
    char *pw = run_capture("ali-passgen 2>/dev/null");
    GtkWidget *d = gtk_message_dialog_new(NULL, GTK_DIALOG_MODAL,
        GTK_MESSAGE_INFO, GTK_BUTTONS_CLOSE,
        "%s\n%s", T("Fresh password (96-bit, from Terry's Dice):",
                     "Taze parola (96-bit, Terry's Dice'tan):"),
        pw ? pw : T("(terry-dice not ready)", "(terry-dice hazır değil)"));
    GtkWidget *lbl = gtk_message_dialog_get_message_area(GTK_MESSAGE_DIALOG(d));
    for (GList *c = gtk_container_get_children(GTK_CONTAINER(lbl)); c; c = c->next)
        if (GTK_IS_LABEL(c->data)) gtk_label_set_selectable(GTK_LABEL(c->data), TRUE);
    gtk_dialog_run(GTK_DIALOG(d));
    gtk_widget_destroy(d);
}

static int count_vaults(void) {
    int n = 0;
    const char *home = g_get_home_dir();
    GDir *d = g_dir_open(home, 0, NULL);
    if (!d) return 0;
    const char *e;
    while ((e = g_dir_read_name(d))) {
        size_t l = strlen(e);
        if (l > 6 && strcmp(e + l - 6, ".vault") == 0) n++;
    }
    g_dir_close(d);
    return n;
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

    GtkWidget *win = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(win), "ALI Center");
    gtk_window_set_icon_name(GTK_WINDOW(win), "ali-center");
    gtk_window_set_default_size(GTK_WINDOW(win), 720, 480);
    gtk_window_set_position(GTK_WINDOW(win), GTK_WIN_POS_CENTER);
    g_signal_connect(win, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    GtkWidget *hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_container_add(GTK_CONTAINER(win), hbox);
    GtkWidget *stack = gtk_stack_new();
    gtk_stack_set_transition_type(GTK_STACK(stack), GTK_STACK_TRANSITION_TYPE_SLIDE_LEFT_RIGHT);
    GtkWidget *sb = gtk_stack_sidebar_new();
    gtk_stack_sidebar_set_stack(GTK_STACK_SIDEBAR(sb), GTK_STACK(stack));
    gtk_box_pack_start(GTK_BOX(hbox), sb, FALSE, FALSE, 0);
    GtkWidget *sep = gtk_separator_new(GTK_ORIENTATION_VERTICAL);
    gtk_box_pack_start(GTK_BOX(hbox), sep, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(hbox), stack, TRUE, TRUE, 0);

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

        GtkWidget *b1 = gtk_button_new_with_label(T("System Info (fastfetch)", "Sistem Bilgisi (fastfetch)"));
        g_signal_connect(b1, "clicked", G_CALLBACK(on_sysinfo_clicked), NULL);
        gtk_box_pack_start(GTK_BOX(box), b1, FALSE, FALSE, 0);

        GtkWidget *b2 = gtk_button_new_with_label(T("Check for Updates (apt)", "Güncellemeleri Denetle (apt)"));
        g_signal_connect(b2, "clicked", G_CALLBACK(on_update_clicked), NULL);
        gtk_box_pack_start(GTK_BOX(box), b2, FALSE, FALSE, 0);

        /* Governor readout + Perf/Normal + fastest mirror */
        {
            char *gov = run_capture("cat /sys/devices/system/cpu/cpu0/cpufreq/scaling_governor 2>/dev/null");
            char govline[160];
            snprintf(govline, sizeof(govline), "%s: %s",
                T("Governor", "Yönetici"), gov ? gov : T("(no cpufreq)", "(cpufreq yok)"));
            GtkWidget *gl = gtk_label_new(govline);
            gtk_box_pack_start(GTK_BOX(box), gl, FALSE, FALSE, 0);
            GtkWidget *grow = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
            GtkWidget *bp = gtk_button_new_with_label(T("Performance", "Performans"));
            g_signal_connect(bp, "clicked", G_CALLBACK(on_perf_clicked), gl);
            gtk_box_pack_start(GTK_BOX(grow), bp, TRUE, TRUE, 0);
            GtkWidget *bn = gtk_button_new_with_label(T("Normal", "Normal"));
            g_signal_connect(bn, "clicked", G_CALLBACK(on_normal_clicked), gl);
            gtk_box_pack_start(GTK_BOX(grow), bn, TRUE, TRUE, 0);
            gtk_box_pack_start(GTK_BOX(box), grow, FALSE, FALSE, 0);
            GtkWidget *bm = gtk_button_new_with_label(T("Fastest Mirror", "En Hızlı Ayna"));
            g_signal_connect(bm, "clicked", G_CALLBACK(on_mirror_clicked), NULL);
            gtk_box_pack_start(GTK_BOX(box), bm, FALSE, FALSE, 0);
        }

        GtkWidget *hint = gtk_label_new(T("Keyboard: Turkish (tr) + English supported.\nSwitch with XFCE Panel -> Keyboard applet.",
            "Klavye: Türkçe (tr) + İngilizce desteklenir.\nXFCE Paneli -> Klavye uygulamasıyla değiştir."));
        gtk_box_pack_start(GTK_BOX(box), hint, FALSE, FALSE, 8);

        stack_page(stack, box, "system", T("System", "Sistem"));
    }

    /* --- Appearance tab --- */
    {
        GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
        gtk_container_set_border_width(GTK_CONTAINER(box), 16);
        GtkWidget *l = gtk_label_new(T("Theme: Greybird - Icons: elementary-xfce\nWallpapers: /usr/share/backgrounds/ali/",
            "Tema: Greybird - Simgeler: elementary-xfce\nDuvar kağıtları: /usr/share/backgrounds/ali/"));
        gtk_box_pack_start(GTK_BOX(box), l, FALSE, FALSE, 0);
        GtkWidget *b = gtk_button_new_with_label(T("Open Wallpapers Folder", "Duvar Kağıtları Klasörünü Aç"));
        g_signal_connect(b, "clicked", G_CALLBACK(on_wallpapers_clicked), NULL);
        gtk_box_pack_start(GTK_BOX(box), b, FALSE, FALSE, 0);
        /* Night light: status + on/off/auto (terrydavis honors force) */
        {
            char *nl = run_capture("pgrep -x redshift >/dev/null 2>&1 && echo on || echo off");
            char *nf = run_capture("cat /var/lib/terrydavis/night.force 2>/dev/null");
            char nline[192];
            snprintf(nline, sizeof(nline), "%s: %s%s%s",
                T("Night light", "Gece ışığı"),
                (nl && strcmp(nl, "on") == 0) ? T("ON", "AÇIK") : T("off", "kapalı"),
                (nf && *nf) ? " (" : "",
                (nf && *nf) ? (strcmp(nf, "on") == 0 ? T("forced)", "zorla)") : T("held off)", "tutuluyor)")) : "");
            GtkWidget *nlab = gtk_label_new(nline);
            gtk_box_pack_start(GTK_BOX(box), nlab, FALSE, FALSE, 0);
            GtkWidget *nrow = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
            GtkWidget *n1 = gtk_button_new_with_label(T("On", "Aç"));
            g_signal_connect(n1, "clicked", G_CALLBACK(on_night_clicked), (gpointer)"on");
            gtk_box_pack_start(GTK_BOX(nrow), n1, TRUE, TRUE, 0);
            GtkWidget *n2 = gtk_button_new_with_label(T("Off", "Kapat"));
            g_signal_connect(n2, "clicked", G_CALLBACK(on_night_clicked), (gpointer)"off");
            gtk_box_pack_start(GTK_BOX(nrow), n2, TRUE, TRUE, 0);
            GtkWidget *n3 = gtk_button_new_with_label(T("Auto", "Otomatik"));
            g_signal_connect(n3, "clicked", G_CALLBACK(on_night_clicked), (gpointer)"auto");
            gtk_box_pack_start(GTK_BOX(nrow), n3, TRUE, TRUE, 0);
            gtk_box_pack_start(GTK_BOX(box), nrow, FALSE, FALSE, 0);
        }
        /* Transparent terminal (applies to new windows) */
        {
            GtkWidget *trow = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
            GtkWidget *t0 = gtk_label_new(T("Terminal:", "Uçbirim:"));
            gtk_box_pack_start(GTK_BOX(trow), t0, FALSE, FALSE, 0);
            GtkWidget *t1 = gtk_button_new_with_label(T("Transparent", "Saydam"));
            g_signal_connect(t1, "clicked", G_CALLBACK(on_transparent_clicked), (gpointer)"on");
            gtk_box_pack_start(GTK_BOX(trow), t1, TRUE, TRUE, 0);
            GtkWidget *t2 = gtk_button_new_with_label(T("Opaque", "Opak"));
            g_signal_connect(t2, "clicked", G_CALLBACK(on_transparent_clicked), (gpointer)"off");
            gtk_box_pack_start(GTK_BOX(trow), t2, TRUE, TRUE, 0);
            gtk_box_pack_start(GTK_BOX(box), trow, FALSE, FALSE, 0);
        }
        GtkWidget *h = gtk_label_new(T("Tip: right-click Desktop -> Desktop Settings to change wallpaper.",
            "İpucu: duvar kağıdını değiştirmek için Masaüstüne sağ tıkla -> Masaüstü Ayarları."));
        gtk_box_pack_start(GTK_BOX(box), h, FALSE, FALSE, 8);
        stack_page(stack, box, "appearance", T("Appearance", "Görünüm"));
    }

    /* --- Apps tab --- */
    {
        GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
        gtk_container_set_border_width(GTK_CONTAINER(box), 16);
        GtkWidget *b1 = gtk_button_new_with_label(T("Open ALI Terminal", "ALI Uçbirimini Aç"));
        g_signal_connect(b1, "clicked", G_CALLBACK(on_terminal_clicked), NULL);
        gtk_box_pack_start(GTK_BOX(box), b1, FALSE, FALSE, 0);
        GtkWidget *b2 = gtk_button_new_with_label(T("Open Files", "Dosyaları Aç"));
        g_signal_connect(b2, "clicked", G_CALLBACK(on_files_clicked), NULL);
        gtk_box_pack_start(GTK_BOX(box), b2, FALSE, FALSE, 0);
        GtkWidget *b3 = gtk_button_new_with_label(T("Open Browser", "Tarayıcıyı Aç"));
        g_signal_connect(b3, "clicked", G_CALLBACK(on_browser_clicked), NULL);
        gtk_box_pack_start(GTK_BOX(box), b3, FALSE, FALSE, 0);
        GtkWidget *b4 = gtk_button_new_with_label(T("Screenshot", "Ekran Görüntüsü"));
        g_signal_connect(b4, "clicked", G_CALLBACK(on_shot_clicked), NULL);
        gtk_box_pack_start(GTK_BOX(box), b4, FALSE, FALSE, 0);
        GtkWidget *b5 = gtk_button_new_with_label(T("Disk Usage", "Disk Kullanımı"));
        g_signal_connect(b5, "clicked", G_CALLBACK(on_disk_clicked), NULL);
        gtk_box_pack_start(GTK_BOX(box), b5, FALSE, FALSE, 0);
        /* Game corner: status + auto/on/off cycle */
        {
            GtkWidget *gl = gtk_label_new(NULL);
            gtk_label_set_text(GTK_LABEL(gl), game_status_text());
            gtk_box_pack_start(GTK_BOX(box), gl, FALSE, FALSE, 8);
            GtkWidget *gb = gtk_button_new_with_label(T("Game Mode: Auto / On / Off", "Oyun Modu: Otomatik / Açık / Kapalı"));
            g_signal_connect(gb, "clicked", G_CALLBACK(on_game_clicked), gl);
            gtk_box_pack_start(GTK_BOX(box), gb, FALSE, FALSE, 0);
        }
        stack_page(stack, box, "apps", T("Apps", "Uygulamalar"));
    }

    /* --- Install tab --- */
    {
        GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
        gtk_container_set_border_width(GTK_CONTAINER(box), 16);
        GtkWidget *l = gtk_label_new(T("Try live, then install to disk when ready.",
            "Önce canlı dene, hazır olunca diske kur."));
        gtk_box_pack_start(GTK_BOX(box), l, FALSE, FALSE, 0);
        GtkWidget *b = gtk_button_new_with_label(T("Install ALI to Disk", "ALI'yi Diske Kur"));
        gtk_widget_set_size_request(b, -1, 48);
        g_signal_connect(b, "clicked", G_CALLBACK(on_install_clicked), NULL);
        gtk_box_pack_start(GTK_BOX(box), b, FALSE, FALSE, 0);
        stack_page(stack, box, "install", T("Install", "Kurulum"));
    }

    /* --- Updates tab (1.3.6) --- */
    {
        GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
        gtk_container_set_border_width(GTK_CONTAINER(box), 16);
        GtkWidget *l = gtk_label_new("?");
        gtk_label_set_selectable(GTK_LABEL(l), TRUE);
        on_updates_refresh(NULL, l);
        gtk_box_pack_start(GTK_BOX(box), l, FALSE, FALSE, 0);
        GtkWidget *b1 = gtk_button_new_with_label(T("Check Now", "Şimdi Denetle"));
        g_signal_connect(b1, "clicked", G_CALLBACK(on_update_clicked), NULL);
        gtk_box_pack_start(GTK_BOX(box), b1, FALSE, FALSE, 0);
        GtkWidget *b2 = gtk_button_new_with_label(T("Show List", "Listeyi Göster"));
        g_signal_connect(b2, "clicked", G_CALLBACK(on_updates_list), NULL);
        gtk_box_pack_start(GTK_BOX(box), b2, FALSE, FALSE, 0);
        GtkWidget *b3 = gtk_button_new_with_label(T("Upgrade Now", "Şimdi Yükselt"));
        g_signal_connect(b3, "clicked", G_CALLBACK(on_upgrade_clicked), NULL);
        gtk_box_pack_start(GTK_BOX(box), b3, FALSE, FALSE, 0);
        GtkWidget *b4 = gtk_button_new_with_label(T("Refresh Count", "Sayıyı Yenile"));
        g_signal_connect(b4, "clicked", G_CALLBACK(on_updates_refresh), l);
        gtk_box_pack_start(GTK_BOX(box), b4, FALSE, FALSE, 0);
        stack_page(stack, box, "updates", T("Updates", "Güncellemeler"));
    }

    /* --- Status tab --- */
    {
        GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
        gtk_container_set_border_width(GTK_CONTAINER(box), 16);
        GtkWidget *l = gtk_label_new(NULL);
        gtk_label_set_selectable(GTK_LABEL(l), TRUE);
        gtk_label_set_text(GTK_LABEL(l), build_status_text());
        gtk_box_pack_start(GTK_BOX(box), l, FALSE, FALSE, 0);
        GtkWidget *b = gtk_button_new_with_label(T("Refresh", "Yenile"));
        g_signal_connect(b, "clicked", G_CALLBACK(on_status_refresh), l);
        gtk_box_pack_start(GTK_BOX(box), b, FALSE, FALSE, 0);
        GtkWidget *bb = gtk_button_new_with_label(T("Boot Report", "Açılış Raporu"));
        g_signal_connect(bb, "clicked", G_CALLBACK(on_bootreport_clicked), NULL);
        gtk_box_pack_start(GTK_BOX(box), bb, FALSE, FALSE, 0);
        stack_page(stack, box, "status", T("Status", "Durum"));
    }

    /* --- Security tab (Sentinel IDS/IPS) --- */
    {
        GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
        gtk_container_set_border_width(GTK_CONTAINER(box), 16);
        GtkWidget *l = gtk_label_new(NULL);
        gtk_label_set_selectable(GTK_LABEL(l), TRUE);
        gtk_label_set_text(GTK_LABEL(l), build_security_text());
        gtk_box_pack_start(GTK_BOX(box), l, FALSE, FALSE, 0);
        GtkWidget *row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
        GtkWidget *b1 = gtk_button_new_with_label(T("Refresh", "Yenile"));
        g_signal_connect(b1, "clicked", G_CALLBACK(on_security_refresh), l);
        gtk_box_pack_start(GTK_BOX(row), b1, TRUE, TRUE, 0);
        GtkWidget *b2 = gtk_button_new_with_label(T("Run Scan Now", "Şimdi Tara"));
        g_signal_connect(b2, "clicked", G_CALLBACK(on_scan_clicked), NULL);
        gtk_box_pack_start(GTK_BOX(row), b2, TRUE, TRUE, 0);
        gtk_box_pack_start(GTK_BOX(box), row, FALSE, FALSE, 0);
        /* Terry's Dice row: status + one-click password + vault count */
        {
            char dice[256];
            char *id = run_capture("terry-dice id 2>/dev/null");
            if (id)
                snprintf(dice, sizeof(dice), "%s %.12s... | %s: %d",
                    T("Dice: ready (machine", "Zar: hazır (makine"),
                    id, T("Vaults here", "Buradaki kasalar"), count_vaults());
            else
                snprintf(dice, sizeof(dice), "%s",
                    T("Dice: ready (terry-dice installed)", "Zar: hazır (terry-dice kurulu)"));
            GtkWidget *dl = gtk_label_new(dice);
            gtk_box_pack_start(GTK_BOX(box), dl, FALSE, FALSE, 0);
            GtkWidget *b3 = gtk_button_new_with_label(T("Generate Password", "Parola Üret"));
            g_signal_connect(b3, "clicked", G_CALLBACK(on_passgen_clicked), NULL);
            gtk_box_pack_start(GTK_BOX(box), b3, FALSE, FALSE, 0);
            /* Firewall row: live status + unblock + allow */
            {
                char *fw = run_capture("sudo -n ufw status 2>/dev/null | head -1");
                char fwline[256];
                snprintf(fwline, sizeof(fwline), "%s: %s",
                    T("Firewall", "Güvenlik Duvarı"),
                    fw ? fw : T("(run once in terminal: sudo ufw status)",
                                "(uçbirimde bir kez çalıştır: sudo ufw status)"));
                GtkWidget *fl = gtk_label_new(fwline);
                gtk_box_pack_start(GTK_BOX(box), fl, FALSE, FALSE, 0);
                GtkWidget *frow = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
                GtkWidget *b4 = gtk_button_new_with_label(T("Unblock IP…", "Engel Kaldır…"));
                g_signal_connect(b4, "clicked", G_CALLBACK(on_unblock_clicked), NULL);
                gtk_box_pack_start(GTK_BOX(frow), b4, TRUE, TRUE, 0);
                GtkWidget *b5 = gtk_button_new_with_label(T("Allow Port…", "Port Aç…"));
                g_signal_connect(b5, "clicked", G_CALLBACK(on_allow_clicked), NULL);
                gtk_box_pack_start(GTK_BOX(frow), b5, TRUE, TRUE, 0);
                gtk_box_pack_start(GTK_BOX(box), frow, FALSE, FALSE, 0);
            }
        }
        stack_page(stack, box, "security", T("Security", "Güvenlik"));
    }

    /* --- About tab --- */
    {
        GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
        gtk_container_set_border_width(GTK_CONTAINER(box), 16);
        char about[512];
        snprintf(about, sizeof(about), ".-------.\n| o   o |\n|   A   |\n| o   o |\n'-------'\nALI Linux 1.4.8\nXFCE - amd64.\n\nALI Center 1.4.8 - C + GTK3 (+ Sentinel Security tab).");
        char *vs = read_file_all("/run/templeos-oracle");
        if (vs) {
            char *vl = strstr(vs, "verse=");
            if (vl) {
                vl += 6;
                char *nl = strchr(vl, '\n');
                if (nl) *nl = '\0';
                snprintf(about + strlen(about), sizeof(about) - strlen(about), T("\n\nOracle says: %s", "\n\nKahin diyor: %s"), vl);
            }
            free(vs);
        }
        GtkWidget *l = gtk_label_new(about);
        gtk_box_pack_start(GTK_BOX(box), l, FALSE, FALSE, 0);
        stack_page(stack, box, "about", T("About", "Hakkında"));
    }

    gtk_widget_show_all(win);
    gtk_main();
    return 0;
}
