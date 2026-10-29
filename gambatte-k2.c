#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include <ctype.h>
#include <unistd.h>
#include <fcntl.h>
#include <pthread.h>
#include <sched.h>
#include <errno.h>
#include <poll.h>
#include <math.h>
#include <sys/stat.h>
#include <linux/input.h>
#include <dlfcn.h>
#ifdef __ARM_NEON
#include <arm_neon.h>
#endif
#include <glib.h>
#include <gtk/gtk.h>
#include <gdk/gdkx.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include "libretro.h"
#include "fbink.h"
#include "file_picker.h"
#include "main_ui.h"

void sanitize_filename(const char *input, char *output, size_t maxlen);

static void *core_handle = NULL;
typedef void (*retro_init_t)(void);
typedef void (*retro_deinit_t)(void);
typedef bool (*retro_load_game_t)(const struct retro_game_info *);
typedef void (*retro_unload_game_t)(void);
typedef void (*retro_run_t)(void);
typedef void (*retro_reset_t)(void);
typedef void (*retro_set_environment_t)(retro_environment_t);
typedef void (*retro_set_video_refresh_t)(retro_video_refresh_t);
typedef void (*retro_set_audio_sample_batch_t)(retro_audio_sample_batch_t);
typedef void (*retro_set_input_poll_t)(retro_input_poll_t);
typedef void (*retro_set_input_state_t)(retro_input_state_t);
typedef void (*retro_get_system_av_info_t)(struct retro_system_av_info *);
typedef size_t (*retro_serialize_size_t)(void);
typedef bool (*retro_serialize_t)(void *, size_t);
typedef bool (*retro_unserialize_t)(const void *, size_t);

#define retro_init core_retro_init
#define retro_deinit core_retro_deinit
#define retro_load_game core_retro_load_game
#define retro_unload_game core_retro_unload_game
#define retro_run core_retro_run
#define retro_reset core_retro_reset
#define retro_set_environment core_retro_set_environment
#define retro_set_video_refresh core_retro_set_video_refresh
#define retro_set_audio_sample_batch core_retro_set_audio_sample_batch
#define retro_set_input_poll core_retro_set_input_poll
#define retro_set_input_state core_retro_set_input_state
#define retro_get_system_av_info core_retro_get_system_av_info
#define retro_serialize_size core_retro_serialize_size
#define retro_serialize core_retro_serialize
#define retro_unserialize core_retro_unserialize

static retro_init_t core_retro_init = NULL;
static retro_deinit_t core_retro_deinit = NULL;
static retro_load_game_t core_retro_load_game = NULL;
static retro_unload_game_t core_retro_unload_game = NULL;
static retro_run_t core_retro_run = NULL;
static retro_reset_t core_retro_reset = NULL;
static retro_set_environment_t core_retro_set_environment = NULL;
static retro_set_video_refresh_t core_retro_set_video_refresh = NULL;
static retro_set_audio_sample_batch_t core_retro_set_audio_sample_batch = NULL;
static retro_set_input_poll_t core_retro_set_input_poll = NULL;
static retro_set_input_state_t core_retro_set_input_state = NULL;
static retro_get_system_av_info_t core_retro_get_system_av_info = NULL;
static retro_serialize_size_t core_retro_serialize_size = NULL;
static retro_serialize_t core_retro_serialize = NULL;
static retro_unserialize_t core_retro_unserialize = NULL;

/* FBInk stubs in case libfbink is not installed */
int fbink_init(int fbfd, const FBInkConfig* restrict fbink_cfg) { (void)fbfd; (void)fbink_cfg; return -1; }
int fbink_refresh(int fbfd, uint32_t region_top, uint32_t region_left, uint32_t region_width, uint32_t region_height, const FBInkConfig* restrict fbink_cfg) { (void)fbfd; (void)region_top; (void)region_left; (void)region_width; (void)region_height; (void)fbink_cfg; return 0; }
int fbink_print_raw_data(int fbfd, const unsigned char* restrict data, const int w, const int h, const size_t len, short int x_off, short int y_off, const FBInkConfig* restrict fbink_cfg) { (void)fbfd; (void)data; (void)w; (void)h; (void)len; (void)x_off; (void)y_off; (void)fbink_cfg; return 0; }
FBInkInputDevice* fbink_input_scan(uint32_t match_types, uint32_t exclude_types, uint32_t settings, size_t* result_length1) { (void)match_types; (void)exclude_types; (void)settings; if (result_length1) *result_length1 = 0; return NULL; }

static bool load_core(void) {
    if (core_handle && retro_init && retro_load_game && retro_run) return true;

    const char *candidates[] = {
        "cores/gambatte_libretro.so",
        "cores/gambatte_libretro-armhf.so",
        "cores/gambatte_libretro-armel.so",
        "gambatte_libretro.so",
        "gambatte_libretro-armhf.so",
        "gambatte_libretro-armel.so",
        "/mnt/us/extensions/gambatte-k2/cores/gambatte_libretro.so",
        "/mnt/us/extensions/gambatte-k2/cores/gambatte_libretro-armhf.so",
        "/mnt/us/extensions/gambatte-k2/cores/gambatte_libretro-armel.so",
        "/mnt/us/extensions/gambatte-k2/gambatte_libretro.so",
        "/mnt/us/extensions/gambatte-k2/gambatte_libretro-armhf.so",
        "/mnt/us/extensions/gambatte-k2/gambatte_libretro-armel.so",
        NULL
    };

    for (int i = 0; candidates[i] != NULL; i++) {
        if (access(candidates[i], R_OK) == 0) {
            core_handle = dlopen(candidates[i], RTLD_LAZY | RTLD_GLOBAL);
            if (core_handle) {
                fprintf(stdout, "Successfully loaded core from: %s\n", candidates[i]);
                break;
            }
        }
    }

    if (!core_handle) {
        fprintf(stderr, "Failed to load gambatte libretro core\n");
        return false;
    }

    retro_init = (retro_init_t)dlsym(core_handle, "retro_init");
    retro_deinit = (retro_deinit_t)dlsym(core_handle, "retro_deinit");
    retro_load_game = (retro_load_game_t)dlsym(core_handle, "retro_load_game");
    retro_unload_game = (retro_unload_game_t)dlsym(core_handle, "retro_unload_game");
    retro_run = (retro_run_t)dlsym(core_handle, "retro_run");
    retro_reset = (retro_reset_t)dlsym(core_handle, "retro_reset");
    retro_set_environment = (retro_set_environment_t)dlsym(core_handle, "retro_set_environment");
    retro_set_video_refresh = (retro_set_video_refresh_t)dlsym(core_handle, "retro_set_video_refresh");
    retro_set_audio_sample_batch = (retro_set_audio_sample_batch_t)dlsym(core_handle, "retro_set_audio_sample_batch");
    retro_set_input_poll = (retro_set_input_poll_t)dlsym(core_handle, "retro_set_input_poll");
    retro_set_input_state = (retro_set_input_state_t)dlsym(core_handle, "retro_set_input_state");
    retro_get_system_av_info = (retro_get_system_av_info_t)dlsym(core_handle, "retro_get_system_av_info");
    retro_serialize_size = (retro_serialize_size_t)dlsym(core_handle, "retro_serialize_size");
    retro_serialize = (retro_serialize_t)dlsym(core_handle, "retro_serialize");
    retro_unserialize = (retro_unserialize_t)dlsym(core_handle, "retro_unserialize");

    return (retro_init && retro_load_game && retro_run);
}

static char internal_game_name[17];

// GTK widgets
static GtkWidget *window, *drawing_area, *drawing_area_container, *joystick_area;
static GtkWidget *hamburger_btn = NULL, *title_label = NULL;
static GtkWidget *popup_menu = NULL;
static volatile gint drawing_area_abs_x = 0, drawing_area_abs_y = 0;
static volatile gint joystick_area_abs_x = 0, joystick_area_abs_y = 0;
static int PIXBUF_WIDTH = 0, PIXBUF_HEIGHT = 0;

// Framebuffer/FBInk
int fbfd = FBFD_AUTO, fbfd_refresh = FBFD_AUTO;
FBInkConfig fbink_cfg = {0}, fbink_cfg_refresh = {0};

// Mutexes
GMutex *pixbuf_mutex = NULL, *slots_mutex = NULL, *game_loop_mutex = NULL;
static GAsyncQueue *frame_queue;

// Audio
typedef struct AudioBackend {
    void *mixer_handle;
} AudioBackend;
static AudioBackend *ab;

int16_t *MixerOpenPlay(int _, int __, int ___, const char *____) __attribute__((weak));
uint8_t *MixerGetBufPlay(void *_, int *__, int *___) __attribute__((weak));
void MixerWaitTillAudioProcessed(void *_) __attribute__((weak));
void MixerDrain(void *_) __attribute__((weak));
void MixerClose(void *_) __attribute__((weak));
void MixerReleaseBufPlay(void *_, int __, uint8_t *___) __attribute__((weak));
int MixerGetNumBytes(void *_) __attribute__((weak));

// Buffers
static GdkPixbuf *pixbuf = NULL;
static guchar *grayscale_frame_ptr = NULL;

static void ensure_pixbuf(int width, int height) {
    if (!pixbuf || PIXBUF_WIDTH != width || PIXBUF_HEIGHT != height) {
        if (pixbuf) g_object_unref(pixbuf);
        PIXBUF_WIDTH = width;
        PIXBUF_HEIGHT = height;
        pixbuf = gdk_pixbuf_new(GDK_COLORSPACE_RGB, FALSE, 8, width, height);
    }
}

static gboolean expose_event(GtkWidget *widget, GdkEventExpose *event, gpointer data) {
    if (!pixbuf) return FALSE;
    g_mutex_lock(pixbuf_mutex);
    GdkWindow *win = gtk_widget_get_window(widget);
    cairo_t *cr = gdk_cairo_create(win);
    gdk_cairo_set_source_pixbuf(cr, pixbuf, 0, 0);
    cairo_paint(cr);
    cairo_destroy(cr);
    g_mutex_unlock(pixbuf_mutex);
    return FALSE;
}

static uint8_t grayscale_lut[65536] = {0};  // for RGB565 -> RGB888 -> grayscale
static struct retro_game_info game = {0};
// 
typedef struct { uint16_t *data; unsigned width, height; size_t pitch; } FrameJob;
typedef struct { int x, y, w, h, frame_width, frame_height; } DirtyRect;

typedef enum { DRAW_MODE_GTK, DRAW_MODE_FBINK } DrawMode;
static DrawMode draw_mode = DRAW_MODE_GTK;

typedef enum { STYLE_MODE_DARK, STYLE_MODE_LIGHT } StyleMode;
static StyleMode style_mode = STYLE_MODE_DARK;
static volatile gint refresh_screen = 0;
////// JOYSTICK
typedef enum { GB_BUTTON_ARC, GB_BUTTON_RECT } GbButtonGeometry;

typedef struct { float rel_x, rel_y, rel_r, rel_w, rel_h; gint x, y, r, w, h, active; GbButtonGeometry geometry; const char *label; } GbButtonRegion;

enum { GB_UP, GB_DOWN, GB_LEFT, GB_RIGHT, GB_A, GB_B, GB_SELECT, GB_START, GB_BUTTON_COUNT };

static GbButtonRegion gb_button_regions[GB_BUTTON_COUNT] = {
//    rel_x  rel_y  rel_r  rel_w  rel_h  x   y   r   w   h   active geometry        label 
    { 0.24,  0.15,  0.10,  0.14,  0.14,  0,  0,  0,  0,  0,  0,     GB_BUTTON_RECT, ""       }, // UP
    { 0.24,  0.65,  0.10,  0.14,  0.14,  0,  0,  0,  0,  0,  0,     GB_BUTTON_RECT, ""       }, // DOWN
    { 0.10,  0.40,  0.10,  0.14,  0.14,  0,  0,  0,  0,  0,  0,     GB_BUTTON_RECT, ""       }, // LEFT
    { 0.38,  0.40,  0.10,  0.14,  0.14,  0,  0,  0,  0,  0,  0,     GB_BUTTON_RECT, ""       }, // RIGHT
    { 0.89,  0.24,  0.08,  0.00,  0.00,  0,  0,  0,  0,  0,  0,     GB_BUTTON_ARC,  "A"      },
    { 0.67,  0.43,  0.08,  0.00,  0.00,  0,  0,  0,  0,  0,  0,     GB_BUTTON_ARC,  "B"      },
    { 0.39,  0.87,  0.07,  0.13,  0.07,  0,  0,  0,  0,  0,  0,     GB_BUTTON_RECT, "SELECT" },
    { 0.61,  0.87,  0.07,  0.13,  0.07,  0,  0,  0,  0,  0,  0,     GB_BUTTON_RECT, "START"  }
};

static int gb_button_state[GB_BUTTON_COUNT] = {0};

#define MAX_TOUCHES 2
typedef struct { bool active; int x, y; } TouchSlot;
static TouchSlot slots_snap[MAX_TOUCHES];
///////

// Global state
#define MAX_STATE_FILES 5
#define MAX_FILENAME_LEN 256

typedef struct {
    char filename[MAX_FILENAME_LEN];
    char modified_time[2048];
    int exists;
} StateFileEntry;

enum { STATE_MODE_LOAD, STATE_MODE_SAVE };
static gboolean running = TRUE;
static gboolean ready = FALSE;

static float brightness = 0; // TODO settings on gui
static float contrast = 1.5;

static double audio_sample_rate = 32000;
static double video_fps = 60;
static int   frame_interval_us = 16742;

static int SCALE_FACTOR_A = 3;

static int SCREEN_BORDER = 5; // px

static int kill_the_ghost = 0;

static uint8_t bayer16[16][16] = {
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

#ifdef __ARM_NEON
// 2) Pre‑load 16 NEON threshold vectors—run once at init
static uint8x16_t t_vecs[16];
#endif

void init_bayer16_thresholds(void) {
#ifdef __ARM_NEON
    for (int r = 0; r < 16; r++) {
        t_vecs[r] = vld1q_u8(bayer16[r]);
    }
#endif
}

void init_grayscale_lut(float contrast, float brightness) {
    for (uint32_t i = 0; i < 65536; i++) {
        // Convert RGB565 to RGB888
        int r = ((i >> 11) & 0x1F) << 3;
        int g = ((i >> 5) & 0x3F) << 2;
        int b = (i & 0x1F) << 3;
        // BT.601 luminance approx
        int L = (r*77 + g*150 + b*29) >> 8;
        // brightness & contrast
        int La = (L - 128) * contrast + 128 + brightness * 255;
        grayscale_lut[i] = (uint8_t)(La < 0 ? 0 : La > 255 ? 255 : La);
    }
}

// https://github.com/koreader/koreader/blob/c3352ee171aa8ea695f64518a38c03b759e7cb24/frontend/device/kindle/device.lua#L477
const char *detect_touch_device() {
    static char detected_touch_path[256] = "/dev/input/event1"; // fallback default on Kindle
    if (fbink_input_scan) {
        size_t dev_count = 0;
        uint32_t match_mask = 0x1 | 0x2; // touchscreen | tablet
        FBInkInputDevice *devices = fbink_input_scan(match_mask, 0, 0, &dev_count);
        if (devices && dev_count > 0) {
            for (size_t i = 0; i < dev_count; ++i) {
                if (devices[i].matched) {
                    strncpy(detected_touch_path, devices[i].path, sizeof(detected_touch_path) - 1);
                    detected_touch_path[sizeof(detected_touch_path) - 1] = '\0';
                    break;
                }
            }
            free(devices);
        }
    }
    fprintf(stdout, "Detected touch device: %s\n", detected_touch_path);
    return detected_touch_path;
}

// touchscreen thread
static gpointer touch_thread_fn(gpointer data) {
    /* Use normal Linux CFS scheduling on single-core CPU to prevent starvation */
    const char *touch_path = detect_touch_device();
    int fd = open(touch_path, O_RDONLY | O_NONBLOCK);
    if (fd < 0) { perror("open touch device"); return NULL; }

    struct input_event ev;
    struct pollfd fds = { .fd = fd, .events = POLLIN };
    TouchSlot slots[MAX_TOUCHES];
    memset(slots, 0, sizeof(slots));
    int current_slot = 0;

    while (running) {
        int ret = poll(&fds, 1, 30);
        if (ret <= 0) continue;
        while (read(fd, &ev, sizeof(ev)) == sizeof(ev)) {

            if (ev.type == EV_ABS) {
                switch (ev.code) {
                    case ABS_MT_SLOT:
                        if (ev.value >= 0 && ev.value < MAX_TOUCHES)
                            current_slot = ev.value;
                        break;
                    case ABS_MT_TRACKING_ID: {
                        if (current_slot >= 0 && current_slot < MAX_TOUCHES)
                            slots[current_slot].active = (ev.value != -1);
                        break;
                    }
                    case ABS_MT_POSITION_X: {
                        if (current_slot >= 0 && current_slot < MAX_TOUCHES)
                            slots[current_slot].x = ev.value;
                        break;
                    }
                    case ABS_MT_POSITION_Y: {
                        if (current_slot >= 0 && current_slot < MAX_TOUCHES)
                            slots[current_slot].y = ev.value;
                        break;
                    }
                }
            }
            else if (ev.type == EV_SYN && (ev.code == SYN_REPORT || ev.code == SYN_MT_REPORT)) {
                g_mutex_lock(slots_mutex);
                memcpy(slots_snap, slots, sizeof(slots));
                g_mutex_unlock(slots_mutex);
            }
        }
    }
    close(fd);
    return NULL;
}

// input callbacks | gambatte thread
static void input_poll(void){
    // fprintf(stdout, "input_poll\n");
    // Clear all buttons
    for (int i = 0; i < GB_BUTTON_COUNT; ++i)
        gb_button_state[i] = 0;

    // Snapshot touch slots
    TouchSlot slots[MAX_TOUCHES];

    g_mutex_lock(slots_mutex);
    memcpy(slots, slots_snap, sizeof(slots));
    g_mutex_unlock(slots_mutex);

    // For each active touch, set the corresponding button
    for (int s = 0; s < MAX_TOUCHES; ++s) {
        if (!slots[s].active)
            continue;

        int rel_x = slots[s].x - g_atomic_int_get(&joystick_area_abs_x);
        int rel_y = slots[s].y - g_atomic_int_get(&joystick_area_abs_y);

        for (int i = 0; i < GB_BUTTON_COUNT; ++i) {
            int x = g_atomic_int_get(&gb_button_regions[i].x); // cx
            int y = g_atomic_int_get(&gb_button_regions[i].y); // cy
            int r = g_atomic_int_get(&gb_button_regions[i].r);  

            int dx = rel_x - x;
            int dy = rel_y - y;
            // circle equation
            if (dx * dx + dy * dy <= r * r) {
                gb_button_state[i] = 1;
            }
        }
    }

    static int prev_button_state[GB_BUTTON_COUNT] = {0};
    gboolean state_changed = FALSE;
    for (int i = 0; i < GB_BUTTON_COUNT; ++i) {
        if (gb_button_state[i] != prev_button_state[i]) {
            prev_button_state[i] = gb_button_state[i];
            state_changed = TRUE;
        }
    }
    if (state_changed && joystick_area) {
        gtk_widget_queue_draw(joystick_area);
    }
}

// input state | gambatte thread
static int16_t input_state(unsigned port, unsigned device, unsigned index, unsigned id) {
    if (device == RETRO_DEVICE_JOYPAD && port == 0) {
        switch (id) {
            case RETRO_DEVICE_ID_JOYPAD_UP:     return gb_button_state[GB_UP];
            case RETRO_DEVICE_ID_JOYPAD_DOWN:   return gb_button_state[GB_DOWN];
            case RETRO_DEVICE_ID_JOYPAD_LEFT:   return gb_button_state[GB_LEFT];
            case RETRO_DEVICE_ID_JOYPAD_RIGHT:  return gb_button_state[GB_RIGHT];
            case RETRO_DEVICE_ID_JOYPAD_A:      return gb_button_state[GB_A];
            case RETRO_DEVICE_ID_JOYPAD_B:      return gb_button_state[GB_B];
            case RETRO_DEVICE_ID_JOYPAD_SELECT: return gb_button_state[GB_SELECT];
            case RETRO_DEVICE_ID_JOYPAD_START:  return gb_button_state[GB_START];
        }
    }
    return 0;    
}

// gtk thread
static void on_area_configure_event(GtkWidget *widget, GdkEvent *event, gpointer user_data) {
    GdkWindow *gdk_win = gtk_widget_get_window(widget);
    int abs_x = 0, abs_y = 0;
    if (gdk_win)
        gdk_window_get_origin(gdk_win, &abs_x, &abs_y);

    if (widget == drawing_area) {
        g_atomic_int_set(&drawing_area_abs_x, abs_x);
        g_atomic_int_set(&drawing_area_abs_y, abs_y);

        fprintf(stdout, "drawing_area area abs pos: %4d %4d\n", abs_x, abs_y);

    } else if (widget == joystick_area) {
        g_atomic_int_set(&joystick_area_abs_x, abs_x);
        g_atomic_int_set(&joystick_area_abs_y, abs_y);

        GtkAllocation allocation;
        gtk_widget_get_allocation(widget, &allocation);
        int w = allocation.width;
        int h = allocation.height;

        double dpad_cx = w * 0.22;
        double dpad_cy = h * 0.46;
        double dpad_petal_h = h * 0.20;
        double dpad_gap = w * 0.015;
        double dpad_dist = dpad_gap + dpad_petal_h * 0.55;
        int dpad_hitbox_r = (int)(dpad_petal_h * 0.60);

        // UP
        g_atomic_int_set(&gb_button_regions[GB_UP].x, (int)dpad_cx);
        g_atomic_int_set(&gb_button_regions[GB_UP].y, (int)(dpad_cy - dpad_dist));
        g_atomic_int_set(&gb_button_regions[GB_UP].r, dpad_hitbox_r);

        // DOWN
        g_atomic_int_set(&gb_button_regions[GB_DOWN].x, (int)dpad_cx);
        g_atomic_int_set(&gb_button_regions[GB_DOWN].y, (int)(dpad_cy + dpad_dist));
        g_atomic_int_set(&gb_button_regions[GB_DOWN].r, dpad_hitbox_r);

        // LEFT
        g_atomic_int_set(&gb_button_regions[GB_LEFT].x, (int)(dpad_cx - dpad_dist));
        g_atomic_int_set(&gb_button_regions[GB_LEFT].y, (int)dpad_cy);
        g_atomic_int_set(&gb_button_regions[GB_LEFT].r, dpad_hitbox_r);

        // RIGHT
        g_atomic_int_set(&gb_button_regions[GB_RIGHT].x, (int)(dpad_cx + dpad_dist));
        g_atomic_int_set(&gb_button_regions[GB_RIGHT].y, (int)dpad_cy);
        g_atomic_int_set(&gb_button_regions[GB_RIGHT].r, dpad_hitbox_r);

        // B
        g_atomic_int_set(&gb_button_regions[GB_B].x, (int)(w * 0.72));
        g_atomic_int_set(&gb_button_regions[GB_B].y, (int)(h * 0.54));
        g_atomic_int_set(&gb_button_regions[GB_B].r, (int)(w * 0.055));

        // A
        g_atomic_int_set(&gb_button_regions[GB_A].x, (int)(w * 0.86));
        g_atomic_int_set(&gb_button_regions[GB_A].y, (int)(h * 0.36));
        g_atomic_int_set(&gb_button_regions[GB_A].r, (int)(w * 0.055));

        // SELECT
        g_atomic_int_set(&gb_button_regions[GB_SELECT].x, (int)(w * 0.44));
        g_atomic_int_set(&gb_button_regions[GB_SELECT].y, (int)(h * 0.86));
        g_atomic_int_set(&gb_button_regions[GB_SELECT].r, 52);

        // START
        g_atomic_int_set(&gb_button_regions[GB_START].x, (int)(w * 0.56));
        g_atomic_int_set(&gb_button_regions[GB_START].y, (int)(h * 0.86));
        g_atomic_int_set(&gb_button_regions[GB_START].r, 52);

        fprintf(stdout, "joystick_area abs pos: %4d %4d (w:%4d h:%4d)\n", abs_x, abs_y, w, h);
    }
}

static void draw_dpad_petal(cairo_t *cr, double cx, double cy, double petal_w, double petal_h, double center_gap, double r, double angle_rad, gboolean pressed, StyleMode mode) {
    cairo_save(cr);
    cairo_translate(cr, cx, cy);
    cairo_rotate(cr, angle_rad);

    double y_tip = -center_gap;
    double y_sh = -(center_gap + petal_h * 0.40);
    double y_top = -(center_gap + petal_h);
    double x_w = petal_w / 2.0;

    cairo_new_path(cr);
    cairo_arc(cr, x_w - r, y_top + r, r, -M_PI / 2.0, 0);
    cairo_line_to(cr, x_w, y_sh);
    cairo_line_to(cr, 0, y_tip);
    cairo_line_to(cr, -x_w, y_sh);
    cairo_line_to(cr, -x_w, y_top + r);
    cairo_arc(cr, -x_w + r, y_top + r, r, M_PI, 3.0 * M_PI / 2.0);
    cairo_close_path(cr);

    if (pressed) {
        if (mode == STYLE_MODE_LIGHT) {
            cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);
            cairo_fill_preserve(cr);
            cairo_set_line_width(cr, 3.0);
            cairo_stroke(cr);
            cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
        } else {
            cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
            cairo_fill_preserve(cr);
            cairo_set_line_width(cr, 3.0);
            cairo_stroke(cr);
            cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);
        }
    } else {
        if (mode == STYLE_MODE_LIGHT) {
            cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
            cairo_fill_preserve(cr);
            cairo_set_source_rgb(cr, 0.1, 0.1, 0.1);
            cairo_set_line_width(cr, 3.0);
            cairo_stroke(cr);
            cairo_set_source_rgb(cr, 0.1, 0.1, 0.1);
        } else {
            cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);
            cairo_fill_preserve(cr);
            cairo_set_source_rgb(cr, 0.9, 0.9, 0.9);
            cairo_set_line_width(cr, 3.0);
            cairo_stroke(cr);
            cairo_set_source_rgb(cr, 0.9, 0.9, 0.9);
        }
    }

    double c_y = (y_top + y_sh) / 2.0;
    double c_w = petal_w * 0.22;
    double c_h = petal_h * 0.16;

    cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);
    cairo_set_line_join(cr, CAIRO_LINE_JOIN_ROUND);
    cairo_set_line_width(cr, 3.5);

    cairo_move_to(cr, -c_w, c_y + c_h * 0.4);
    cairo_line_to(cr, 0, c_y - c_h * 0.6);
    cairo_line_to(cr, c_w, c_y + c_h * 0.4);
    cairo_stroke(cr);

    cairo_restore(cr);
}

static void draw_action_button(cairo_t *cr, double cx, double cy, double radius, const char *label, gboolean pressed, StyleMode mode) {
    cairo_new_path(cr);
    cairo_arc(cr, cx, cy, radius, 0, 2 * M_PI);

    if (pressed) {
        if (mode == STYLE_MODE_LIGHT) {
            cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);
            cairo_fill_preserve(cr);
            cairo_set_line_width(cr, 3.0);
            cairo_stroke(cr);
            cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
        } else {
            cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
            cairo_fill_preserve(cr);
            cairo_set_line_width(cr, 3.0);
            cairo_stroke(cr);
            cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);
        }
    } else {
        if (mode == STYLE_MODE_LIGHT) {
            cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
            cairo_fill_preserve(cr);
            cairo_set_source_rgb(cr, 0.1, 0.1, 0.1);
            cairo_set_line_width(cr, 3.0);
            cairo_stroke(cr);
            cairo_set_source_rgb(cr, 0.1, 0.1, 0.1);
        } else {
            cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);
            cairo_fill_preserve(cr);
            cairo_set_source_rgb(cr, 0.9, 0.9, 0.9);
            cairo_set_line_width(cr, 3.0);
            cairo_stroke(cr);
            cairo_set_source_rgb(cr, 0.9, 0.9, 0.9);
        }
    }

    cairo_select_font_face(cr, "Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
    cairo_set_font_size(cr, radius * 0.65);
    cairo_text_extents_t te;
    cairo_text_extents(cr, label, &te);
    cairo_move_to(cr, cx - te.width / 2.0 - te.x_bearing, cy + te.height / 2.0);
    cairo_show_text(cr, label);
}

static void draw_capsule_button(cairo_t *cr, double cx, double cy, double w, double h, const char *label, gboolean pressed, StyleMode mode) {
    double r = h / 2.0;
    double x0 = cx - w / 2.0;
    double y0 = cy - h / 2.0;

    cairo_new_path(cr);
    cairo_arc(cr, x0 + w - r, y0 + r, r, -M_PI / 2.0, M_PI / 2.0);
    cairo_arc(cr, x0 + r, y0 + r, r, M_PI / 2.0, 3.0 * M_PI / 2.0);
    cairo_close_path(cr);

    if (pressed) {
        if (mode == STYLE_MODE_LIGHT) {
            cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);
            cairo_fill_preserve(cr);
            cairo_set_line_width(cr, 2.0);
            cairo_stroke(cr);
            cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
        } else {
            cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
            cairo_fill_preserve(cr);
            cairo_set_line_width(cr, 2.0);
            cairo_stroke(cr);
            cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);
        }
    } else {
        if (mode == STYLE_MODE_LIGHT) {
            cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
            cairo_fill_preserve(cr);
            cairo_set_source_rgb(cr, 0.2, 0.2, 0.2);
            cairo_set_line_width(cr, 2.0);
            cairo_stroke(cr);
            cairo_set_source_rgb(cr, 0.2, 0.2, 0.2);
        } else {
            cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);
            cairo_fill_preserve(cr);
            cairo_set_source_rgb(cr, 0.8, 0.8, 0.8);
            cairo_set_line_width(cr, 2.0);
            cairo_stroke(cr);
            cairo_set_source_rgb(cr, 0.8, 0.8, 0.8);
        }
    }

    cairo_select_font_face(cr, "Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
    cairo_set_font_size(cr, 13.0);
    cairo_text_extents_t te;
    cairo_text_extents(cr, label, &te);
    cairo_move_to(cr, cx - te.width / 2.0 - te.x_bearing, cy + te.height / 2.0);
    cairo_show_text(cr, label);
}

static gboolean gb_controls_draw(GtkWidget *widget, GdkEventExpose *event, gpointer data) {
    GtkAllocation alloc;
    gtk_widget_get_allocation(widget, &alloc);
    int width = alloc.width;
    int height = alloc.height;

    GdkWindow *win = gtk_widget_get_window(widget);
    cairo_t *cr = gdk_cairo_create(win);

    if (style_mode == STYLE_MODE_LIGHT) {
        cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
    } else {
        cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);
    }
    cairo_paint(cr);

    // Subtle top border divider line
    if (style_mode == STYLE_MODE_LIGHT) {
        cairo_set_source_rgb(cr, 0.88, 0.88, 0.88);
    } else {
        cairo_set_source_rgb(cr, 0.22, 0.22, 0.22);
    }
    cairo_set_line_width(cr, 1.0);
    cairo_move_to(cr, 0, 0);
    cairo_line_to(cr, width, 0);
    cairo_stroke(cr);

    // D-Pad
    double dpad_cx = width * 0.22;
    double dpad_cy = height * 0.46;
    double petal_w = width * 0.075;
    double petal_h = height * 0.20;
    double center_gap = width * 0.015;
    double r_corner = 14.0;

    draw_dpad_petal(cr, dpad_cx, dpad_cy, petal_w, petal_h, center_gap, r_corner, 0, gb_button_state[GB_UP], style_mode);
    draw_dpad_petal(cr, dpad_cx, dpad_cy, petal_w, petal_h, center_gap, r_corner, M_PI / 2.0, gb_button_state[GB_RIGHT], style_mode);
    draw_dpad_petal(cr, dpad_cx, dpad_cy, petal_w, petal_h, center_gap, r_corner, M_PI, gb_button_state[GB_DOWN], style_mode);
    draw_dpad_petal(cr, dpad_cx, dpad_cy, petal_w, petal_h, center_gap, r_corner, 3.0 * M_PI / 2.0, gb_button_state[GB_LEFT], style_mode);

    // Action Buttons A and B
    double btn_b_x = width * 0.72;
    double btn_b_y = height * 0.54;
    double btn_a_x = width * 0.86;
    double btn_a_y = height * 0.36;
    double btn_r = width * 0.044;

    draw_action_button(cr, btn_b_x, btn_b_y, btn_r, "B", gb_button_state[GB_B], style_mode);
    draw_action_button(cr, btn_a_x, btn_a_y, btn_r, "A", gb_button_state[GB_A], style_mode);

    // Capsule Buttons SELECT and START
    double sel_x = width * 0.44;
    double sel_y = height * 0.86;
    double sta_x = width * 0.56;
    double sta_y = height * 0.86;
    double cap_w = 88.0;
    double cap_h = 28.0;

    draw_capsule_button(cr, sel_x, sel_y, cap_w, cap_h, "SELECT", gb_button_state[GB_SELECT], style_mode);
    draw_capsule_button(cr, sta_x, sta_y, cap_w, cap_h, "START", gb_button_state[GB_START], style_mode);

    cairo_destroy(cr);
    return FALSE;
}

static AudioBackend *audio_backend_init(const int sample_rate) {
    if (!MixerOpenPlay) return NULL;
    AudioBackend *ab = calloc(1, sizeof(AudioBackend));
    ab->mixer_handle = MixerOpenPlay(sample_rate, 2, 16, "Music");
    if (!ab->mixer_handle) {
        fprintf(stderr, "MixerOpenPlay failed\n");
        free(ab);
        return NULL;
    }
    return ab;
}

static void audio_backend_deinit(AudioBackend **pab) {
    if (!pab || !*pab) return;
    AudioBackend *ab = *pab;
    if (ab->mixer_handle) {
        if (MixerWaitTillAudioProcessed) MixerWaitTillAudioProcessed(ab->mixer_handle);
        if (MixerDrain) MixerDrain(ab->mixer_handle);
        if (MixerClose) MixerClose(ab->mixer_handle);
        ab->mixer_handle = NULL;
    }
    free(ab);
    *pab = NULL;
}

static size_t audio_sample_batch(const int16_t *data, size_t frames) {
    if (!ab || !ab->mixer_handle || !MixerGetNumBytes || !MixerGetBufPlay || !MixerReleaseBufPlay) return frames;
    if (MixerGetNumBytes(ab->mixer_handle) > frames * 40 ) return frames;

    int r, s; uint8_t *b = MixerGetBufPlay(ab->mixer_handle, &r, &s);
    if (!b || r != 0 || s <= 0) return frames;
    
    frames = frames > s/4 ? s/4 : frames;
    memcpy(b, data, frames * 4);
    MixerReleaseBufPlay(ab->mixer_handle, frames * 4, b);
    return frames;
}

// STEP 2
static gpointer process_frame_job(gpointer user_data) {
    FrameJob *job = user_data;
    if (!job || !job->data) return NULL;

    const uint16_t *emulator_frame_ptr = job->data;
    int emulator_frame_width     = job->width;
    int emulator_frame_height    = job->height;
    int emulator_frame_rowstride = job->pitch / sizeof(uint16_t);

    int scaled_frame_width  = emulator_frame_width  * SCALE_FACTOR_A;
    int scaled_frame_height = emulator_frame_height * SCALE_FACTOR_A;

    ensure_pixbuf(scaled_frame_width, scaled_frame_height);
    if (!pixbuf) return NULL;

    g_mutex_lock(pixbuf_mutex);
    guchar *dst = gdk_pixbuf_get_pixels(pixbuf);
    int dst_rowstride = gdk_pixbuf_get_rowstride(pixbuf);
    int nchan = gdk_pixbuf_get_n_channels(pixbuf);

    for (int y = 0, y_scaled = 0; y < emulator_frame_height; y++, y_scaled += SCALE_FACTOR_A) {
        const uint16_t *src_row = emulator_frame_ptr + y * emulator_frame_rowstride;
        for (int x = 0, x_scaled = 0; x < emulator_frame_width; x++, x_scaled += SCALE_FACTOR_A) {
            uint8_t gray = grayscale_lut[src_row[x]];
            for (int sy = 0; sy < SCALE_FACTOR_A; sy++) {
                guchar *row = dst + (y_scaled + sy) * dst_rowstride;
                uint8_t threshold = bayer16[(y_scaled + sy) & 15][x_scaled & 15];
                uint8_t bw = (gray >= threshold) ? 255 : 0;
                for (int sx = 0; sx < SCALE_FACTOR_A; sx++) {
                    int px = (x_scaled + sx) * nchan;
                    row[px + 0] = bw;
                    row[px + 1] = bw;
                    row[px + 2] = bw;
                }
            }
        }
    }
    g_mutex_unlock(pixbuf_mutex);

    if (drawing_area) {
        gtk_widget_queue_draw(drawing_area);
    }

    return NULL;
}

static gboolean should_process_frame(void) {
    static int64_t last_frame_time = 0;
    const int FRAME_INTERVAL_US = 1000000 / 30; // 
    int64_t current_time = g_get_monotonic_time();

    if ((current_time - last_frame_time) > FRAME_INTERVAL_US) {
        last_frame_time = current_time;
        return TRUE;
    }
    return FALSE;
}

// frame thread
static gpointer frame_thread_loop(gpointer data){

    struct sched_param param;
    param.sched_priority = 70;

    if (pthread_setschedparam(pthread_self(), SCHED_RR, &param) != 0) {
        perror("pthread_setschedparam for frame_thread_loop");
    }

    while (running) { 
        FrameJob *job = g_async_queue_pop(frame_queue);
        // check sentinel here
        if (job) {
            if (should_process_frame()) {
                process_frame_job(job);
            }
            free(job->data);
            free(job);
        }
    }
   
    return NULL;
}

// emulator thread
static void video_refresh(const void *data, unsigned width, unsigned height, size_t pitch) {
    if (!data) return;

    FrameJob *job = malloc(sizeof(FrameJob));

    job->width  = width;
    job->height = height;
    job->pitch  = pitch;

    if (draw_mode == DRAW_MODE_GTK) {
        if (should_process_frame()){
            job->data = data;
            process_frame_job(job);
            free(job);
        }
        return;
    }

    uint16_t *frame_copy = malloc(height * pitch);
    memcpy(frame_copy, data, height * pitch);
    job->data = frame_copy;

    // queue frame precessing and exit asap to prevent core slow down
    // drop any old frame waiting in the queue and push the newest frame atomically
    g_async_queue_lock(frame_queue);
    FrameJob *old = g_async_queue_try_pop_unlocked(frame_queue);
    if (old) {
        free(old->data); free(old);
    }
    g_async_queue_push_unlocked(frame_queue, job);
    g_async_queue_unlock(frame_queue);

}

static bool environment_cb(unsigned cmd, void *data) {
    // fprintf(stderr, "environment_cb: %i\n", cmd);
    switch (cmd) {
        
        case RETRO_ENVIRONMENT_GET_VARIABLE: {
            struct retro_variable *var = (struct retro_variable *)data;

            // if (strcmp(var->key, "gambatte_gb_colorization") == 0) {
            //     fprintf(stdout, "gambatte_gb_colorization: internal\n");
            //     var->value = "internal";
            //     return true;
            // }
            // if (strcmp(var->key, "gambatte_gb_internal_palette") == 0) {
            //     fprintf(stdout, "gambatte_gb_internal_palette: GBC - Grayscale\n");
            //     var->value = "GBC - Grayscale";
            //     return true;
            // }

            if (strcmp(var->key, "gambatte_gbc_color_correction") == 0) {
                fprintf(stdout, "gambatte_gbc_color_correction: disabled\n");
                var->value = "disabled";
                return true;
            }
            if (strcmp(var->key, "gambatte_mix_frames") == 0) {
                fprintf(stdout, "gambatte_mix_frames: disabled\n");
                var->value = "disabled";
                return true;
            }

            // Default fallback
            var->value = NULL;
            return false;
        }
        
        case RETRO_ENVIRONMENT_SET_PIXEL_FORMAT: {
            enum retro_pixel_format *fmt = (enum retro_pixel_format *)data;
            if (*fmt == RETRO_PIXEL_FORMAT_RGB565) {
                return true;
            }
            return false;
        }

        case RETRO_ENVIRONMENT_GET_SYSTEM_DIRECTORY: {
            const char **dir = data;
            static const char *system_dir = "./gambatte";
            *dir = system_dir;
            return true;
        }
        
        case RETRO_ENVIRONMENT_GET_CAN_DUPE: {
            bool *can_dupe = data;
            *can_dupe = true;
            return true;
        }

        default:
            return false;
    }
}

static gpointer emulator_thread_fn(gpointer data) {

    // const int frame_interval_us = 16742; // FRAME_USEC;
    /* Normal CFS scheduling on single-core CPU */

    while (running) {

        int64_t start = g_get_monotonic_time();

        g_mutex_lock(game_loop_mutex);
        if (ready) {
            retro_run(); // will trigger video_refresh via callback
        }
        g_mutex_unlock(game_loop_mutex);

        int64_t elapsed = g_get_monotonic_time() - start;

        if (elapsed < frame_interval_us) {                  
            g_usleep(frame_interval_us - elapsed);
        } else {
            g_print("warning negative elapsed: %f\n", (double) (elapsed) / 1000.0);
        }

        // if ( ((double) (elapsed) / 1000.0) > 4 ){
        //     g_print("retro run slow, elapsed: %f\n", (double) (elapsed) / 1000.0);
        // }

    }
    return NULL;
}

// TODO refactor this 
void on_open(GtkButton *button, gpointer user_data) {
        fprintf(stderr, "on_open\n");

        g_mutex_lock(game_loop_mutex);
        
        ready = FALSE; // use mutexes color?

        char *game_path = simple_file_chooser_list(GTK_WINDOW(window));
        g_print("Selected: %s\n", game_path);
        if (game_path) {
            g_print("game_path: %s\n", game_path);

            if (!load_core()) {
                g_print("Failed to load libretro core\n");
                g_mutex_unlock(game_loop_mutex);
                g_free(game_path);
                return;
            }

            if(game.data) {
                if (retro_unload_game) retro_unload_game();
                if (retro_reset) retro_reset();
                if (retro_deinit) retro_deinit();
                audio_backend_deinit(&ab);
            }

            if (retro_set_environment) retro_set_environment(environment_cb);
            if (retro_set_video_refresh) retro_set_video_refresh(video_refresh);
            if (retro_set_audio_sample_batch) retro_set_audio_sample_batch(audio_sample_batch);
            if (retro_set_input_poll) retro_set_input_poll(input_poll);
            if (retro_set_input_state) retro_set_input_state(input_state);

            fprintf(stdout, "Loaded core symbols\n");

            if (retro_init) retro_init();

            g_print("game_path 2: %s\n", game_path);
 

            FILE *game_file = fopen(game_path, "rb");
            if (!game_file) {
                fprintf(stderr, "Failed to open game file\n");
                g_mutex_unlock(game_loop_mutex);
                return;
            }
            fseek(game_file, 0, SEEK_END);
            game.size = ftell(game_file);
            fseek(game_file, 0, SEEK_SET);
            game.data = malloc(game.size);
            fread((void*)game.data, 1, game.size, game_file);
            fclose(game_file);

            if (!retro_load_game(&game)) {
                fprintf(stderr, "Failed to load game\n");
                g_mutex_unlock(game_loop_mutex);
                return;
            }

            struct retro_system_av_info av_info;
            retro_get_system_av_info(&av_info);

            audio_sample_rate = av_info.timing.sample_rate;
            video_fps         = av_info.timing.fps;
            frame_interval_us = (int) (1000000.0 / video_fps);

            ab = audio_backend_init( 32760 );

            g_print("Libretro core reports sample rate: %.2f Hz sample_rate:: %f fusec: %d \n", video_fps, audio_sample_rate, frame_interval_us );

            g_atomic_int_set(&refresh_screen, 10);

            // libretro.cpp:2589
            char raw_name[17];
            strncpy(raw_name, (const char*)game.data + 0x134, sizeof(raw_name) - 1);
            raw_name[sizeof(raw_name) - 1] = '\0';

            sanitize_filename(raw_name, internal_game_name, sizeof(internal_game_name));

            g_print("internal_game_name: %s\n", internal_game_name);

            if (title_label && internal_game_name[0] != 0) {
                gtk_label_set_text(GTK_LABEL(title_label), internal_game_name);
            }
                        
            ready = TRUE;
        } else if (game.data) {
            ready = TRUE;
        }
        g_mutex_unlock(game_loop_mutex);
        g_free(game_path);
}

/////
void save_state(const char *filename) {
    if (!retro_serialize_size || !retro_serialize) return;
    size_t size = retro_serialize_size();
    if (size == 0) return;
    void *state = malloc(size);
    if (!state) return;
    if (retro_serialize(state, size)) {
        // char abs_path[PATH_MAX];
        // g_print("retro_serialize: \n");
        // realpath(filename, abs_path);
        // g_print("Saving state to: %s\n", abs_path);        
        FILE *f = fopen(filename, "wb");
        if (f) {
            fwrite(state, 1, size, f);
            fclose(f);
        }
    }
    free(state);
}

void load_state(const char *filename) {
    if (!retro_serialize_size || !retro_unserialize) return;
    size_t size = retro_serialize_size();
    if (size == 0) return;
    void *state = malloc(size);
    if (!state) return;
    FILE *f = fopen(filename, "rb");
    if (f) {
        fread(state, 1, size, f);
        fclose(f);
        retro_unserialize(state, size);
    }
    free(state);
}

static void on_option_selected(GtkWidget *button, gpointer user_data) {
    int option = GPOINTER_TO_INT(user_data);
    GtkWidget *dialog = gtk_widget_get_toplevel(button);
    
    // Handle selected option (option 1-5)
    g_print("Selected option: %d\n", option);
    
    // Close dialog
    gtk_dialog_response(GTK_DIALOG(dialog), option);
}

static int load_save_dialog(GtkWindow *parent, StateFileEntry *entries, int action) {
    GtkWidget *dialog = gtk_dialog_new_with_buttons(
       "L:A_D:application_ID:gambatteLoadSaveDialog",
        parent,
        GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
        GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,        
        NULL
    );
    
    gtk_window_set_default_size(GTK_WINDOW(dialog), 300, 400);
    GtkWidget *content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    gtk_container_set_border_width(GTK_CONTAINER(content_area), 15);
    GtkWidget *padding_frame = gtk_alignment_new(0.5, 0.5, 1.0, 1.0);
    gtk_alignment_set_padding(GTK_ALIGNMENT(padding_frame), 15, 15, 20, 20);  // Top, bottom, left, right
    gtk_container_add(GTK_CONTAINER(content_area), padding_frame);
    GtkWidget *vbox = gtk_vbox_new(TRUE, 10);
    gtk_container_add(GTK_CONTAINER(padding_frame), vbox);
    
    for (int i = 0; i < MAX_STATE_FILES; i++) {
        gchar *label;

        if (entries[i].exists) {
            label = g_strdup_printf("slot %d: %s", i + 1, entries[i].modified_time);
        } else {
            label = g_strdup_printf("slot %d: (empty)", i + 1);
        }

        GtkWidget *btn = gtk_button_new_with_label(label);
        
        if (action == STATE_MODE_SAVE || (action == STATE_MODE_LOAD && entries[i].exists) )
            g_signal_connect(btn, "clicked", G_CALLBACK(on_option_selected), GINT_TO_POINTER(i));

        gtk_box_pack_start(GTK_BOX(vbox), btn, FALSE, FALSE, 0);
        g_free(label);
    }
    
    // Show and run dialog
    gtk_widget_show_all(dialog);
    gint res = gtk_dialog_run(GTK_DIALOG(dialog)); 
    gtk_widget_destroy(dialog);
    return res;
}

void sanitize_filename(const char *input, char *output, size_t maxlen) {
    size_t j = 0;
    for (size_t i = 0; input[i] != '\0' && j < maxlen - 1; ++i) {
        char c = input[i];
        if (isalnum((unsigned char)c) || c == '_' || c == '-' || c == '.') {
            output[j++] = c;
        }
    }
    output[j] = '\0';
}

void on_state_mode_click(GtkButton *button, gpointer user_data) {
    
    int action = GPOINTER_TO_INT (user_data);
    
    g_mutex_lock(game_loop_mutex);
    
    // ready = FALSE; // use mutexes color?

    StateFileEntry entries[MAX_STATE_FILES];

    for (int i = 0; i < MAX_STATE_FILES; ++i) {
        snprintf(entries[i].filename, MAX_FILENAME_LEN, "./state/%s.%d", internal_game_name, i + 1);

        struct stat st;
        if (stat(entries[i].filename, &st) == 0) {
            time_t time = st.st_mtime;
            struct tm * stime = localtime(&time);
            strftime(entries[i].modified_time, 2048, "%Y-%m-%d %H:%M", stime);
            entries[i].exists = 1;
        } else {
            entries[i].exists = 0;
        }        

        g_print("file %s %d %s\n", entries[i].filename, entries[i].exists, entries[i].modified_time);
    }

    gint res = load_save_dialog(GTK_WINDOW(window), entries, action);
    
    if (res >= 0 && res < MAX_STATE_FILES) {
        
        if (action == STATE_MODE_LOAD)
            load_state( entries[res].filename );

        if (action == STATE_MODE_SAVE)
            save_state( entries[res].filename );

    }
    g_print("on_load option: %d\n", res);
    
    g_mutex_unlock(game_loop_mutex); // ready = TRUE;
}

void on_refresh(GtkButton *button, gpointer user_data) {
    g_mutex_lock(game_loop_mutex);
    g_mutex_lock(pixbuf_mutex);
    fprintf(stdout, "refresh_screen\n");
    if (fbink_refresh) {
        fbink_refresh(fbfd_refresh, drawing_area_abs_y, drawing_area_abs_x, PIXBUF_WIDTH, PIXBUF_HEIGHT, &fbink_cfg_refresh);
    }
    g_mutex_unlock(game_loop_mutex);
    g_mutex_unlock(pixbuf_mutex);
    g_atomic_int_set(&refresh_screen, 30);
    if (drawing_area) {
        gtk_widget_queue_draw(drawing_area);
    }
}

// GTK button callback to quit
void on_quit(GtkButton *button, gpointer user_data) {
    running = FALSE; // Signal the thread to stop
    gtk_main_quit();
}
static void on_save_state_click(GtkMenuItem *item, gpointer user_data) {
    on_state_mode_click(NULL, GINT_TO_POINTER(STATE_MODE_SAVE));
}

static void on_load_state_click(GtkMenuItem *item, gpointer user_data) {
    on_state_mode_click(NULL, GINT_TO_POINTER(STATE_MODE_LOAD));
}

static void apply_theme(void) {
    GdkColor bg_color, fg_color;
    if (style_mode == STYLE_MODE_DARK) {
        gdk_color_parse("#000000", &bg_color);
        gdk_color_parse("#FFFFFF", &fg_color);
    } else {
        gdk_color_parse("#FFFFFF", &bg_color);
        gdk_color_parse("#000000", &fg_color);
    }

    if (window) gtk_widget_modify_bg(window, GTK_STATE_NORMAL, &bg_color);
    if (drawing_area) gtk_widget_modify_bg(drawing_area, GTK_STATE_NORMAL, &bg_color);
    if (drawing_area_container) gtk_widget_modify_bg(drawing_area_container, GTK_STATE_NORMAL, &bg_color);
    if (joystick_area) gtk_widget_modify_bg(joystick_area, GTK_STATE_NORMAL, &bg_color);
    if (hamburger_btn) {
        gtk_widget_modify_bg(hamburger_btn, GTK_STATE_NORMAL, &bg_color);
        gtk_widget_modify_fg(hamburger_btn, GTK_STATE_NORMAL, &fg_color);
    }
    if (title_label) {
        gtk_widget_modify_fg(title_label, GTK_STATE_NORMAL, &fg_color);
    }

    if (window) gtk_widget_queue_draw(window);
    if (joystick_area) gtk_widget_queue_draw(joystick_area);
    g_atomic_int_set(&refresh_screen, 30);
}

static void on_toggle_theme_click(GtkMenuItem *item, gpointer user_data) {
    style_mode = (style_mode == STYLE_MODE_LIGHT) ? STYLE_MODE_DARK : STYLE_MODE_LIGHT;
    apply_theme();
}

static void create_popup_menu(GtkWidget *win) {
    popup_menu = gtk_menu_new();

    GtkWidget *item_open = gtk_menu_item_new_with_label("📂  Open ROM...");
    g_signal_connect(item_open, "activate", G_CALLBACK(on_open), NULL);
    gtk_menu_shell_append(GTK_MENU_SHELL(popup_menu), item_open);

    GtkWidget *item_save = gtk_menu_item_new_with_label("💾  Save State");
    g_signal_connect(item_save, "activate", G_CALLBACK(on_save_state_click), NULL);
    gtk_menu_shell_append(GTK_MENU_SHELL(popup_menu), item_save);

    GtkWidget *item_load = gtk_menu_item_new_with_label("📥  Load State");
    g_signal_connect(item_load, "activate", G_CALLBACK(on_load_state_click), NULL);
    gtk_menu_shell_append(GTK_MENU_SHELL(popup_menu), item_load);

    GtkWidget *sep1 = gtk_separator_menu_item_new();
    gtk_menu_shell_append(GTK_MENU_SHELL(popup_menu), sep1);

    GtkWidget *item_theme = gtk_menu_item_new_with_label("🌓  Toggle Light / Dark");
    g_signal_connect(item_theme, "activate", G_CALLBACK(on_toggle_theme_click), NULL);
    gtk_menu_shell_append(GTK_MENU_SHELL(popup_menu), item_theme);

    GtkWidget *item_refresh = gtk_menu_item_new_with_label("🔄  Refresh Screen");
    g_signal_connect(item_refresh, "activate", G_CALLBACK(on_refresh), NULL);
    gtk_menu_shell_append(GTK_MENU_SHELL(popup_menu), item_refresh);

    GtkWidget *sep2 = gtk_separator_menu_item_new();
    gtk_menu_shell_append(GTK_MENU_SHELL(popup_menu), sep2);

    GtkWidget *item_quit = gtk_menu_item_new_with_label("❌  Quit");
    g_signal_connect(item_quit, "activate", G_CALLBACK(on_quit), NULL);
    gtk_menu_shell_append(GTK_MENU_SHELL(popup_menu), item_quit);

    gtk_widget_show_all(popup_menu);
}

void on_hamburger_click(GtkButton *button, gpointer user_data) {
    if (!popup_menu) {
        create_popup_menu(window);
    }
    gtk_menu_popup(GTK_MENU(popup_menu), NULL, NULL, NULL, NULL, 0, gtk_get_current_event_time());
}

static void init_gtk_and_window() {
    gtk_rc_parse_string(
        "style \"custom-button\" {\n"
        "  bg[NORMAL]      = \"#FFFFFF\"\n"
        "  fg[NORMAL]      = \"#000000\"\n"
        "  bg[ACTIVE]      = \"#000000\"\n"
        "  fg[ACTIVE]      = \"#FFFFFF\"\n"
        "  bg[PRELIGHT]    = \"#FFFFFF\"\n"
        "  fg[PRELIGHT]    = \"#000000\"\n"
        "  xthickness = 4\n"
        "  ythickness = 4\n"
        "  font_name = \"Sans Bold 14\"\n"
        "  GtkWidget::focus-line-width = 0\n"
        "  GtkWidget::focus-padding = 0\n"
        "}\n"
        "style \"menu-style\" {\n"
        "  bg[NORMAL]   = \"#FFFFFF\"\n"
        "  fg[NORMAL]   = \"#000000\"\n"
        "  bg[PRELIGHT] = \"#000000\"\n"
        "  fg[PRELIGHT] = \"#FFFFFF\"\n"
        "  font_name = \"Sans 13\"\n"
        "  xthickness = 10\n"
        "  ythickness = 8\n"
        "}\n"
        "style \"title-style\" {\n"
        "  fg[NORMAL] = \"#000000\"\n"
        "  font_name = \"Sans Bold 10\"\n"
        "}\n"
        "style \"white-dialog\" {\n"
        "  bg[NORMAL] = \"#FFFFFF\"\n"
        "  fg[NORMAL] = \"#000000\"\n"
        "  font_name = \"Sans 12\"\n"
        "  xthickness = 20\n"
        "  ythickness = 20\n"
        "}\n"
        "widget \"*hamburger_btn*\" style \"custom-button\"\n"
        "widget \"*title_label*\" style \"title-style\"\n"
        "widget_class \"*GtkMenuItem*\" style \"menu-style\"\n"
        "widget_class \"*GtkMenu*\" style \"menu-style\"\n"
        "widget_class \"*GtkDialog\" style \"white-dialog\"\n"
    );

    GtkBuilder *builder = gtk_builder_new();
    GError *error = NULL;

    if (!gtk_builder_add_from_string(builder, (const gchar *)main_ui, main_ui_len, &error)) {
        g_error("Failed to load UI: %s", error->message);
        g_error_free(error);
        return;
    }

    gtk_builder_connect_signals(builder, NULL);

    window         = GTK_WIDGET(gtk_builder_get_object(builder, "main_window"));
    drawing_area   = GTK_WIDGET(gtk_builder_get_object(builder, "drawing_area"));
    drawing_area_container = GTK_WIDGET(gtk_builder_get_object(builder, "drawing_area_container"));
    joystick_area  = GTK_WIDGET(gtk_builder_get_object(builder, "joystick_area"));
    hamburger_btn  = GTK_WIDGET(gtk_builder_get_object(builder, "hamburger_btn"));
    title_label    = GTK_WIDGET(gtk_builder_get_object(builder, "title_label"));
    GtkWidget *center_hbox = GTK_WIDGET(gtk_builder_get_object(builder, "center_hbox"));

    g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);
    g_signal_connect(drawing_area, "expose-event", G_CALLBACK(expose_event), NULL);
    g_signal_connect(drawing_area, "configure-event", G_CALLBACK(on_area_configure_event), NULL);
    g_signal_connect(joystick_area, "expose-event", G_CALLBACK(gb_controls_draw), NULL);
    g_signal_connect(joystick_area, "configure-event", G_CALLBACK(on_area_configure_event), NULL);

    if (hamburger_btn) {
        g_signal_connect(hamburger_btn, "clicked", G_CALLBACK(on_hamburger_click), NULL);
    }

    gtk_widget_set_name(window, "window");
    gtk_widget_set_name(drawing_area, "drawing_area");
    gtk_widget_set_name(drawing_area_container, "drawing_area_container");
    gtk_widget_set_name(joystick_area, "joystick_area");

    GdkScreen *screen = gdk_screen_get_default();
    int screen_width  = gdk_screen_get_width(screen);
    int screen_height = gdk_screen_get_height(screen);
    gtk_window_set_default_size(GTK_WINDOW(window), screen_width, screen_height);
    gtk_window_set_title(GTK_WINDOW(window), "L:A_N:application_ID:gambatte_PC:N_O:URL_DM:PZ");
    gtk_window_fullscreen(GTK_WINDOW(window));
    gtk_window_set_decorated(GTK_WINDOW(window), FALSE);

    if (screen_width >= 1000) {
        SCALE_FACTOR_A = 6; /* Kindle Paperwhite 3 / Voyage / Oasis (1072x1448 -> 960x864) */
    } else if (screen_width >= 700) {
        SCALE_FACTOR_A = 4; /* Kindle Paperwhite 2 (758x1024 -> 640x576) */
    } else {
        SCALE_FACTOR_A = 3; /* Kindle Touch / Basic (600x800 -> 480x432) */
    }

    int center_hbox_height = SCALE_FACTOR_A * 144 + (2 * SCREEN_BORDER);
    if (center_hbox) {
        gtk_widget_set_size_request(center_hbox, screen_width, center_hbox_height);
    }
    gtk_widget_set_size_request(drawing_area, 160 * SCALE_FACTOR_A, 144 * SCALE_FACTOR_A);
    gtk_widget_set_size_request(drawing_area_container, 160 * SCALE_FACTOR_A + SCREEN_BORDER * 2, 144 * SCALE_FACTOR_A + SCREEN_BORDER * 2);
    gtk_widget_set_double_buffered(drawing_area, TRUE);

    apply_theme();
    gtk_widget_show_all(window);
    g_object_unref(builder);
}

// Main function
int main(int argc, char *argv[]) {

    init_grayscale_lut(contrast, brightness);
    init_bayer16_thresholds();

    if (fbink_init) {
        fbink_cfg.wfm_mode = WFM_A2;
        fbink_cfg.dithering_mode = HWD_PASSTHROUGH;
        fbink_init(fbfd, &fbink_cfg);

        fbink_cfg_refresh.wfm_mode = WFM_AUTO;
        fbink_cfg_refresh.dithering_mode = HWD_PASSTHROUGH;
        fbink_cfg_refresh.is_flashing = true;
        fbink_init(fbfd_refresh, &fbink_cfg_refresh);
    }

    // threading
#if !GLIB_CHECK_VERSION(2, 32, 0)
    g_thread_init(NULL);
#endif
    gdk_threads_init();

    // mutexes
    pixbuf_mutex    = g_mutex_new();
    slots_mutex     = g_mutex_new();
    game_loop_mutex = g_mutex_new();

    // queues
    frame_queue     = g_async_queue_new();

    // gtk
    gtk_init(&argc, &argv);

    init_gtk_and_window();

    // Start game loop
    GThread *emulator_thread = g_thread_create(emulator_thread_fn, NULL, FALSE, NULL);
    GThread *touch_thread    = g_thread_create(touch_thread_fn, NULL, FALSE, NULL);    
    GThread *frame_thread    = g_thread_create(frame_thread_loop, NULL, FALSE, NULL);    

    gtk_widget_show_all(window);
    gdk_threads_enter();

    gtk_main();
    gdk_threads_leave();

    audio_backend_deinit(&ab);
    
    // Cleanup
    if (game.data) {
        if (retro_deinit) retro_deinit();
        free((void*)game.data);
    }

    if (pixbuf) g_object_unref(pixbuf);
    if (grayscale_frame_ptr) free(grayscale_frame_ptr);
    g_mutex_free(pixbuf_mutex);
    
    return EXIT_SUCCESS;
}