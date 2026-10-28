/*
 * kindle_compat.c - Compatibility implementation of GameScreen, GameSpeaker,
 * Lipstick, and Ink widgets for gambatte-k2 on Kindle devices.
 */

#include <glib.h>
#include <glib-object.h>
#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdio.h>

/* Forward declare GrayShm native functions from gray_shm.c */
void* gray_shm_create (void* xid, gint width, gint height);
void gray_shm_resize (void* shm, gint w, gint h);
void gray_shm_commit_rect (void* shm, gint x, gint y, gint w, gint h);
uint8_t* gray_shm_get_buffer (void* shm);
void gray_shm_destroy (void* shm);

/* ========================================================================= */
/* 1. InkRowHScale & InkRowToggleButton Widgets                              */
/* ========================================================================= */

typedef struct _InkRowHScale {
    GtkHScale parent_instance;
} InkRowHScale;

typedef struct _InkRowHScaleClass {
    GtkHScaleClass parent_class;
} InkRowHScaleClass;

G_DEFINE_TYPE(InkRowHScale, ink_row_hscale, GTK_TYPE_HSCALE)

static void ink_row_hscale_init(InkRowHScale *self) { (void)self; }
static void ink_row_hscale_class_init(InkRowHScaleClass *klass) { (void)klass; }

void ink_row_hscale_set_lower(InkRowHScale* self, gdouble value) {
    if (self) {
        GtkAdjustment *adj = gtk_range_get_adjustment(GTK_RANGE(self));
        if (adj) {
            gtk_adjustment_set_lower(adj, value);
        }
    }
}

void ink_row_hscale_set_upper(InkRowHScale* self, gdouble value) {
    if (self) {
        GtkAdjustment *adj = gtk_range_get_adjustment(GTK_RANGE(self));
        if (adj) {
            gtk_adjustment_set_upper(adj, value);
        }
    }
}

typedef struct _InkRowToggleButton {
    GtkToggleButton parent_instance;
} InkRowToggleButton;

typedef struct _InkRowToggleButtonClass {
    GtkToggleButtonClass parent_class;
} InkRowToggleButtonClass;

G_DEFINE_TYPE(InkRowToggleButton, ink_row_toggle_button, GTK_TYPE_TOGGLE_BUTTON)

static void ink_row_toggle_button_init(InkRowToggleButton *self) { (void)self; }
static void ink_row_toggle_button_class_init(InkRowToggleButtonClass *klass) { (void)klass; }

/* ========================================================================= */
/* 2. Lipstick (Kindle lipc wrapper)                                         */
/* ========================================================================= */

typedef struct _Lipstick {
    GObject parent_instance;
} Lipstick;

typedef struct _LipstickClass {
    GObjectClass parent_class;
} LipstickClass;

G_DEFINE_TYPE(Lipstick, lipstick, G_TYPE_OBJECT)

static void lipstick_init(Lipstick *self) { (void)self; }
static void lipstick_class_init(LipstickClass *klass) { (void)klass; }

GQuark lipstick_error_quark(void) {
    return g_quark_from_static_string("lipstick-error-quark");
}

Lipstick* lipstick_new(const gchar* service, GError** error) {
    (void)service;
    (void)error;
    return (Lipstick*)g_object_new(lipstick_get_type(), NULL);
}

gchar* lipstick_get_string_property(Lipstick* self, const gchar* service, const gchar* property) {
    (void)self;
    (void)service;
    (void)property;
    return g_strdup("normal");
}

typedef gboolean (*LipstickEventCallback)(gconstpointer value, void* user_data);

void lipstick_subscribe(Lipstick* self, GType g_type, GBoxedCopyFunc g_dup_func, GDestroyNotify g_destroy_func, const gchar* service, const gchar* name, LipstickEventCallback callback, void* callback_target) {
    (void)self; (void)g_type; (void)g_dup_func; (void)g_destroy_func; (void)service; (void)name; (void)callback; (void)callback_target;
}

/* ========================================================================= */
/* 3. GameSpeaker (Kindle Audio Dummy / Safe sink)                           */
/* ========================================================================= */

typedef struct _GameSpeaker {
    GObject parent_instance;
} GameSpeaker;

typedef struct _GameSpeakerClass {
    GObjectClass parent_class;
} GameSpeakerClass;

G_DEFINE_TYPE(GameSpeaker, game_speaker, G_TYPE_OBJECT)

static void game_speaker_init(GameSpeaker *self) { (void)self; }
static void game_speaker_class_init(GameSpeakerClass *klass) { (void)klass; }

GameSpeaker* game_speaker_new(gint rate, gint channels, gint sampleSize) {
    (void)rate; (void)channels; (void)sampleSize;
    return (GameSpeaker*)g_object_new(game_speaker_get_type(), NULL);
}

void game_speaker_start(GameSpeaker* self) { (void)self; }
void game_speaker_stop(GameSpeaker* self) { (void)self; }
void game_speaker_push_frame(GameSpeaker* self, void* data, gsize frames) { (void)self; (void)data; (void)frames; }

/* ========================================================================= */
/* 4. GameScreen (Rendering, Scaling, Dithering)                             */
/* ========================================================================= */

typedef struct _GameScreen GameScreen;
typedef struct _GameScreenClass GameScreenClass;
typedef struct _GameScreenPrivate GameScreenPrivate;

struct _GameScreenPrivate {
    void* shm_handle;
    uint8_t grayscale_lut[65536];
};

struct _GameScreen {
    GObject parent_instance;
    GameScreenPrivate * priv;
    gboolean ready;
    GtkDrawingArea* drawing_area;
    gint x;
    gint y;
    gint base_width;
    gint base_height;
    gint scale;
    guint8* grayscale_frame_ptr;
    gboolean _inverted_colors;
};

struct _GameScreenClass {
    GObjectClass parent_class;
};

static void game_screen_init(GameScreen *self);
static void game_screen_class_init(GameScreenClass *klass);

G_DEFINE_TYPE(GameScreen, game_screen, G_TYPE_OBJECT)

static uint8_t bayer16_mat[16][16] = {
    {  0,  64,  16,  80,   4,  68,  20,  84,   1,  65,  17,  81,   5,  69,  21,  85 },
    {128, 192, 144, 208, 132, 196, 148, 212, 129, 193, 145, 209, 133, 197, 149, 213 },
    { 32,  96,  48, 112,  36, 100,  52, 116,  33,  97,  49, 113,  37, 101,  53, 117 },
    {160, 224, 176, 240, 164, 228, 180, 244, 161, 225, 177, 241, 165, 229, 181, 245 },
    {  8,  72,  24,  88,  12,  76,  28,  92,   9,  73,  25,  89,  13,  77,  29,  93 },
    {136, 200, 152, 216, 140, 204, 156, 220, 137, 201, 153, 217, 141, 205, 157, 221 },
    { 40, 104,  56, 120,  44, 108,  60, 124,  41, 105,  57, 121,  45, 109,  61, 125 },
    {168, 232, 184, 248, 172, 236, 188, 252, 169, 233, 185, 249, 173, 237, 189, 253 },
    {  2,  66,  18,  82,   6,  70,  22,  86,   3,  67,  19,  83,   7,  71,  23,  87 },
    {130, 194, 146, 210, 134, 198, 150, 214, 131, 195, 147, 211, 135, 199, 151, 215 },
    { 34,  98,  50, 114,  38, 102,  54, 118,  35,  99,  51, 115,  39, 103,  55, 119 },
    {162, 226, 178, 242, 166, 230, 182, 246, 163, 227, 179, 243, 167, 231, 183, 247 },
    { 10,  74,  26,  90,  14,  78,  30,  94,  11,  75,  27,  91,  15,  79,  31,  95 },
    {138, 202, 154, 218, 142, 206, 158, 222, 139, 203, 155, 219, 143, 207, 159, 223 },
    { 42, 106,  58, 122,  46, 110,  62, 126,  43, 107,  59, 123,  47, 111,  63, 127 },
    {170, 234, 186, 250, 174, 238, 190, 254, 171, 235, 187, 251, 175, 239, 191, 255 }
};

static gboolean on_drawing_area_expose(GtkWidget *widget, GdkEventExpose *event, gpointer user_data) {
    GameScreen *self = (GameScreen*)user_data;
    if (!self || !self->grayscale_frame_ptr || self->base_width <= 0 || self->base_height <= 0) {
        return FALSE;
    }
    gint sw = self->base_width * (self->scale > 0 ? self->scale : 1);
    gint sh = self->base_height * (self->scale > 0 ? self->scale : 1);

    GdkWindow *win = gtk_widget_get_window(widget);
    if (!win) return FALSE;

#if GTK_CHECK_VERSION(3, 0, 0)
    cairo_t *cr = gdk_cairo_create(win);
    cairo_surface_t *surf = cairo_image_surface_create_for_data(
        self->grayscale_frame_ptr, CAIRO_FORMAT_A8, sw, sh, sw);
    cairo_set_source_surface(cr, surf, 0, 0);
    cairo_paint(cr);
    cairo_surface_destroy(surf);
    cairo_destroy(cr);
#else
    gdk_draw_gray_image(
        win,
        widget->style->fg_gc[GTK_STATE_NORMAL],
        0, 0,
        sw, sh,
        GDK_RGB_DITHER_NONE,
        self->grayscale_frame_ptr,
        sw
    );
#endif
    (void)event;
    return TRUE;
}

static void game_screen_init_lut(GameScreen *self) {
    for (uint32_t i = 0; i < 65536; i++) {
        int r = ((i >> 11) & 0x1F) << 3;
        int g = ((i >> 5) & 0x3F) << 2;
        int b = (i & 0x1F) << 3;
        int L = (r * 77 + g * 150 + b * 29) >> 8;
        if (L < 0) L = 0;
        if (L > 255) L = 255;
        self->priv->grayscale_lut[i] = (uint8_t)L;
    }
}

static void game_screen_init(GameScreen *self) {
    self->priv = g_new0(GameScreenPrivate, 1);
    self->base_width = 160;
    self->base_height = 144;
    self->scale = 3;
    self->ready = FALSE;
    self->grayscale_frame_ptr = NULL;
    self->_inverted_colors = FALSE;
    game_screen_init_lut(self);
}

static void game_screen_finalize(GObject *obj) {
    GameScreen *self = (GameScreen*)obj;
    if (self->grayscale_frame_ptr) {
        g_free(self->grayscale_frame_ptr);
        self->grayscale_frame_ptr = NULL;
    }
    if (self->priv) {
        if (self->priv->shm_handle) {
            gray_shm_destroy(self->priv->shm_handle);
            self->priv->shm_handle = NULL;
        }
        g_free(self->priv);
        self->priv = NULL;
    }
    G_OBJECT_CLASS(game_screen_parent_class)->finalize(obj);
}

static void game_screen_class_init(GameScreenClass *klass) {
    G_OBJECT_CLASS(klass)->finalize = game_screen_finalize;
}

GameScreen* game_screen_new(GtkDrawingArea** da) {
    GameScreen *self = (GameScreen*)g_object_new(game_screen_get_type(), NULL);
    if (da && *da) {
        self->drawing_area = *da;
        g_signal_connect(G_OBJECT(self->drawing_area), "expose-event",
                         G_CALLBACK(on_drawing_area_expose), self);
    }
    return self;
}

void game_screen_start(GameScreen* self) {
    if (self) self->ready = TRUE;
}

void game_screen_stop(GameScreen* self) {
    if (self) self->ready = FALSE;
}

void game_screen_refresh(GameScreen* self) {
    if (self && self->drawing_area) {
        gtk_widget_queue_draw(GTK_WIDGET(self->drawing_area));
    }
}

void game_screen_resize(GameScreen* self, gint w, gint h, gint s) {
    if (!self) return;
    self->base_width = w > 0 ? w : 160;
    self->base_height = h > 0 ? h : 144;
    self->scale = s > 0 ? s : 1;

    gint sw = self->base_width * self->scale;
    gint sh = self->base_height * self->scale;

    if (self->grayscale_frame_ptr) {
        g_free(self->grayscale_frame_ptr);
    }
    self->grayscale_frame_ptr = g_malloc0(sw * sh);

    if (self->drawing_area) {
        gtk_widget_set_size_request(GTK_WIDGET(self->drawing_area), sw, sh);
    }
}

void game_screen_push_frame(GameScreen* self, void* data, guint width, guint height, gsize pitch) {
    if (!self || !data) return;

    gint s = self->scale > 0 ? self->scale : 1;
    gint sw = (gint)width * s;
    gint sh = (gint)height * s;

    if (!self->grayscale_frame_ptr || self->base_width != (gint)width || self->base_height != (gint)height) {
        game_screen_resize(self, width, height, s);
    }

    const uint16_t *src_pixels = (const uint16_t*)data;
    gint src_stride = pitch / sizeof(uint16_t);
    uint8_t *dst = self->grayscale_frame_ptr;

    /* Scaled Dithering: Nearest-neighbor + Bayer 16x16 */
    for (guint y = 0; y < height; y++) {
        const uint16_t *src_row = src_pixels + y * src_stride;
        for (gint sy = 0; sy < s; sy++) {
            gint dy = (gint)y * s + sy;
            uint8_t *dst_row = dst + dy * sw;
            for (guint x = 0; x < width; x++) {
                uint8_t gray = self->priv->grayscale_lut[src_row[x]];
                if (self->_inverted_colors) {
                    gray = 255 - gray;
                }
                uint8_t thresh = bayer16_mat[dy & 15][((gint)x * s) & 15];
                uint8_t val = (gray >= thresh) ? 255 : 0;
                for (gint sx = 0; sx < s; sx++) {
                    dst_row[x * s + sx] = val;
                }
            }
        }
    }

    if (self->drawing_area) {
        gtk_widget_queue_draw(GTK_WIDGET(self->drawing_area));
    }
}

gint game_screen_get_width(GameScreen* self) {
    return self ? self->base_width * (self->scale > 0 ? self->scale : 1) : 0;
}

gint game_screen_get_height(GameScreen* self) {
    return self ? self->base_height * (self->scale > 0 ? self->scale : 1) : 0;
}

void game_screen_set_inverted_colors(GameScreen* self, gboolean value) {
    if (self) self->_inverted_colors = value;
}

GType game_screen_commit_mode_get_type(void) {
    static volatile gsize type_id = 0;
    if (g_once_init_enter(&type_id)) {
        static const GEnumValue values[] = {
            { 0, "GAME_SCREEN_COMMIT_MODE_SHM", "shm" },
            { 1, "GAME_SCREEN_COMMIT_MODE_FBINK", "fbink" },
            { 0, NULL, NULL }
        };
        GType id = g_enum_register_static("GameScreenCommitMode", values);
        g_once_init_leave(&type_id, id);
    }
    return type_id;
}
/* ========================================================================= */
/* 5. Minimal Gee compatibility wrappers (backed by pure GLib)              */
/* ========================================================================= */

#include "gee.h"

typedef struct _GeeHashMapWrapper {
    GObject parent_instance;
    GHashTable *table;
    GBoxedCopyFunc k_dup;
    GDestroyNotify k_destroy;
    GBoxedCopyFunc v_dup;
    GDestroyNotify v_destroy;
} GeeHashMapWrapper;

typedef struct _GeeHashMapWrapperClass {
    GObjectClass parent_class;
} GeeHashMapWrapperClass;

G_DEFINE_TYPE(GeeHashMapWrapper, gee_hash_map_wrapper, G_TYPE_OBJECT)

static void gee_hash_map_wrapper_init(GeeHashMapWrapper *self) {
    self->table = g_hash_table_new(g_str_hash, g_str_equal);
}

static void gee_hash_map_wrapper_finalize(GObject *obj) {
    GeeHashMapWrapper *self = (GeeHashMapWrapper*)obj;
    if (self->table) {
        GHashTableIter iter;
        gpointer k, v;
        g_hash_table_iter_init(&iter, self->table);
        while (g_hash_table_iter_next(&iter, &k, &v)) {
            if (self->k_destroy && k) self->k_destroy(k);
            if (self->v_destroy && v) self->v_destroy(v);
        }
        g_hash_table_destroy(self->table);
        self->table = NULL;
    }
    G_OBJECT_CLASS(gee_hash_map_wrapper_parent_class)->finalize(obj);
}

static void gee_hash_map_wrapper_class_init(GeeHashMapWrapperClass *klass) {
    G_OBJECT_CLASS(klass)->finalize = gee_hash_map_wrapper_finalize;
}

gpointer gee_hash_map_new (GType k_type, GBoxedCopyFunc k_dup, GDestroyNotify k_destroy, GType v_type, GBoxedCopyFunc v_dup, GDestroyNotify v_destroy, ...) {
    (void)k_type; (void)v_type;
    GeeHashMapWrapper *wrapper = (GeeHashMapWrapper*)g_object_new(gee_hash_map_wrapper_get_type(), NULL);
    wrapper->k_dup = k_dup;
    wrapper->k_destroy = k_destroy;
    wrapper->v_dup = v_dup;
    wrapper->v_destroy = v_destroy;
    return (gpointer)wrapper;
}

gpointer gee_abstract_map_get (GeeAbstractMap* self, gconstpointer key) {
    GeeHashMapWrapper *w = (GeeHashMapWrapper*)self;
    if (!w || !w->table || !key) return NULL;
    return g_hash_table_lookup(w->table, key);
}

void gee_abstract_map_set (GeeAbstractMap* self, gconstpointer key, gconstpointer value) {
    GeeHashMapWrapper *w = (GeeHashMapWrapper*)self;
    if (!w || !w->table || !key) return;
    gpointer k = w->k_dup ? w->k_dup((gpointer)key) : (gpointer)key;
    gpointer v = w->v_dup ? w->v_dup((gpointer)value) : (gpointer)value;
    gpointer old_k = NULL, old_v = NULL;
    if (g_hash_table_lookup_extended(w->table, key, &old_k, &old_v)) {
        if (w->k_destroy && old_k) w->k_destroy(old_k);
        if (w->v_destroy && old_v) w->v_destroy(old_v);
    }
    g_hash_table_insert(w->table, k, v);
}

gboolean gee_abstract_map_has_key (GeeAbstractMap* self, gconstpointer key) {
    GeeHashMapWrapper *w = (GeeHashMapWrapper*)self;
    if (!w || !w->table || !key) return FALSE;
    return g_hash_table_lookup_extended(w->table, key, NULL, NULL);
}

/* GeeArrayList wrapper */
typedef struct _GeeArrayListWrapper {
    GObject parent_instance;
    GPtrArray *array;
    GBoxedCopyFunc dup_func;
    GDestroyNotify destroy_func;
} GeeArrayListWrapper;

typedef struct _GeeArrayListWrapperClass {
    GObjectClass parent_class;
} GeeArrayListWrapperClass;

G_DEFINE_TYPE(GeeArrayListWrapper, gee_array_list_wrapper, G_TYPE_OBJECT)

static void gee_array_list_wrapper_init(GeeArrayListWrapper *self) {
    self->array = g_ptr_array_new();
}

static void gee_array_list_wrapper_finalize(GObject *obj) {
    GeeArrayListWrapper *self = (GeeArrayListWrapper*)obj;
    if (self->array) {
        if (self->destroy_func) {
            for (guint i = 0; i < self->array->len; i++) {
                gpointer p = g_ptr_array_index(self->array, i);
                if (p) self->destroy_func(p);
            }
        }
        g_ptr_array_free(self->array, TRUE);
        self->array = NULL;
    }
    G_OBJECT_CLASS(gee_array_list_wrapper_parent_class)->finalize(obj);
}

static void gee_array_list_wrapper_class_init(GeeArrayListWrapperClass *klass) {
    G_OBJECT_CLASS(klass)->finalize = gee_array_list_wrapper_finalize;
}

gpointer gee_array_list_new (GType g_type, GBoxedCopyFunc g_dup_func, GDestroyNotify g_destroy_func, ...) {
    (void)g_type;
    GeeArrayListWrapper *wrapper = (GeeArrayListWrapper*)g_object_new(gee_array_list_wrapper_get_type(), NULL);
    wrapper->dup_func = g_dup_func;
    wrapper->destroy_func = g_destroy_func;
    return (gpointer)wrapper;
}

gboolean gee_abstract_collection_add (GeeAbstractCollection* self, gconstpointer item) {
    GeeArrayListWrapper *w = (GeeArrayListWrapper*)self;
    if (!w || !w->array) return FALSE;
    gpointer p = w->dup_func ? w->dup_func((gpointer)item) : (gpointer)item;
    g_ptr_array_add(w->array, p);
    return TRUE;
}

gint gee_abstract_collection_get_size (GeeCollection* self) {
    GeeArrayListWrapper *w = (GeeArrayListWrapper*)self;
    return (w && w->array) ? (gint)w->array->len : 0;
}

gpointer gee_abstract_list_get (GeeAbstractList* self, gint index) {
    GeeArrayListWrapper *w = (GeeArrayListWrapper*)self;
    if (!w || !w->array || index < 0 || (guint)index >= w->array->len) return NULL;
    return g_ptr_array_index(w->array, (guint)index);
}

void gee_list_sort (GeeList* self, GCompareDataFunc compare_func, gpointer user_data, GDestroyNotify destroy_func) {
    GeeArrayListWrapper *w = (GeeArrayListWrapper*)self;
    if (w && w->array && compare_func) {
        g_ptr_array_sort_with_data(w->array, compare_func, user_data);
    }
    if (destroy_func && user_data) {
        destroy_func(user_data);
    }
}

/* GeeIterator wrapper */
typedef struct _GeeIteratorWrapper {
    GObject parent_instance;
    GeeArrayListWrapper *list;
    gint current_idx;
} GeeIteratorWrapper;

typedef struct _GeeIteratorWrapperClass {
    GObjectClass parent_class;
} GeeIteratorWrapperClass;

G_DEFINE_TYPE(GeeIteratorWrapper, gee_iterator_wrapper, G_TYPE_OBJECT)

static void gee_iterator_wrapper_init(GeeIteratorWrapper *self) {
    self->list = NULL;
    self->current_idx = -1;
}

static void gee_iterator_wrapper_finalize(GObject *obj) {
    GeeIteratorWrapper *self = (GeeIteratorWrapper*)obj;
    if (self->list) {
        g_object_unref(self->list);
        self->list = NULL;
    }
    G_OBJECT_CLASS(gee_iterator_wrapper_parent_class)->finalize(obj);
}

static void gee_iterator_wrapper_class_init(GeeIteratorWrapperClass *klass) {
    G_OBJECT_CLASS(klass)->finalize = gee_iterator_wrapper_finalize;
}

GeeIterator* gee_iterable_iterator (GeeIterable* self) {
    GeeIteratorWrapper *it = (GeeIteratorWrapper*)g_object_new(gee_iterator_wrapper_get_type(), NULL);
    if (self) {
        it->list = (GeeArrayListWrapper*)g_object_ref(self);
    }
    it->current_idx = -1;
    return (GeeIterator*)it;
}

gboolean gee_iterator_next (GeeIterator* self) {
    GeeIteratorWrapper *it = (GeeIteratorWrapper*)self;
    if (!it || !it->list || !it->list->array) return FALSE;
    if (it->current_idx + 1 < (gint)it->list->array->len) {
        it->current_idx++;
        return TRUE;
    }
    return FALSE;
}

gpointer gee_iterator_get (GeeIterator* self) {
    GeeIteratorWrapper *it = (GeeIteratorWrapper*)self;
    if (!it || !it->list || !it->list->array) return NULL;
    if (it->current_idx >= 0 && (guint)it->current_idx < it->list->array->len) {
        return g_ptr_array_index(it->list->array, (guint)it->current_idx);
    }
    return NULL;
}

GeeCollection* gee_abstract_map_get_keys (GeeMap* self) {
    GeeHashMapWrapper *w = (GeeHashMapWrapper*)self;
    GeeArrayListWrapper *keys_list = (GeeArrayListWrapper*)gee_array_list_new(G_TYPE_STRING, (GBoxedCopyFunc)g_strdup, g_free, NULL);
    if (w && w->table) {
        GHashTableIter iter;
        gpointer k, v;
        g_hash_table_iter_init(&iter, w->table);
        while (g_hash_table_iter_next(&iter, &k, &v)) {
            gee_abstract_collection_add((GeeAbstractCollection*)keys_list, k);
        }
    }
    return (GeeCollection*)keys_list;
}

/* ========================================================================= */
/* 5. settings_bind_bind Implementation                                      */
/* ========================================================================= */

typedef void (*SettingsBindSaveCallback) (void* user_data);

typedef struct {
    GKeyFile *kf;
    gchar *section;
    gchar *key;
    GType t_type;
    SettingsBindSaveCallback save_cb;
    gpointer save_cb_target;
} SettingsBindData;

static void on_settings_property_changed(GObject *obj, GParamSpec *pspec, gpointer user_data) {
    SettingsBindData *data = (SettingsBindData*)user_data;
    if (!data || !data->kf || !data->section || !data->key) return;

    if (data->t_type == G_TYPE_STRING) {
        gchar *val = NULL;
        g_object_get(obj, pspec->name, &val, NULL);
        if (val) {
            g_key_file_set_string(data->kf, data->section, data->key, val);
            g_free(val);
        }
    } else if (data->t_type == G_TYPE_INT) {
        gint val = 0;
        g_object_get(obj, pspec->name, &val, NULL);
        g_key_file_set_integer(data->kf, data->section, data->key, val);
    } else if (data->t_type == G_TYPE_BOOLEAN) {
        gboolean val = FALSE;
        g_object_get(obj, pspec->name, &val, NULL);
        g_key_file_set_boolean(data->kf, data->section, data->key, val);
    }

    if (data->save_cb) {
        data->save_cb(data->save_cb_target);
    }
}

void settings_bind_bind (GType t_type, GBoxedCopyFunc t_dup_func, GDestroyNotify t_destroy_func,
                         GKeyFile* kf, const gchar* section, const gchar* key,
                         GObject* obj, const gchar* prop,
                         SettingsBindSaveCallback save_cb, void* save_cb_target) {
    (void)t_dup_func; (void)t_destroy_func;
    if (!kf || !section || !key || !obj || !prop) return;

    /* 1. Load initial value from GKeyFile into GObject property */
    if (t_type == G_TYPE_STRING) {
        gchar *val = g_key_file_get_string(kf, section, key, NULL);
        if (val) {
            g_object_set(obj, prop, val, NULL);
            g_free(val);
        }
    } else if (t_type == G_TYPE_INT) {
        GError *err = NULL;
        gint val = g_key_file_get_integer(kf, section, key, &err);
        if (!err) {
            g_object_set(obj, prop, val, NULL);
        } else {
            g_clear_error(&err);
        }
    } else if (t_type == G_TYPE_BOOLEAN) {
        GError *err = NULL;
        gboolean val = g_key_file_get_boolean(kf, section, key, &err);
        if (!err) {
            g_object_set(obj, prop, val, NULL);
        } else {
            g_clear_error(&err);
        }
    }

    /* 2. Bind change notification so saving happens when settings change */
    SettingsBindData *data = g_new0(SettingsBindData, 1);
    data->kf = kf;
    data->section = g_strdup(section);
    data->key = g_strdup(key);
    data->t_type = t_type;
    data->save_cb = save_cb;
    data->save_cb_target = save_cb_target;

    gchar *signal_name = g_strdup_printf("notify::%s", prop);
    g_signal_connect(obj, signal_name, G_CALLBACK(on_settings_property_changed), data);
    g_free(signal_name);
}

