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
static gchar *selected_path = NULL;

static void update_tree_view(GtkListStore *store) {
    GDir *d;
    const gchar *name;
    gchar full[1024];

    gtk_list_store_clear(store);

    /* 1. Go up entry */
    GtkTreeIter iter;
    gtk_list_store_append(store, &iter);
    gchar *parent_path = g_strdup_printf("%s/..", current_dir);
    gtk_list_store_set(store, &iter, COL_NAME, "../", COL_PATH, parent_path, -1);
    g_free(parent_path);

    /* 2. Read directory entries */
    d = g_dir_open(current_dir, 0, NULL);
    if (!d) return;

    GList *file_list = NULL;
    while ((name = g_dir_read_name(d))) {
        snprintf(full, sizeof(full), "%s/%s", current_dir, name);
        struct stat st;
        if (stat(full, &st) == 0 &&
            (S_ISDIR(st.st_mode) ||
             g_str_has_suffix(name, ".gb") ||
             g_str_has_suffix(name, ".gbc") ||
             g_str_has_suffix(name, ".GB") ||
             g_str_has_suffix(name, ".GBC"))) {
            file_list = g_list_prepend(file_list, g_strdup(name));
        }
    }
    g_dir_close(d);

    file_list = g_list_sort(file_list, (GCompareFunc)g_ascii_strcasecmp);

    for (GList *l = file_list; l != NULL; l = l->next) {
        name = (const gchar*)l->data;
        snprintf(full, sizeof(full), "%s/%s", current_dir, name);

        struct stat st;
        gboolean is_dir = (stat(full, &st) == 0 && S_ISDIR(st.st_mode));
        gchar *disp_name = is_dir ? g_strdup_printf("[%s]", name) : g_strdup(name);

        gtk_list_store_append(store, &iter);
        gtk_list_store_set(store, &iter, COL_NAME, disp_name, COL_PATH, full, -1);

        g_free(disp_name);
        g_free(l->data);
    }
    g_list_free(file_list);
}

char *simple_file_chooser_list(GtkWindow *parent) {
    GtkWidget *dlg, *sw, *tree;
    GtkListStore *store;
    GtkCellRenderer *renderer;
    GtkTreeViewColumn *column;

    if (!current_dir) {
        if (g_file_test("/mnt/us/roms", G_FILE_TEST_IS_DIR)) {
            current_dir = g_strdup("/mnt/us/roms");
        } else if (g_file_test("/mnt/us", G_FILE_TEST_IS_DIR)) {
            current_dir = g_strdup("/mnt/us");
        } else {
            current_dir = g_get_current_dir();
        }
    }

    g_free(selected_path);
    selected_path = NULL;

    dlg = gtk_dialog_new_with_buttons(
        "L:A_N:application_ID:filepicker",
        parent,
        GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
        GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
        NULL
    );

    sw = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sw), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
    gtk_widget_set_size_request(sw, 450, 600);

    store = gtk_list_store_new(NUM_COLS, G_TYPE_STRING, G_TYPE_STRING);
    tree = gtk_tree_view_new_with_model(GTK_TREE_MODEL(store));
    g_object_unref(store);

    renderer = gtk_cell_renderer_text_new();
    g_object_set(renderer, "font", "Sans 14", NULL);
    column = gtk_tree_view_column_new_with_attributes("Select ROM", renderer, "text", COL_NAME, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(tree), column);

    gtk_container_add(GTK_CONTAINER(sw), tree);
    gtk_box_pack_start(GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(dlg))), sw, TRUE, TRUE, 0);

    update_tree_view(store);
    gtk_widget_show_all(dlg);

    /* Event loop for file chooser */
    gboolean done = FALSE;
    gulong handler_id = g_signal_connect_swapped(tree, "row-activated",
        G_CALLBACK(gtk_dialog_response), dlg);

    while (!done) {
        gint resp = gtk_dialog_run(GTK_DIALOG(dlg));
        if (resp == GTK_RESPONSE_CANCEL || resp == GTK_RESPONSE_DELETE_EVENT) {
            break;
        }

        /* Check selection */
        GtkTreeSelection *sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(tree));
        GtkTreeModel *model;
        GtkTreeIter iter;
        if (gtk_tree_selection_get_selected(sel, &model, &iter)) {
            gchar *path = NULL;
            gtk_tree_model_get(model, &iter, COL_PATH, &path, -1);
            if (path) {
                struct stat st;
                if (stat(path, &st) == 0 && S_ISDIR(st.st_mode)) {
                    g_free(current_dir);
                    current_dir = g_strdup(path);
                    update_tree_view(store);
                    g_free(path);
                    continue;
                }
                selected_path = path;
                done = TRUE;
            }
        }
    }

    gtk_widget_destroy(dlg);
    return selected_path;
}
