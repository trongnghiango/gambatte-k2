#ifndef FILE_PICKER_H
#define FILE_PICKER_H

#include <gtk/gtk.h>

GtkWidget *create_rom_picker_view(void (*on_selected)(const char *path), void (*on_back)(void));
void refresh_rom_picker_view(void);

#endif
