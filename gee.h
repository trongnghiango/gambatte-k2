#ifndef _GEE_H_COMPAT_
#define _GEE_H_COMPAT_

#include <glib.h>
#include <glib-object.h>

typedef GObject GeeAbstractCollection;
typedef GObject GeeCollection;
typedef GObject GeeAbstractList;
typedef GObject GeeList;
typedef GObject GeeArrayList;
typedef GObject GeeAbstractMap;
typedef GObject GeeMap;
typedef GObject GeeHashMap;
typedef GObject GeeIterable;
typedef GObject GeeIterator;
typedef GObject GeeSet;

#ifdef __cplusplus
extern "C" {
#endif

gpointer gee_hash_map_new (GType k_type, GBoxedCopyFunc k_dup, GDestroyNotify k_destroy, GType v_type, GBoxedCopyFunc v_dup, GDestroyNotify v_destroy, ...);
gpointer gee_abstract_map_get (GeeAbstractMap* self, gconstpointer key);
void gee_abstract_map_set (GeeAbstractMap* self, gconstpointer key, gconstpointer value);
gboolean gee_abstract_map_has_key (GeeAbstractMap* self, gconstpointer key);
GeeCollection* gee_abstract_map_get_keys (GeeMap* self);

gpointer gee_array_list_new (GType g_type, GBoxedCopyFunc g_dup_func, GDestroyNotify g_destroy_func, ...);
gboolean gee_abstract_collection_add (GeeAbstractCollection* self, gconstpointer item);
gint gee_abstract_collection_get_size (GeeCollection* self);
gpointer gee_abstract_list_get (GeeAbstractList* self, gint index);
void gee_list_sort (GeeList* self, GCompareDataFunc compare_func, gpointer user_data, GDestroyNotify destroy_func);

GeeIterator* gee_iterable_iterator (GeeIterable* self);
gboolean gee_iterator_next (GeeIterator* self);
gpointer gee_iterator_get (GeeIterator* self);

#ifdef __cplusplus
}
#endif

#endif /* _GEE_H_COMPAT_ */
