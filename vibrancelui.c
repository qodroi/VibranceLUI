/*
 *   Copyright (c) 2024 Roi

 *   VibranceLUI core that communicated with Nvidia API & does some init stuff.

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


#include <X11/extensions/Xinerama.h>

#include "vibrancelui.h"

global_display_t gdisplay = { 0 };

/* GetNvXScreen doesn't seem to get exported by any of the shared
 * libraries linked to our binary, so we redefine it as weak.
 */
int __attribute__((weak)) GetNvXScreen(Display *dpy)
{
    int def_scn, screen;

    def_scn = DefaultScreen(dpy);

    if (XNVCTRLIsNvScreen(dpy, def_scn))
        return def_scn;

    for (screen = 0; screen < ScreenCount(dpy); screen++) {
        if (XNVCTRLIsNvScreen(dpy, screen)) {
            DEBUG_PRINTF("Default X screen %d is not an NVIDIA X screen.  "
                   "Using X screen %d instead.\n",
                   def_scn, screen);
            return screen;
        }
    }

    DIE("Unable to find any NVIDIA X screens; aborting.\n");
}

/* Query the current applied digital vibrance for the given @dpyId,
 * and store the return value in @ret_vibrance
 */
bool
get_monitor_vibrance(int *ret_vibrance, int dpyId)
{
    return XNVCTRLQueryTargetAttribute(gdisplay.dpy, NV_CTRL_TARGET_TYPE_DISPLAY,
        dpyId, 0, NV_CTRL_DIGITAL_VIBRANCE, ret_vibrance);
}

/* Clear @monitor(s) digital vibrance */
void __always_inline __reset_monitor_vibrance(int monitor_number, bool affect_all)
{
    set_monitor_vibrance(monitor_number, 0, affect_all);
}

/* Set the digital vibrance of the specified @monitor(s)
     to the target @vibrance_level value */
void
set_monitor_vibrance(int monitor_number, int vibrance_level,
                        bool affect_all)
{
    monitor_config_t monitor_conf = gdisplay.monitors_conf[monitor_number];
    int dpyid = 0; /* None of my monitors have dpyID of 0, but just in case. */

   	/* 	Check if @vibrance_level is NOT within the acceptable range. 
		NOTE: This relies on one monitor configuration, I assume they all
		have the same min\max vibrance configuration.
	*/
   	if (!(monitor_conf.min_vibrance <= vibrance_level &&
			vibrance_level <= monitor_conf.max_vibrance))
		return;

    do {
        XNVCTRLSetTargetAttribute(gdisplay.dpy,
                                NV_CTRL_TARGET_TYPE_DISPLAY,
                                affect_all == false ? monitor_conf.dpyId : dpyid,
                                0, /* Leave it zero */
                                NV_CTRL_DIGITAL_VIBRANCE,
                                vibrance_level);
    } while (affect_all && dpyid++ <= user_data.nm);
    XFlush(gdisplay.dpy);
}

/* Query the valid range of vibrance level for the specified monitor */
static int query_valid_vibrance_levels(monitor_config_t *monitor_conf, int dpyid)
{
    NVCTRLAttributeValidValuesRec valid_values;
    int ret;

    ret = XNVCTRLQueryValidTargetAttributeValues(gdisplay.dpy,
                NV_CTRL_TARGET_TYPE_DISPLAY,
                dpyid,
                0,
                NV_CTRL_DIGITAL_VIBRANCE,
                &valid_values);
    if (!ret)
        return ret;
    if (valid_values.type != ATTRIBUTE_TYPE_RANGE)
        return -1;

    monitor_conf->min_vibrance = valid_values.u.range.min;
    monitor_conf->max_vibrance = valid_values.u.range.max;

    return 0;
}

static void query_mon_height_and_width(monitor_config_t *monitor_conf, int scrn)
{
    XineramaScreenInfo *xine_scr;
    int n_entries;

    xine_scr = XineramaQueryScreens(gdisplay.dpy, &n_entries);
    if (xine_scr == NULL)
        return;
    
    monitor_conf->height = xine_scr[scrn].height;
    monitor_conf->width = xine_scr[scrn].width;

    XFree(xine_scr);
}

/* Initalize display configuration and data on program launch */
static int device_display_config_init(int nvscreen)
{
    int dpyid, mon_index;

    XNVCTRLQueryTargetBinaryData(gdisplay.dpy,
                                NV_CTRL_TARGET_TYPE_X_SCREEN,
                                nvscreen,
                                0,
                                NV_CTRL_BINARY_DATA_DISPLAYS_ENABLED_ON_XSCREEN,
                                (unsigned char **)&gdisplay.data,
                                NULL);
    gdisplay.ndisplays = gdisplay.data[0];

    gdisplay.monitors_conf = calloc(sizeof(*gdisplay.monitors_conf) *
                 gdisplay.ndisplays, sizeof(*gdisplay.monitors_conf));
    if (gdisplay.monitors_conf == NULL)
        DIE("Failed to allocate memory for internal structure\n");

    for (dpyid = 1, mon_index = 0; dpyid <= gdisplay.ndisplays; dpyid++, mon_index++) 
    {
        get_monitor_vibrance(&gdisplay.monitors_conf[mon_index].vibrance_level, dpyid);
        gdisplay.monitors_conf[mon_index].dpyId = dpyid;
        query_valid_vibrance_levels(&gdisplay.monitors_conf[mon_index], dpyid);
        query_mon_height_and_width(&gdisplay.monitors_conf[mon_index], mon_index);
    }

    return 0;
}

int main(int argc, char const *argv[])
{
    int nvscreen;
    Display *dpy;

    /* NULL gets the display based on the
        envvar $DISPLAY name */
    dpy = XOpenDisplay(NULL);
    if (!dpy)
        DIE("XOpenDisplay");
    gdisplay.dpy = dpy;

    pthread_spin_init(&gdisplay.lock, PTHREAD_PROCESS_PRIVATE);
    nvscreen = GetNvXScreen(gdisplay.dpy);
    device_display_config_init(nvscreen);
    do_init_gtk_window(argc, argv);
    pthread_spin_destroy(&gdisplay.lock);

	return 0;
}
