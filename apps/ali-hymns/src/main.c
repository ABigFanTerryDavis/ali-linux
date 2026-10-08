#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ALI Hymns 1.3.5 - TempleOS-tribute chiptune player, C + GTK3.
 * Original melodies (no covers). Two backends, auto-picked: `play` (sox,
 * real audio - works everywhere) first, PC-speaker `beep` as fallback.
 * --tr flag or ALI_LANG=tr for Turkish buttons.
 */

static int LANG_TR = 0;
static const char *T(const char *en, const char *tr) { return LANG_TR ? tr : en; }

/* 0 = none, 1 = beep, 2 = play */
static int backend(void) {
    static int b = -1;
    if (b >= 0) return b;
    b = 0;
    if (g_find_program_in_path("play")) b = 2;
    else if (g_find_program_in_path("beep")) b = 1;
    return b;
}

static void launch_bg(const char *cmd) {
    /* fire-and-forget through /bin/sh so beep plays without freezing UI */
    gchar *argv[] = { (gchar *)"/bin/sh", (gchar *)"-c", (gchar *)cmd, NULL };
    GError *err = NULL;
    if (!g_spawn_async(NULL, argv, NULL, G_SPAWN_SEARCH_PATH, NULL, NULL, NULL, &err)) {
        GtkWidget *d = gtk_message_dialog_new(NULL, GTK_DIALOG_MODAL,
            GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE,
            "Could not play:\n%s", err ? err->message : "");
        gtk_dialog_run(GTK_DIALOG(d));
        gtk_widget_destroy(d);
        if (err) g_error_free(err);
    }
}

/* freq:ms pairs; beep form + play form (freq:secs loop) */
static const char *H_TEMPLE =
    "beep -f 523 -l 300 -n -f 659 -l 300 -n -f 784 -l 300 -n -f 1047 -l 500 "
    "-n -f 784 -l 300 -n -f 659 -l 300 -n -f 523 -l 600";
static const char *P_TEMPLE = "523:0.30 659:0.30 784:0.30 1047:0.50 784:0.30 659:0.30 523:0.60";
static const char *H_ORACLE =
    "beep -f 440 -l 200 -n -f 523 -l 200 -n -f 659 -l 200 -n -f 880 -l 400 "
    "-n -f 784 -l 200 -n -f 659 -l 200 -n -f 523 -l 200 -n -f 440 -l 500";
static const char *P_ORACLE = "440:0.20 523:0.20 659:0.20 880:0.40 784:0.20 659:0.20 523:0.20 440:0.50";
static const char *H_640 =
    "beep -f 523 -l 150 -n -f 392 -l 150 -n -f 523 -l 150 -n -f 659 -l 150 "
    "-n -f 587 -l 150 -n -f 784 -l 400 -n -f 659 -l 150 -n -f 1047 -l 600";
static const char *P_640 = "523:0.15 392:0.15 523:0.15 659:0.15 587:0.15 784:0.40 659:0.15 1047:0.60";
static const char *H_DESERT =
    "beep -f 329 -l 250 -n -f 392 -l 250 -n -f 440 -l 250 -n -f 494 -l 400 "
    "-n -f 440 -l 250 -n -f 392 -l 250 -n -f 329 -l 600";
static const char *P_DESERT = "329:0.25 392:0.25 440:0.25 494:0.40 440:0.25 392:0.25 329:0.60";
static const char *H_FLUTE =
    "beep -f 587 -l 400 -n -f 740 -l 400 -n -f 880 -l 400 -n -f 784 -l 300 "
    "-n -f 740 -l 300 -n -f 587 -l 600";
static const char *P_FLUTE = "587:0.40 740:0.40 880:0.40 784:0.30 740:0.30 587:0.60";
static const char *H_AMEN =
    "beep -f 523 -l 200 -n -f 587 -l 200 -n -f 659 -l 200 -n -f 784 -l 200 "
    "-n -f 880 -l 400 -n -f 784 -l 200 -n -f 1047 -l 600";
static const char *P_AMEN = "523:0.20 587:0.20 659:0.20 784:0.20 880:0.40 784:0.20 1047:0.60";

static const char *N_TEMPLE = "C5 E5 G5 C6 G5 E5 C5";
static const char *N_ORACLE = "A4 C5 E5 A5 G5 E5 C5 A4";
static const char *N_640 = "C5 G4 C5 E5 D5 G5 E5 C6";
static const char *N_DESERT = "E4 G4 A4 B4 A4 G4 E4";
static const char *N_FLUTE = "D5 F#5 A5 G5 F#5 D5";
static const char *N_AMEN = "C5 D5 E5 G5 A5 G5 C6";

static GtkWidget *now_label = NULL;
static GtkWidget *vol_scale = NULL;
static int queue_idx = 0;
static int queue_on = 0;
static const char *queue_beep[6];
static const char *queue_pairs[6];
static const char *queue_notes[6];

static void play_hymn(const char *beep, const char *pairs) {
    int b = backend();
    if (b == 2) {
        double v = gtk_range_get_value(GTK_RANGE(vol_scale)) / 100.0;
        char cmd[1024];
        snprintf(cmd, sizeof(cmd),
            "for p in %s; do play -q -v %.2f -n synth ${p##*:} sine ${p%%:*}; done",
            pairs, v);
        launch_bg(cmd);
    } else if (b == 1) {
        launch_bg(beep);
    } else {
        gtk_label_set_text(GTK_LABEL(now_label),
            T("No sound backend (install sox or beep).",
              "Ses altyapısı yok (sox ya da beep kur)."));
    }
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

static void on_play(GtkButton *b, gpointer u) {
    (void)b;
    const char **h = (const char **)u;
    play_hymn(h[0], h[1]);
    char buf[256];
    snprintf(buf, sizeof(buf), "%s\n%s", T("Now playing:", "Şimdi çalıyor:"), h[2]);
    gtk_label_set_text(GTK_LABEL(now_label), buf);
}

static void on_stop(GtkButton *b, gpointer u) {
    (void)b; (void)u;
    queue_on = 0;
    system("pkill -x beep 2>/dev/null; pkill -x play 2>/dev/null");
    gtk_label_set_text(GTK_LABEL(now_label), T("Stopped.", "Durduruldu."));
}

static void play_index(int i) {
    char buf[256];
    snprintf(buf, sizeof(buf), "%s (%d/6)\n%s",
        T("Now playing:", "Şimdi çalıyor:"), i + 1, queue_notes[i]);
    gtk_label_set_text(GTK_LABEL(now_label), buf);
    play_hymn(queue_beep[i], queue_pairs[i]);
}

static void on_prev(GtkButton *b, gpointer u) {
    (void)b; (void)u;
    system("pkill -x beep 2>/dev/null; pkill -x play 2>/dev/null");
    queue_on = 0;
    queue_idx = (queue_idx + 5) % 6;
    play_index(queue_idx);
}

static void on_next(GtkButton *b, gpointer u) {
    (void)b; (void)u;
    system("pkill -x beep 2>/dev/null; pkill -x play 2>/dev/null");
    queue_on = 0;
    queue_idx = (queue_idx + 1) % 6;
    play_index(queue_idx);
}

static void on_playall(GtkButton *b, gpointer u) {
    (void)b; (void)u;
    /* one background shell, six songs chained; Stop (pkill) ends it */
    char cmd[8192];
    cmd[0] = '\0';
    int bk = backend();
    if (bk == 2) {
        double v = gtk_range_get_value(GTK_RANGE(vol_scale)) / 100.0;
        for (int i = 0; i < 6; i++) {
            char one[1024];
            snprintf(one, sizeof(one),
                "%sfor p in %s; do play -q -v %.2f -n synth ${p##*:} sine ${p%%:*}; done",
                i ? " ; " : "", queue_pairs[i], v);
            strncat(cmd, one, sizeof(cmd) - strlen(cmd) - 1);
        }
    } else if (bk == 1) {
        for (int i = 0; i < 6; i++) {
            strncat(cmd, i ? " ; " : "", sizeof(cmd) - strlen(cmd) - 1);
            strncat(cmd, queue_beep[i], sizeof(cmd) - strlen(cmd) - 1);
        }
    } else {
        gtk_label_set_text(GTK_LABEL(now_label),
            T("No sound backend (install sox or beep).",
              "Ses altyapısı yok (sox ya da beep kur)."));
        return;
    }
    queue_on = 0;
    launch_bg(cmd);
    gtk_label_set_text(GTK_LABEL(now_label),
        T("Queue: all six, in order. Stop ends it.",
          "Sıra: altı ilahi sırayla. Durdur bitirir."));
}

static void on_test(GtkButton *b, gpointer u) {
    (void)b; (void)u;
    int bk = backend();
    if (bk == 2) launch_bg("play -q -n synth 0.2 sine 880");
    else if (bk == 1) launch_bg("beep -f 880 -l 200");
    gtk_label_set_text(GTK_LABEL(now_label),
        bk ? T("Backend: real audio. If silent, check volume.",
               "Altyapı: gerçek ses. Sessizse sesi kontrol et.")
            : T("No backend. Install sox.",
                "Altyapı yok. sox kur."));
}

static GtkWidget *hymn_row(const char *title, const char *beep, const char *pairs, const char *notes) {
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    GtkWidget *l = gtk_label_new(title);
    gtk_widget_set_size_request(l, 160, -1);
    gtk_label_set_xalign(GTK_LABEL(l), 0.0);
    gtk_box_pack_start(GTK_BOX(box), l, FALSE, FALSE, 0);
    static const char *sets[6][3];
    static int idx = 0;
    if (idx < 6) {
        sets[idx][0] = beep; sets[idx][1] = pairs; sets[idx][2] = notes;
        queue_beep[idx] = beep; queue_pairs[idx] = pairs; queue_notes[idx] = notes;
    }
    GtkWidget *b = gtk_button_new_with_label(T("Play", "Çal"));
    g_signal_connect(b, "clicked", G_CALLBACK(on_play), (gpointer)sets[idx < 6 ? idx : 5]);
    gtk_box_pack_start(GTK_BOX(box), b, TRUE, TRUE, 0);
    idx++;
    return box;
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
    gtk_window_set_title(GTK_WINDOW(win), T("ALI Hymns", "ALI İlahiler"));
    gtk_window_set_icon_name(GTK_WINDOW(win), "ali-hymns");
    gtk_window_set_default_size(GTK_WINDOW(win), 460, 520);
    gtk_window_set_position(GTK_WINDOW(win), GTK_WIN_POS_CENTER);
    g_signal_connect(win, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
    gtk_container_set_border_width(GTK_CONTAINER(box), 16);
    gtk_container_add(GTK_CONTAINER(win), box);

    GtkWidget *head = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(head),
        T("<b>ALI Hymns</b>\nTempleOS-tribute chiptunes (originals).",
          "<b>ALI İlahiler</b>\nTempleOS anısına chiptune'lar (özgün)."));
    gtk_box_pack_start(GTK_BOX(box), head, FALSE, FALSE, 0);

    gtk_box_pack_start(GTK_BOX(box), hymn_row("Temple Morning", H_TEMPLE, P_TEMPLE, N_TEMPLE), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(box), hymn_row("Oracle's Dance", H_ORACLE, P_ORACLE, N_ORACLE), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(box), hymn_row("640x480", H_640, P_640, N_640), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(box), hymn_row("Desert Walk", H_DESERT, P_DESERT, N_DESERT), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(box), hymn_row("Shepherd's Flute", H_FLUTE, P_FLUTE, N_FLUTE), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(box), hymn_row("Amen", H_AMEN, P_AMEN, N_AMEN), FALSE, FALSE, 0);

    GtkWidget *vrow = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    GtkWidget *vl = gtk_label_new(T("Volume:", "Ses:"));
    gtk_box_pack_start(GTK_BOX(vrow), vl, FALSE, FALSE, 0);
    vol_scale = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 0, 100, 5);
    gtk_range_set_value(GTK_RANGE(vol_scale), 80);
    gtk_box_pack_start(GTK_BOX(vrow), vol_scale, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(box), vrow, FALSE, FALSE, 0);

    GtkWidget *qrow = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    GtkWidget *bprev = gtk_button_new_with_label(T("◀ Prev", "◀ Önceki"));
    g_signal_connect(bprev, "clicked", G_CALLBACK(on_prev), NULL);
    gtk_box_pack_start(GTK_BOX(qrow), bprev, TRUE, TRUE, 0);
    GtkWidget *ball = gtk_button_new_with_label(T("Play All", "Tümünü Çal"));
    g_signal_connect(ball, "clicked", G_CALLBACK(on_playall), NULL);
    gtk_box_pack_start(GTK_BOX(qrow), ball, TRUE, TRUE, 0);
    GtkWidget *bnext = gtk_button_new_with_label(T("Next ▶", "Sonraki ▶"));
    g_signal_connect(bnext, "clicked", G_CALLBACK(on_next), NULL);
    gtk_box_pack_start(GTK_BOX(qrow), bnext, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(box), qrow, FALSE, FALSE, 0);

    GtkWidget *row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    GtkWidget *bs = gtk_button_new_with_label(T("Stop", "Durdur"));
    g_signal_connect(bs, "clicked", G_CALLBACK(on_stop), NULL);
    gtk_box_pack_start(GTK_BOX(row), bs, TRUE, TRUE, 0);
    GtkWidget *bt = gtk_button_new_with_label(T("Test Speaker", "Hoparlörü Dene"));
    g_signal_connect(bt, "clicked", G_CALLBACK(on_test), NULL);
    gtk_box_pack_start(GTK_BOX(row), bt, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(box), row, FALSE, FALSE, 0);

    now_label = gtk_label_new(T("Pick a hymn. Amen.", "Bir ilahi seç. Amin."));
    gtk_label_set_selectable(GTK_LABEL(now_label), TRUE);
    gtk_box_pack_start(GTK_BOX(box), now_label, FALSE, FALSE, 8);

    gtk_widget_show_all(win);
    gtk_main();
    return 0;
}
