#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ALI Hymns 1.2.5 - TempleOS-tribute chiptune player, C + GTK3.
 * Original melodies (no covers). Sound via `beep` (needs a PC speaker;
 * silent on most VMs - the notes still dance on screen either way).
 * --tr flag or ALI_LANG=tr for Turkish buttons.
 */

static int LANG_TR = 0;
static const char *T(const char *en, const char *tr) { return LANG_TR ? tr : en; }

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

/* freq,ms pairs encoded as beep args */
static const char *H_TEMPLE =
    "beep -f 523 -l 300 -n -f 659 -l 300 -n -f 784 -l 300 -n -f 1047 -l 500 "
    "-n -f 784 -l 300 -n -f 659 -l 300 -n -f 523 -l 600";
static const char *H_ORACLE =
    "beep -f 440 -l 200 -n -f 523 -l 200 -n -f 659 -l 200 -n -f 880 -l 400 "
    "-n -f 784 -l 200 -n -f 659 -l 200 -n -f 523 -l 200 -n -f 440 -l 500";
static const char *H_640 =
    "beep -f 523 -l 150 -n -f 392 -l 150 -n -f 523 -l 150 -n -f 659 -l 150 "
    "-n -f 587 -l 150 -n -f 784 -l 400 -n -f 659 -l 150 -n -f 1047 -l 600";

static const char *N_TEMPLE = "C5 E5 G5 C6 G5 E5 C5";
static const char *N_ORACLE = "A4 C5 E5 A5 G5 E5 C5 A4";
static const char *N_640 = "C5 G4 C5 E5 D5 G5 E5 C6";

static GtkWidget *now_label = NULL;

static void on_play(GtkButton *b, gpointer u) {
    (void)b;
    const char **h = (const char **)u;
    launch_bg(h[0]);
    char buf[256];
    snprintf(buf, sizeof(buf), "%s\n%s", T("Now playing:", "Şimdi çalıyor:"), h[1]);
    gtk_label_set_text(GTK_LABEL(now_label), buf);
}

static void on_stop(GtkButton *b, gpointer u) {
    (void)b; (void)u;
    system("pkill -x beep 2>/dev/null");
    gtk_label_set_text(GTK_LABEL(now_label), T("Stopped.", "Durduruldu."));
}

static void on_test(GtkButton *b, gpointer u) {
    (void)b; (void)u;
    launch_bg("beep -f 880 -l 200");
    gtk_label_set_text(GTK_LABEL(now_label),
        T("If you heard a beep, hymns will sing.\nIf not, this machine has no PC speaker.",
          "Bip duyduysan ilahiler çalar.\nDuymadıysan bu makinede PC hoparlörü yok."));
}

static GtkWidget *hymn_row(const char *title, const char *cmd, const char *notes) {
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    GtkWidget *l = gtk_label_new(title);
    gtk_widget_set_size_request(l, 160, -1);
    gtk_label_set_xalign(GTK_LABEL(l), 0.0);
    gtk_box_pack_start(GTK_BOX(box), l, FALSE, FALSE, 0);
    static const char *sets[3][2];
    static int idx = 0;
    sets[idx][0] = cmd; sets[idx][1] = notes;
    GtkWidget *b = gtk_button_new_with_label(T("Play", "Çal"));
    g_signal_connect(b, "clicked", G_CALLBACK(on_play), (gpointer)sets[idx]);
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

    GtkWidget *win = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(win), T("ALI Hymns", "ALI İlahiler"));
    gtk_window_set_default_size(GTK_WINDOW(win), 420, 300);
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

    gtk_box_pack_start(GTK_BOX(box), hymn_row("Temple Morning", H_TEMPLE, N_TEMPLE), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(box), hymn_row("Oracle's Dance", H_ORACLE, N_ORACLE), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(box), hymn_row("640x480", H_640, N_640), FALSE, FALSE, 0);

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
