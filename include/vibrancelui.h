// SPDX-License-Identifier: GPL-2.0-only

#ifndef VIBRANCELUI_H
#define VIBRANCELUI_H

#include <inttypes.h>
#include <X11/Xlib.h>
#include <NVCtrl/NVCtrl.h>
#include <NVCtrl/NVCtrlLib.h>

#include "gtkgui.h"

#define DIE(msg) do { perror(msg); exit(EXIT_FAILURE); } while (0)

#ifdef DEBUG
#   define DEBUG_PRINTF(fmt, args...) fprintf(stderr, "DEBUG: %s:%d:%s(): " fmt,        \
            __FILE__, __LINE__, __func__, ##args)
#else
#   define DEBUG_PRINTF(fmt, args...)
#endif

typedef struct per_monitor_settings {
    int vibrance_level;
    int64_t max_vibrance,
            min_vibrance;
    bool is_active;
    int dpyId;
} monitor_config_t;

/* TODO: Implement @monitors as linked list */
typedef struct global_display {
    Display *dpy;
    monitor_config_t *monitors;
    /* @data - array taken from XNVCTRLQueryTargetBinaryData.
        number of displays is in @ndisplays */
    int *data;
    int ndisplays;
} global_display_t;

/* Convert between digital vibrance value to a percentage */
static inline int
dv_value_to_percentage(int value, monitor_config_t *monitor)
{
    return ((value - monitor->min_vibrance) * (100)) /
                (monitor->max_vibrance - monitor->min_vibrance);
}

/* Convert between digital vibrance percentage to value */
static inline int
dv_percentage_to_value(int percentage, monitor_config_t *monitor)
{
    return (1.0f/100.0f) * ((percentage * monitor->max_vibrance) +
            (100 * monitor->min_vibrance) - (percentage * monitor->min_vibrance));
}

void set_monitor_vibrance(int, int,
                        bool, unsigned int);
bool get_monitor_digital_vibrance(int *, int);
extern global_display_t gdisplay;


#endif /* VIBRANCELUI_H */