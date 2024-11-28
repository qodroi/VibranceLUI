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

#include <gtk/gtk.h>
#include <NVCtrl/NVCtrl.h>
#include <NVCtrl/NVCtrlLib.h>

#include "vgui.h"

#define DEFAULT_DP_VIBRANCE_LEVEL           0
#define DEFAULT_MAX_VIBRANCE_LEVEL          1023
#define DEFAULT_MIN_VIBRANCE_LEVEL          -1024

#define DIE(msg) do { perror(msg); exit(EXIT_FAILURE); } while (0)

#ifdef DEBUG
#   define DEBUG_PRINTF(fmt, args...) fprintf(stderr, "DEBUG: %s:%d:%s(): " fmt,        \
            __FILE__, __LINE__, __func__, ##args)
#else
#   define DEBUG_PRINTF(fmt, args...)
#endif

typedef struct per_monitor_settings {
    int vibrance_level;
    int width, height;
    int64_t max_vibrance,
            min_vibrance;
    int dpyId;
} monitor_config_t;

typedef struct global_display {
    pthread_spinlock_t lock;
    Display *dpy;
    monitor_config_t *monitors_conf;
    int *data; /* array taken from XNVCTRLQueryTargetBinaryData */
    int ndisplays; /* "Screen" by X11's defintion is different. */
} global_display_t;

/* Private user configuartion structure */
typedef struct app_user_data {
	GdkDisplay *user_gdk_display;
	GListModel *glist_monitors; /* Monitors belong to Display (user_display) */
	guint nm; /* Number of monitors (fetched from GListModel) */
	bool affect_all; /* Affect all monitors? */
	int dropd_def_mon; /* Default monitor to affect; set by DropDown */
} user_data_t;

/* Convert between digital vibrance value to a percentage */
int inline
dv_value_to_percentage(int value, monitor_config_t *monitor_conf)
{
    return ((value - monitor_conf->min_vibrance) * (100)) /
                (monitor_conf->max_vibrance - monitor_conf->min_vibrance);
}

/* Convert between digital vibrance percentage to value */
int inline
dv_percentage_to_value(int percentage, monitor_config_t *monitor_conf)
{
    return (1.0f/100.0f) * ((percentage * (monitor_conf ? monitor_conf->max_vibrance : 1023)) +      \
            (100 * (monitor_conf ? monitor_conf->min_vibrance : -1024)) -                            \
            (percentage * (monitor_conf ? monitor_conf->min_vibrance : -1024)));
}

void set_monitor_vibrance(int, int,
                        bool);
bool get_monitor_vibrance(int *, int);

void __reset_monitor_vibrance(int, bool);

extern global_display_t gdisplay;
extern user_data_t user_data;

#endif /* VIBRANCELUI_H */
