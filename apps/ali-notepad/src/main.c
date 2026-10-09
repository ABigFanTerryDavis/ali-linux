#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ALI Notepad v2 (1.3.5) - C + GTK3, open/save/find, TR via --tr/ALI_LANG.
 * Title shows "ALI Notepad" everywhere.
 */

static int LANG_TR = 0;
static const char *T(const char *en, const char *tr) { return LANG_TR ? tr : en; }

static GtkWidget *textview;
static char *current_file = NULL;
static char *last_find = NULL;
static GtkWidget *notebook = NULL;

/* forward declarations: helpers are used before they are defined */
static void set_title(GtkWindow *win);
static void set_current_file(GtkWindow *win, const char *path);
static void set_page_title(GtkWidget *page);
static void sync_current(void);
static void remember_recent(const char *path);
static void new_tab(GtkWindow *win, const char *path);
static void find_next(GtkWindow *win);

static GtkWidget *current_page(void) {
    if (!notebook) return NULL;
    return gtk_notebook_get_nth_page(GTK_NOTEBOOK(notebook),
        gtk_notebook_get_current_page(GTK_NOTEBOOK(notebook)));
}

static void sync_current(void) {
    GtkWidget *page = current_page();
    if (!page) { textview = NULL; return; }
    textview = g_object_get_data(G_OBJECT(page), "ali-view");
    char *f = g_object_get_data(G_OBJECT(page), "ali-file");
    free(current_file);
    current_file = f ? strdup(f) : NULL;
}

static void set_page_title(GtkWidget *page) {
    char *f = g_object_get_data(G_OBJECT(page), "ali-file");
    const char *base = f ? strrchr(f, '/') : NULL;
    gtk_notebook_set_tab_label_text(GTK_NOTEBOOK(notebook), page,
        (f && base) ? base + 1 : (f ? f : T("Untitled", "Başlıksız")));
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

static void set_title(GtkWindow *win) {
    char t[512];
    if (current_file)
        snprintf(t, sizeof(t), "%s - ALI Notepad", current_file);
    else
        snprintf(t, sizeof(t), T("Untitled - ALI Notepad", "Başlıksız - ALI Notepad"));
    gtk_window_set_title(win, t);
}

static void set_current_file(GtkWindow *win, const char *path) {
    GtkWidget *page = current_page();
    if (page) {
        g_object_set_data_full(G_OBJECT(page), "ali-file",
            path ? g_strdup(path) : NULL, g_free);
        set_page_title(page);
    }
    free(current_file);
    current_file = path ? strdup(path) : NULL;
    set_title(win);
}

static void load_file(const char *path, GtkWindow *win) {
    FILE *f = fopen(path, "rb");
    if (!f) {
        GtkWidget *d = gtk_message_dialog_new(win, GTK_DIALOG_MODAL,
            GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE, T("Cannot open:\n%s", "Açılamadı:\n%s"), path);
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
    set_current_file(win, path);
    remember_recent(path);
}

static int save_file(const char *path, GtkWindow *win) {
    GtkTextBuffer *tb = gtk_text_view_get_buffer(GTK_TEXT_VIEW(textview));
    GtkTextIter s, e;
    gtk_text_buffer_get_bounds(tb, &s, &e);
    char *txt = gtk_text_buffer_get_text(tb, &s, &e, FALSE);
    FILE *f = fopen(path, "wb");
    if (!f) {
        GtkWidget *d = gtk_message_dialog_new(win, GTK_DIALOG_MODAL,
            GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE, T("Cannot save:\n%s", "Kaydedilemedi:\n%s"), path);
        gtk_dialog_run(GTK_DIALOG(d));
        gtk_widget_destroy(d);
        g_free(txt);
        return 0;
    }
    fwrite(txt, 1, strlen(txt), f);
    fclose(f);
    g_free(txt);
    set_current_file(win, path);
    remember_recent(path);
    GtkTextBuffer *tb2 = gtk_text_view_get_buffer(GTK_TEXT_VIEW(textview));
    gtk_text_buffer_set_modified(tb2, FALSE);
    return 1;
}

static void new_tab(GtkWindow *win, const char *path) {
    GtkWidget *scroll = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll),
        GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    GtkWidget *view = gtk_text_view_new();
    gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(view), GTK_WRAP_WORD_CHAR);
    gtk_container_add(GTK_CONTAINER(scroll), view);
    g_object_set_data(G_OBJECT(scroll), "ali-view", view);
    textview = view;
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), scroll,
        gtk_label_new(T("Untitled", "Başlıksız")));
    gtk_widget_show_all(scroll);
    gtk_notebook_set_current_page(GTK_NOTEBOOK(notebook),
        gtk_notebook_get_n_pages(GTK_NOTEBOOK(notebook)) - 1);
    if (path) load_file(path, win);
    else set_current_file(win, NULL);
}

static void on_switch(GtkNotebook *nb, GtkWidget *page, guint n, gpointer u) {
    (void)nb; (void)page; (void)n;
    sync_current();
    set_title(GTK_WINDOW(u));
}

static void on_new(GtkMenuItem *m, gpointer u) {
    (void)m;
    new_tab(GTK_WINDOW(u), NULL);
}

static void on_open(GtkMenuItem *m, gpointer u) {
    (void)m;
    GtkWindow *win = GTK_WINDOW(u);
    GtkWidget *d = gtk_file_chooser_dialog_new(T("Open - ALI Notepad", "Aç - ALI Notepad"), win,
        GTK_FILE_CHOOSER_ACTION_OPEN,
        T("_Cancel", "_Vazgeç"), GTK_RESPONSE_CANCEL, T("_Open", "_Aç"), GTK_RESPONSE_ACCEPT, NULL);
    if (gtk_dialog_run(GTK_DIALOG(d)) == GTK_RESPONSE_ACCEPT) {
        char *p = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(d));
        new_tab(win, p);
        g_free(p);
    }
    gtk_widget_destroy(d);
}

static void remember_recent(const char *path) {
    char dir[512], list[512];
    snprintf(dir, sizeof(dir), "%s/.config/ali-notepad", g_get_home_dir());
    g_mkdir_with_parents(dir, 0700);
    snprintf(list, sizeof(list), "%s/recent", dir);
    /* prepend, dedupe, keep 5 */
    char *old = NULL;
    g_file_get_contents(list, &old, NULL, NULL);
    GString *s = g_string_new(path);
    g_string_append_c(s, '\n');
    if (old) {
        char **lines = g_strsplit(old, "\n", -1);
        int kept = 0;
        for (int i = 0; lines[i] && kept < 4; i++) {
            if (lines[i][0] && strcmp(lines[i], path) != 0) {
                g_string_append(s, lines[i]);
                g_string_append_c(s, '\n');
                kept++;
            }
        }
        g_strfreev(lines);
        g_free(old);
    }
    g_file_set_contents(list, s->str, -1, NULL);
    g_string_free(s, TRUE);
}

static void on_recent(GtkMenuItem *m, gpointer u) {
    const char *path = (const char *)g_object_get_data(G_OBJECT(m), "ali-path");
    if (!path) return;
    if (!g_file_test(path, G_FILE_TEST_EXISTS)) return;
    new_tab(GTK_WINDOW(u), path);
}

static void build_recent_menu(GtkWidget *fmenu, gpointer win) {
    char list[512];
    snprintf(list, sizeof(list), "%s/.config/ali-notepad/recent", g_get_home_dir());
    char *content = NULL;
    if (!g_file_get_contents(list, &content, NULL, NULL)) return;
    GtkWidget *rm = gtk_menu_new();
    GtkWidget *ri = gtk_menu_item_new_with_label(T("Recent", "Son Açılanlar"));
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(ri), rm);
    char **lines = g_strsplit(content, "\n", -1);
    int n = 0;
    for (int i = 0; lines[i] && n < 5; i++) {
        if (!lines[i][0]) continue;
        const char *base = strrchr(lines[i], '/');
        GtkWidget *it = gtk_menu_item_new_with_label(base ? base + 1 : lines[i]);
        g_object_set_data_full(G_OBJECT(it), "ali-path", g_strdup(lines[i]), g_free);
        g_signal_connect(it, "activate", G_CALLBACK(on_recent), win);
        gtk_menu_shell_append(GTK_MENU_SHELL(rm), it);
        n++;
    }
    g_strfreev(lines);
    g_free(content);
    if (n > 0)
        gtk_menu_shell_append(GTK_MENU_SHELL(fmenu), ri);
}

static void on_close_tab(GtkMenuItem *m, gpointer u) {
    (void)m;
    GtkWindow *win = GTK_WINDOW(u);
    GtkWidget *page = current_page();
    if (!page) return;
    GtkTextBuffer *tb = gtk_text_view_get_buffer(GTK_TEXT_VIEW(textview));
    if (textview && gtk_text_buffer_get_modified(tb)) {
        GtkWidget *d = gtk_message_dialog_new(win, GTK_DIALOG_MODAL,
            GTK_MESSAGE_QUESTION, GTK_BUTTONS_YES_NO,
            T("Close without saving?", "Kaydetmeden kapatılsın mı?"));
        int r = gtk_dialog_run(GTK_DIALOG(d));
        gtk_widget_destroy(d);
        if (r != GTK_RESPONSE_YES) return;
    }
    int n = gtk_notebook_page_num(GTK_NOTEBOOK(notebook), page);
    gtk_notebook_remove_page(GTK_NOTEBOOK(notebook), n);
    if (gtk_notebook_get_n_pages(GTK_NOTEBOOK(notebook)) == 0)
        new_tab(win, NULL);
    else {
        sync_current();
        set_title(win);
    }
}

static void on_save(GtkMenuItem *m, gpointer u) {
    (void)m;
    GtkWindow *win = GTK_WINDOW(u);
    if (current_file) { save_file(current_file, win); return; }
    GtkWidget *d = gtk_file_chooser_dialog_new(T("Save - ALI Notepad", "Kaydet - ALI Notepad"), win,
        GTK_FILE_CHOOSER_ACTION_SAVE,
        T("_Cancel", "_Vazgeç"), GTK_RESPONSE_CANCEL, T("_Save", "_Kaydet"), GTK_RESPONSE_ACCEPT, NULL);
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
    GtkWidget *d = gtk_file_chooser_dialog_new(T("Save As - ALI Notepad", "Farklı Kaydet - ALI Notepad"), win,
        GTK_FILE_CHOOSER_ACTION_SAVE,
        T("_Cancel", "_Vazgeç"), GTK_RESPONSE_CANCEL, T("_Save", "_Kaydet"), GTK_RESPONSE_ACCEPT, NULL);
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
        "ALI Notepad v2\nC + GTK3 for ALI Linux.");
    gtk_dialog_run(GTK_DIALOG(d));
    gtk_widget_destroy(d);
}

static void find_next(GtkWindow *win) {
    GtkTextBuffer *tb = gtk_text_view_get_buffer(GTK_TEXT_VIEW(textview));
    const char *q = last_find;
    char *tmp = NULL;
    if (!q) {
        GtkWidget *d = gtk_dialog_new_with_buttons(T("Find", "Bul"), win,
            GTK_DIALOG_MODAL, T("_Cancel", "_Vazgeç"), GTK_RESPONSE_CANCEL,
            T("_Find", "_Bul"), GTK_RESPONSE_ACCEPT, NULL);
        GtkWidget *e = gtk_entry_new();
        gtk_container_add(GTK_CONTAINER(gtk_dialog_get_content_area(GTK_DIALOG(d))), e);
        gtk_widget_show_all(d);
        if (gtk_dialog_run(GTK_DIALOG(d)) == GTK_RESPONSE_ACCEPT) {
            tmp = g_strdup(gtk_entry_get_text(GTK_ENTRY(e)));
            q = tmp;
        }
        gtk_widget_destroy(d);
        if (!q || !*q) { g_free(tmp); return; }
        free(last_find);
        last_find = strdup(q);
    }
    GtkTextIter ins, mstart, mend;
    gtk_text_buffer_get_iter_at_mark(tb, &ins, gtk_text_buffer_get_insert(tb));
    if (!gtk_text_iter_forward_search(&ins, last_find, GTK_TEXT_SEARCH_TEXT_ONLY, &mstart, &mend, NULL)) {
        gtk_text_buffer_get_start_iter(tb, &ins);
        if (!gtk_text_iter_forward_search(&ins, last_find, GTK_TEXT_SEARCH_TEXT_ONLY, &mstart, &mend, NULL)) {
            GtkWidget *d = gtk_message_dialog_new(win, GTK_DIALOG_MODAL,
                GTK_MESSAGE_INFO, GTK_BUTTONS_CLOSE,
                T("Not found: %s", "Bulunamadı: %s"), last_find);
            gtk_dialog_run(GTK_DIALOG(d));
            gtk_widget_destroy(d);
            g_free(tmp);
            return;
        }
    }
    gtk_text_buffer_select_range(tb, &mstart, &mend);
    gtk_text_view_scroll_to_iter(GTK_TEXT_VIEW(textview), &mstart, 0.0, FALSE, 0, 0);
    g_free(tmp);
}

static void on_find(GtkMenuItem *m, gpointer u) {
    (void)m;
    free(last_find); last_find = NULL;
    find_next(GTK_WINDOW(u));
}

static void on_find_next(GtkMenuItem *m, gpointer u) {
    (void)m;
    find_next(GTK_WINDOW(u));
}

static void on_print_draw(GtkPrintOperation *op, GtkPrintContext *ctx,
    gint page, gpointer u) {
    (void)op; (void)page;
    GtkTextView *view = GTK_TEXT_VIEW(u);
    GtkTextBuffer *tb = gtk_text_view_get_buffer(view);
    GtkTextIter s, e;
    gtk_text_buffer_get_bounds(tb, &s, &e);
    char *txt = gtk_text_buffer_get_text(tb, &s, &e, FALSE);
    PangoLayout *lo = gtk_print_context_create_pango_layout(ctx);
    pango_layout_set_font_description(lo,
        pango_font_description_from_string("Monospace 10"));
    pango_layout_set_text(lo, txt ? txt : "", -1);
    g_free(txt);
    cairo_t *cr = gtk_print_context_get_cairo_context(ctx);
    cairo_move_to(cr, 40, 40);
    pango_cairo_show_layout(cr, lo);
    g_object_unref(lo);
}

static void on_print(GtkMenuItem *m, gpointer u) {
    (void)m;
    GtkWindow *win = GTK_WINDOW(u);
    if (!textview) return;
    GtkPrintOperation *op = gtk_print_operation_new();
    gtk_print_operation_set_job_name(op, T("ALI Notepad", "ALI Notepad"));
    g_signal_connect(op, "draw-page", G_CALLBACK(on_print_draw), textview);
    gtk_print_operation_run(op, GTK_PRINT_OPERATION_ACTION_PRINT_DIALOG,
        win, NULL);
    g_object_unref(op);
}

int main(int argc, char **argv) {
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--tr") == 0) { LANG_TR = 1; break; }
    }
    if (!LANG_TR) {
        const char *env = getenv("ALI_LANG");
        if (env && (strcmp(env, "tr") == 0 || strcmp(env, "TR") == 0)) LANG_TR = 1;
    }
    /* --tr is a flag, not a file */
    if (LANG_TR && argc > 1 && strcmp(argv[argc - 1], "--tr") == 0) argc--;
    gtk_init(&argc, &argv);
    ali_style();

    /* open file from command line: ali-notepad file.txt (flags skipped below) */

    GtkWidget *win = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_icon_name(GTK_WINDOW(win), "ali-notepad");
    gtk_window_set_default_size(GTK_WINDOW(win), 800, 600);
    gtk_window_set_position(GTK_WINDOW(win), GTK_WIN_POS_CENTER);
    g_signal_connect(win, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_container_add(GTK_CONTAINER(win), vbox);

    GtkWidget *menubar = gtk_menu_bar_new();
    GtkWidget *mfile = gtk_menu_item_new_with_label(T("File", "Dosya"));
    GtkWidget *medit = gtk_menu_item_new_with_label(T("Edit", "Düzenle"));
    GtkWidget *mhelp = gtk_menu_item_new_with_label(T("Help", "Yardım"));
    GtkWidget *fmenu = gtk_menu_new();
    GtkWidget *emenu = gtk_menu_new();
    GtkWidget *hmenu = gtk_menu_new();
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(mfile), fmenu);
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(medit), emenu);
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(mhelp), hmenu);
    gtk_menu_shell_append(GTK_MENU_SHELL(menubar), mfile);
    gtk_menu_shell_append(GTK_MENU_SHELL(menubar), medit);
    gtk_menu_shell_append(GTK_MENU_SHELL(menubar), mhelp);

    GtkWidget *i_new = gtk_menu_item_new_with_label(T("New", "Yeni"));
    GtkWidget *i_open = gtk_menu_item_new_with_label(T("Open...", "Aç..."));
    GtkWidget *i_save = gtk_menu_item_new_with_label(T("Save", "Kaydet"));
    GtkWidget *i_saveas = gtk_menu_item_new_with_label(T("Save As...", "Farklı Kaydet..."));
    GtkWidget *i_quit = gtk_menu_item_new_with_label(T("Quit", "Çık"));
    GtkWidget *i_print = gtk_menu_item_new_with_label(T("Print...", "Yazdır..."));
    GtkWidget *i_close = gtk_menu_item_new_with_label(T("Close Tab", "Sekmeyi Kapat"));
    GtkWidget *i_find = gtk_menu_item_new_with_label(T("Find...", "Bul..."));
    GtkWidget *i_next = gtk_menu_item_new_with_label(T("Find Next", "Sonrakini Bul"));
    GtkWidget *i_about = gtk_menu_item_new_with_label(T("About", "Hakkında"));
    gtk_menu_shell_append(GTK_MENU_SHELL(fmenu), i_new);
    gtk_menu_shell_append(GTK_MENU_SHELL(fmenu), i_open);
    gtk_menu_shell_append(GTK_MENU_SHELL(fmenu), i_save);
    gtk_menu_shell_append(GTK_MENU_SHELL(fmenu), i_saveas);
    gtk_menu_shell_append(GTK_MENU_SHELL(fmenu), gtk_separator_menu_item_new());
    gtk_menu_shell_append(GTK_MENU_SHELL(fmenu), i_print);
    gtk_menu_shell_append(GTK_MENU_SHELL(fmenu), i_close);
    gtk_menu_shell_append(GTK_MENU_SHELL(fmenu), i_quit);
    build_recent_menu(fmenu, win);
    gtk_menu_shell_append(GTK_MENU_SHELL(emenu), i_find);
    gtk_menu_shell_append(GTK_MENU_SHELL(emenu), i_next);
    gtk_menu_shell_append(GTK_MENU_SHELL(hmenu), i_about);

    g_signal_connect(i_new, "activate", G_CALLBACK(on_new), win);
    g_signal_connect(i_find, "activate", G_CALLBACK(on_find), win);
    g_signal_connect(i_next, "activate", G_CALLBACK(on_find_next), win);
    g_signal_connect(i_open, "activate", G_CALLBACK(on_open), win);
    g_signal_connect(i_save, "activate", G_CALLBACK(on_save), win);
    g_signal_connect(i_saveas, "activate", G_CALLBACK(on_saveas), win);
    g_signal_connect(i_print, "activate", G_CALLBACK(on_print), win);
    g_signal_connect(i_close, "activate", G_CALLBACK(on_close_tab), win);
    g_signal_connect(i_quit, "activate", G_CALLBACK(gtk_main_quit), NULL);
    g_signal_connect(i_about, "activate", G_CALLBACK(on_about), win);

    gtk_box_pack_start(GTK_BOX(vbox), menubar, FALSE, FALSE, 0);

    notebook = gtk_notebook_new();
    gtk_notebook_set_scrollable(GTK_NOTEBOOK(notebook), TRUE);
    g_signal_connect(notebook, "switch-page", G_CALLBACK(on_switch), win);
    gtk_box_pack_start(GTK_BOX(vbox), notebook, TRUE, TRUE, 0);

    /* accelerators: Ctrl+N/T/O/S/W/F */
    GtkAccelGroup *ag = gtk_accel_group_new();
    gtk_window_add_accel_group(GTK_WINDOW(win), ag);
    gtk_widget_add_accelerator(i_new, "activate", ag, GDK_KEY_n, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
    gtk_widget_add_accelerator(i_new, "activate", ag, GDK_KEY_t, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
    gtk_widget_add_accelerator(i_open, "activate", ag, GDK_KEY_o, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
    gtk_widget_add_accelerator(i_save, "activate", ag, GDK_KEY_s, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
    gtk_widget_add_accelerator(i_close, "activate", ag, GDK_KEY_w, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
    gtk_widget_add_accelerator(i_find, "activate", ag, GDK_KEY_f, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
    gtk_widget_add_accelerator(i_print, "activate", ag, GDK_KEY_p, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);

    /* one tab per file argument (ali-notepad a.txt b.txt), else blank */
    {
        int opened = 0;
        for (int i = 1; i < argc; i++) {
            if (strcmp(argv[i], "--tr") == 0) continue;
            if (argv[i][0] == '-') continue;
            new_tab(GTK_WINDOW(win), argv[i]);
            opened++;
        }
        if (!opened) new_tab(GTK_WINDOW(win), NULL);
    }

    gtk_widget_show_all(win);
    gtk_main();
    free(current_file);
    return 0;
}
