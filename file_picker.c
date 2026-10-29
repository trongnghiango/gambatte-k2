#include <gtk/gtk.h>
#include <dirent.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <string.h>
#include "file_picker.h"

enum {
    COL_NAME,
    COL_PATH,
    NUM_COLS
};

static gchar *current_dir = NULL;
static GtkListStore *global_rom_store = NULL;
static GtkWidget *global_path_lbl = NULL;
static void (*global_on_selected_cb)(const char *path) = NULL;
static void (*global_on_back_cb)(void) = NULL;

static void update_tree_view(GtkListStore *store) {
    if (!store) return;
    GDir *d;
    const gchar *name;
    gchar full[1024];

    if (!current_dir) {
        if (g_file_test("/mnt/us/roms", G_FILE_TEST_IS_DIR)) {
            current_dir = g_strdup("/mnt/us/roms");
        } else if (g_file_test("/mnt/us", G_FILE_TEST_IS_DIR)) {
            current_dir = g_strdup("/mnt/us");
        } else {
            current_dir = g_get_current_dir();
        }
    }

    if (global_path_lbl) {
        gchar *path_text = g_strdup_printf("Folder: %s", current_dir);
        gtk_label_set_text(GTK_LABEL(global_path_lbl), path_text);
        g_free(path_text);
    }

    gtk_list_store_clear(store);

    /* 1. Go up entry */
    GtkTreeIter iter;
    gtk_list_store_append(store, &iter);
    gchar *parent_path = g_strdup_printf("%s/..", current_dir);
    gtk_list_store_set(store, &iter, COL_NAME, "[..] (Go Up)", COL_PATH, parent_path, -1);
    g_free(parent_path);

    /* 2. Read directory entries */
    d = g_dir_open(current_dir, 0, NULL);
    if (!d) return;

    GList *dir_list = NULL;
    GList *rom_list = NULL;

    while ((name = g_dir_read_name(d))) {
        snprintf(full, sizeof(full), "%s/%s", current_dir, name);
        struct stat st;
        if (stat(full, &st) == 0) {
            if (S_ISDIR(st.st_mode)) {
                if (name[0] != '.') {
                    dir_list = g_list_prepend(dir_list, g_strdup(name));
                }
            } else if (g_str_has_suffix(name, ".gb") ||
                       g_str_has_suffix(name, ".gbc") ||
                       g_str_has_suffix(name, ".GB") ||
                       g_str_has_suffix(name, ".GBC")) {
                rom_list = g_list_prepend(rom_list, g_strdup(name));
            }
        }
    }
    g_dir_close(d);

    dir_list = g_list_sort(dir_list, (GCompareFunc)g_ascii_strcasecmp);
    rom_list = g_list_sort(rom_list, (GCompareFunc)g_ascii_strcasecmp);

    /* Append directories first */
    for (GList *l = dir_list; l != NULL; l = l->next) {
        name = (const gchar*)l->data;
        snprintf(full, sizeof(full), "%s/%s", current_dir, name);
        gchar *disp_name = g_strdup_printf("[DIR]  %s", name);

        gtk_list_store_append(store, &iter);
        gtk_list_store_set(store, &iter, COL_NAME, disp_name, COL_PATH, full, -1);

        g_free(disp_name);
        g_free(l->data);
    }
    g_list_free(dir_list);

    /* Append ROM files */
    for (GList *l = rom_list; l != NULL; l = l->next) {
        name = (const gchar*)l->data;
        snprintf(full, sizeof(full), "%s/%s", current_dir, name);
        gchar *disp_name = g_strdup(name);

        gtk_list_store_append(store, &iter);
        gtk_list_store_set(store, &iter, COL_NAME, disp_name, COL_PATH, full, -1);

        g_free(disp_name);
        g_free(l->data);
    }
    g_list_free(rom_list);
}

void refresh_rom_picker_view(void) {
    if (global_rom_store) {
        update_tree_view(global_rom_store);
    }
}

static void on_rom_row_chosen(const char *filepath) {
    struct stat st;
    if (stat(filepath, &st) == 0 && S_ISDIR(st.st_mode)) {
        g_free(current_dir);
        current_dir = g_strdup(filepath);
        if (global_rom_store) {
            update_tree_view(global_rom_store);
        }
    } else {
        if (global_on_selected_cb) {
            global_on_selected_cb(filepath);
        }
    }
}

static gboolean on_tree_button_release(GtkWidget *widget, GdkEventButton *event, gpointer user_data) {
    if (event->type != GDK_BUTTON_RELEASE || event->button != 1) return FALSE;
    GtkTreeView *tree_view = GTK_TREE_VIEW(widget);
    GtkTreePath *path = NULL;
    GtkTreeViewColumn *col = NULL;

    if (gtk_tree_view_get_path_at_pos(tree_view, (gint)event->x, (gint)event->y, &path, &col, NULL, NULL)) {
        GtkTreeModel *model = gtk_tree_view_get_model(tree_view);
        GtkTreeIter iter;
        if (gtk_tree_model_get_iter(model, &iter, path)) {
            gchar *filepath = NULL;
            gtk_tree_model_get(model, &iter, COL_PATH, &filepath, -1);
            if (filepath) {
                on_rom_row_chosen(filepath);
                g_free(filepath);
            }
        }
        gtk_tree_path_free(path);
        return TRUE;
    }
    return FALSE;
}

static void on_tree_row_activated(GtkTreeView *tree_view, GtkTreePath *path, GtkTreeViewColumn *column, gpointer user_data) {
    GtkTreeModel *model = gtk_tree_view_get_model(tree_view);
    GtkTreeIter iter;
    if (gtk_tree_model_get_iter(model, &iter, path)) {
        gchar *filepath = NULL;
        gtk_tree_model_get(model, &iter, COL_PATH, &filepath, -1);
        if (filepath) {
            on_rom_row_chosen(filepath);
            g_free(filepath);
        }
    }
}

static void on_back_clicked(GtkWidget *btn, gpointer user_data) {
    if (global_on_back_cb) {
        global_on_back_cb();
    }
}

GtkWidget *create_rom_picker_view(void (*on_selected)(const char *path), void (*on_back)(void)) {
    global_on_selected_cb = on_selected;
    global_on_back_cb = on_back;

    GtkWidget *vbox = gtk_vbox_new(FALSE, 8);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 16);

    /* Header Bar */
    GtkWidget *header_hbox = gtk_hbox_new(FALSE, 10);
    GtkWidget *title_lbl = gtk_label_new("SELECT GAME ROM");
    PangoFontDescription *tfont = pango_font_description_from_string("Sans Bold 20");
    gtk_widget_modify_font(title_lbl, tfont);
    pango_font_description_free(tfont);
    gtk_box_pack_start(GTK_BOX(header_hbox), title_lbl, FALSE, FALSE, 4);

    GtkWidget *back_btn = gtk_button_new_with_label("  < Back to Game  ");
    PangoFontDescription *bfont = pango_font_description_from_string("Sans Bold 14");
    gtk_widget_modify_font(back_btn, bfont);
    pango_font_description_free(bfont);
    g_signal_connect(back_btn, "clicked", G_CALLBACK(on_back_clicked), NULL);
    gtk_box_pack_end(GTK_BOX(header_hbox), back_btn, FALSE, FALSE, 4);

    gtk_box_pack_start(GTK_BOX(vbox), header_hbox, FALSE, FALSE, 6);

    /* Current Folder Label */
    global_path_lbl = gtk_label_new("Folder: /mnt/us/roms");
    PangoFontDescription *pfont = pango_font_description_from_string("Sans 14");
    gtk_widget_modify_font(global_path_lbl, pfont);
    pango_font_description_free(pfont);
    gtk_misc_set_alignment(GTK_MISC(global_path_lbl), 0.0, 0.5);
    gtk_box_pack_start(GTK_BOX(vbox), global_path_lbl, FALSE, FALSE, 2);

    GtkWidget *sep = gtk_hseparator_new();
    gtk_box_pack_start(GTK_BOX(vbox), sep, FALSE, FALSE, 4);

    /* Scrolled Window with TreeView */
    GtkWidget *sw = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sw), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);

    global_rom_store = gtk_list_store_new(NUM_COLS, G_TYPE_STRING, G_TYPE_STRING);
    GtkWidget *tree = gtk_tree_view_new_with_model(GTK_TREE_MODEL(global_rom_store));
    g_object_unref(global_rom_store);

    GtkCellRenderer *renderer = gtk_cell_renderer_text_new();
    g_object_set(renderer, "font", "Sans Bold 16", "ypad", 12, NULL);
    GtkTreeViewColumn *column = gtk_tree_view_column_new_with_attributes("Game Files", renderer, "text", COL_NAME, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(tree), column);

    g_signal_connect(tree, "button-release-event", G_CALLBACK(on_tree_button_release), NULL);
    g_signal_connect(tree, "row-activated", G_CALLBACK(on_tree_row_activated), NULL);

    gtk_container_add(GTK_CONTAINER(sw), tree);
    gtk_box_pack_start(GTK_BOX(vbox), sw, TRUE, TRUE, 4);

    update_tree_view(global_rom_store);

    return vbox;
}
