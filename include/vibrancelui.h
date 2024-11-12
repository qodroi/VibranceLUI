/*
 *   Copyright (c) 2024 Roi

 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.

 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.

 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef VIBRANCELUI_H
#define VIBRANCELUI_H

#include <stdbool.h>
#include <inttypes.h>
#include <NVCtrl/NVCtrl.h>
#include <NVCtrl/NVCtrlLib.h>

#include "vibgui.h"

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
static __always_inline int
dv_value_to_percentage(int value, monitor_config_t *monitor)
{
    return ((value - monitor->min_vibrance) * (100)) /
                (monitor->max_vibrance - monitor->min_vibrance);
}

/* Convert between digital vibrance percentage to value */
static __always_inline int
dv_percentage_to_value(int percentage, monitor_config_t *monitor)
{
    return (1.0f/100.0f) * ((percentage * (monitor ? monitor->max_vibrance : 1023)) +
            (100 * (monitor ? monitor->min_vibrance : -1024)) - (percentage * (monitor ? monitor->min_vibrance : -1024)));
}

void set_monitor_vibrance(int, int,
                        bool, unsigned int);
bool get_monitor_digital_vibrance(int *, int);
extern global_display_t gdisplay;


#endif /* VIBRANCELUI_H */
