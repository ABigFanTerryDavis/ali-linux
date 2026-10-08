#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ali-shrine 1.3.6 - idle oracle screensaver. Fullscreen black, one WORD +
 * verse in white, advances every 12s, any key/click wakes. Launched by
 * terrydavis when the seat idles 10+ minutes (xprintidle).
 */

static const char *feed_word(char *wbuf, size_t wn, char *vbuf, size_t vn) {
    FILE *f = fopen("/run/templeos-oracle", "r");
    if (!f) return NULL;
    char line[256];
    wbuf[0] = vbuf[0] = '\0';
    while (fgets(line, sizeof(line), f)) {
        if (strncmp(line, "oracle=", 7) == 0)
            snprintf(wbuf, wn, "%s", line + 7);
        else if (strncmp(line, "verse=", 6) == 0)
            snprintf(vbuf, vn, "%s", line + 6);
    }
    fclose(f);
    char *nl;
    if ((nl = strchr(wbuf, '\n'))) *nl = '\0';
    if ((nl = strchr(vbuf, '\n'))) *nl = '\0';
    return wbuf[0] ? wbuf : NULL;
}

static GtkWidget *word_label = NULL;
static GtkWidget *verse_label = NULL;

static gboolean advance(gpointer u) {
    (void)u;
    char w[128], v[256];
    /* poke a fresh consultation, then display it */
    system("/usr/bin/templeos --once >/dev/null 2>&1");
    if (feed_word(w, sizeof(w), v, sizeof(v))) {
        gtk_label_set_text(GTK_LABEL(word_label), w);
        gtk_label_set_text(GTK_LABEL(verse_label), v);
    }
    return G_SOURCE_CONTINUE;
}

static gboolean wake(GtkWidget *w, GdkEvent *e, gpointer u) {
    (void)w; (void)e; (void)u;
    gtk_main_quit();
    return FALSE;
}

int main(int argc, char **argv) {
    gtk_init(&argc, &argv);

    GtkWidget *win = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(win), "ALI Shrine");
    gtk_window_fullscreen(GTK_WINDOW(win));
    gtk_widget_set_name(win, "ali-shrine");
    g_signal_connect(win, "destroy", G_CALLBACK(gtk_main_quit), NULL);
    g_signal_connect(win, "key-press-event", G_CALLBACK(wake), NULL);
    g_signal_connect(win, "button-press-event", G_CALLBACK(wake), NULL);

    GtkCssProvider *css = gtk_css_provider_new();
    gtk_css_provider_load_from_data(css,
        "#ali-shrine { background-color: #000000; }"
        "#ali-shrine label { color: #ffffff; }", -1, NULL);
    gtk_style_context_add_provider_for_screen(gdk_screen_get_default(),
        GTK_STYLE_PROVIDER(css), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);

    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 24);
    gtk_container_set_border_width(GTK_CONTAINER(box), 60);
    GtkWidget *align = gtk_alignment_new(0.5, 0.5, 0, 0);
    gtk_container_add(GTK_CONTAINER(win), align);
    gtk_container_add(GTK_CONTAINER(align), box);

    word_label = gtk_label_new("ALI");
    gtk_widget_set_name(word_label, "shrine-word");
    PangoAttrList *attrs = pango_attr_list_new();
    pango_attr_list_insert(attrs, pango_attr_size_new(72 * PANGO_SCALE));
    pango_attr_list_insert(attrs, pango_attr_weight_new(PANGO_WEIGHT_BOLD));
    gtk_label_set_attributes(GTK_LABEL(word_label), attrs);
    pango_attr_list_unref(attrs);
    gtk_box_pack_start(GTK_BOX(box), word_label, FALSE, FALSE, 0);

    verse_label = gtk_label_new("");
    gtk_box_pack_start(GTK_BOX(box), verse_label, FALSE, FALSE, 0);

    advance(NULL);
    g_timeout_add_seconds(12, advance, NULL);

    gtk_widget_show_all(win);
    /* grab input so any key wakes */
    gtk_widget_add_events(win, GDK_KEY_PRESS_MASK | GDK_BUTTON_PRESS_MASK);
    gtk_main();
    return 0;
}
