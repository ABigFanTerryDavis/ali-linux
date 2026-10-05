#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ALI Notepad v1 - C + GTK3, minimal open/save.
 * Separate app. Title shows "ALI Notepad" everywhere.
 */

static GtkWidget *textview;
static char *current_file = NULL;

static void set_title(GtkWindow *win) {
    char t[512];
    if (current_file)
        snprintf(t, sizeof(t), "%s - ALI Notepad", current_file);
    else
        snprintf(t, sizeof(t), "Untitled - ALI Notepad");
    gtk_window_set_title(win, t);
}

static void load_file(const char *path, GtkWindow *win) {
    FILE *f = fopen(path, "rb");
    if (!f) {
        GtkWidget *d = gtk_message_dialog_new(win, GTK_DIALOG_MODAL,
            GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE, "Cannot open:\n%s", path);
        gtk_dialog_run(GTK_DIALOG(d));
        gtk_widget_destroy(d);
        return;
    }
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    fseek(f, 0, SEEK_SET);
    char *buf = malloc((size_t)(sz < 0 ? 0 : sz) + 1);
    size_t rd = buf ? fread(buf, 1, sz > 0 ? (size_t)sz : 0, f) : 0;
    fclose(f);
    if (!buf) return;
    buf[rd] = '\0';
    GtkTextBuffer *tb = gtk_text_view_get_buffer(GTK_TEXT_VIEW(textview));
    gtk_text_buffer_set_text(tb, buf, -1);
    free(buf);
    free(current_file);
    current_file = g_strdup(path);
    set_title(win);
}

static int save_file(const char *path, GtkWindow *win) {
    GtkTextBuffer *tb = gtk_text_view_get_buffer(GTK_TEXT_VIEW(textview));
    GtkTextIter s, e;
    gtk_text_buffer_get_bounds(tb, &s, &e);
    char *txt = gtk_text_buffer_get_text(tb, &s, &e, FALSE);
    FILE *f = fopen(path, "wb");
    if (!f) {
        GtkWidget *d = gtk_message_dialog_new(win, GTK_DIALOG_MODAL,
            GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE, "Cannot save:\n%s", path);
        gtk_dialog_run(GTK_DIALOG(d));
        gtk_widget_destroy(d);
        g_free(txt);
        return 0;
    }
    fwrite(txt, 1, strlen(txt), f);
    fclose(f);
    g_free(txt);
    free(current_file);
    current_file = g_strdup(path);
    set_title(win);
    return 1;
}

static void on_new(GtkMenuItem *m, gpointer u) {
    (void)m;
    GtkWindow *win = GTK_WINDOW(u);
    GtkTextBuffer *tb = gtk_text_view_get_buffer(GTK_TEXT_VIEW(textview));
    gtk_text_buffer_set_text(tb, "", -1);
    free(current_file); current_file = NULL;
    set_title(win);
}

static void on_open(GtkMenuItem *m, gpointer u) {
    (void)m;
    GtkWindow *win = GTK_WINDOW(u);
    GtkWidget *d = gtk_file_chooser_dialog_new("Open - ALI Notepad", win,
        GTK_FILE_CHOOSER_ACTION_OPEN,
        "_Cancel", GTK_RESPONSE_CANCEL, "_Open", GTK_RESPONSE_ACCEPT, NULL);
    if (gtk_dialog_run(GTK_DIALOG(d)) == GTK_RESPONSE_ACCEPT) {
        char *p = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(d));
        load_file(p, win);
        g_free(p);
    }
    gtk_widget_destroy(d);
}

static void on_save(GtkMenuItem *m, gpointer u) {
    (void)m;
    GtkWindow *win = GTK_WINDOW(u);
    if (current_file) { save_file(current_file, win); return; }
    GtkWidget *d = gtk_file_chooser_dialog_new("Save - ALI Notepad", win,
        GTK_FILE_CHOOSER_ACTION_SAVE,
        "_Cancel", GTK_RESPONSE_CANCEL, "_Save", GTK_RESPONSE_ACCEPT, NULL);
    gtk_file_chooser_set_do_overwrite_confirmation(GTK_FILE_CHOOSER(d), TRUE);
    if (gtk_dialog_run(GTK_DIALOG(d)) == GTK_RESPONSE_ACCEPT) {
        char *p = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(d));
        save_file(p, win);
        g_free(p);
    }
    gtk_widget_destroy(d);
}

static void on_saveas(GtkMenuItem *m, gpointer u) {
    (void)m;
    GtkWindow *win = GTK_WINDOW(u);
    GtkWidget *d = gtk_file_chooser_dialog_new("Save As - ALI Notepad", win,
        GTK_FILE_CHOOSER_ACTION_SAVE,
        "_Cancel", GTK_RESPONSE_CANCEL, "_Save", GTK_RESPONSE_ACCEPT, NULL);
    gtk_file_chooser_set_do_overwrite_confirmation(GTK_FILE_CHOOSER(d), TRUE);
    if (gtk_dialog_run(GTK_DIALOG(d)) == GTK_RESPONSE_ACCEPT) {
        char *p = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(d));
        save_file(p, win);
        g_free(p);
    }
    gtk_widget_destroy(d);
}

static void on_about(GtkMenuItem *m, gpointer u) {
    (void)m;
    GtkWidget *d = gtk_message_dialog_new(GTK_WINDOW(u), GTK_DIALOG_MODAL,
        GTK_MESSAGE_INFO, GTK_BUTTONS_CLOSE,
        "ALI Notepad v1\nC + GTK3 for ALI Linux.");
    gtk_dialog_run(GTK_DIALOG(d));
    gtk_widget_destroy(d);
}

int main(int argc, char **argv) {
    gtk_init(&argc, &argv);

    /* open file from command line: ali-notepad file.txt */
    const char *start_file = (argc > 1) ? argv[1] : NULL;

    GtkWidget *win = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_default_size(GTK_WINDOW(win), 800, 600);
    gtk_window_set_position(GTK_WINDOW(win), GTK_WIN_POS_CENTER);
    g_signal_connect(win, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_container_add(GTK_CONTAINER(win), vbox);

    GtkWidget *menubar = gtk_menu_bar_new();
    GtkWidget *mfile = gtk_menu_item_new_with_label("File");
    GtkWidget *mhelp = gtk_menu_item_new_with_label("Help");
    GtkWidget *fmenu = gtk_menu_new();
    GtkWidget *hmenu = gtk_menu_new();
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(mfile), fmenu);
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(mhelp), hmenu);
    gtk_menu_shell_append(GTK_MENU_SHELL(menubar), mfile);
    gtk_menu_shell_append(GTK_MENU_SHELL(menubar), mhelp);

    GtkWidget *i_new = gtk_menu_item_new_with_label("New");
    GtkWidget *i_open = gtk_menu_item_new_with_label("Open...");
    GtkWidget *i_save = gtk_menu_item_new_with_label("Save");
    GtkWidget *i_saveas = gtk_menu_item_new_with_label("Save As...");
    GtkWidget *i_quit = gtk_menu_item_new_with_label("Quit");
    GtkWidget *i_about = gtk_menu_item_new_with_label("About");
    gtk_menu_shell_append(GTK_MENU_SHELL(fmenu), i_new);
    gtk_menu_shell_append(GTK_MENU_SHELL(fmenu), i_open);
    gtk_menu_shell_append(GTK_MENU_SHELL(fmenu), i_save);
    gtk_menu_shell_append(GTK_MENU_SHELL(fmenu), i_saveas);
    gtk_menu_shell_append(GTK_MENU_SHELL(fmenu), gtk_separator_menu_item_new());
    gtk_menu_shell_append(GTK_MENU_SHELL(fmenu), i_quit);
    gtk_menu_shell_append(GTK_MENU_SHELL(hmenu), i_about);

    g_signal_connect(i_new, "activate", G_CALLBACK(on_new), win);
    g_signal_connect(i_open, "activate", G_CALLBACK(on_open), win);
    g_signal_connect(i_save, "activate", G_CALLBACK(on_save), win);
    g_signal_connect(i_saveas, "activate", G_CALLBACK(on_saveas), win);
    g_signal_connect(i_quit, "activate", G_CALLBACK(gtk_main_quit), NULL);
    g_signal_connect(i_about, "activate", G_CALLBACK(on_about), win);

    gtk_box_pack_start(GTK_BOX(vbox), menubar, FALSE, FALSE, 0);

    GtkWidget *scroll = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll),
        GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    textview = gtk_text_view_new();
    gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(textview), GTK_WRAP_WORD_CHAR);
    gtk_container_add(GTK_CONTAINER(scroll), textview);
    gtk_box_pack_start(GTK_BOX(vbox), scroll, TRUE, TRUE, 0);

    set_title(GTK_WINDOW(win));
    if (start_file) load_file(start_file, GTK_WINDOW(win));

    gtk_widget_show_all(win);
    gtk_main();
    free(current_file);
    return 0;
}
