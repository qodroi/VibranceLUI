// SPDX-License-Identifier: GPL-2.0-only

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

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
            printf("Default X screen %d is not an NVIDIA X screen.  "
                   "Using X screen %d instead.\n",
                   def_scn, screen);
            return screen;
        }
    }

    DIE("Unable to find any NVIDIA X screens; aborting.\n");
}

/* Query the current set digital vibrance for the given @ndisplay,
 * and store the return value in vibrance
 */
bool __always_inline
get_monitor_digital_vibrance(int *ret_vibrance, int dpyId)
{
    return XNVCTRLQueryTargetAttribute(gdisplay.dpy, NV_CTRL_TARGET_TYPE_DISPLAY,
        dpyId, 0, NV_CTRL_DIGITAL_VIBRANCE, ret_vibrance);
}

/* Set the Digital Vibrance of the specified @monitor
     to the target @vibrance_level value */
void
set_monitor_vibrance(int monitor_number, int vibrance_level,
                        bool affect_all, unsigned int nm)
{
    monitor_config_t monitor_conf = gdisplay.monitors[monitor_number];
    // int ret_vibrance = 0;
    int mon = 0; /* None of my monitors have dpyID of 0, but just in case. */

    if ((!(monitor_conf.min_vibrance <= vibrance_level && vibrance_level <= monitor_conf.max_vibrance
            )) || (vibrance_level == 0 && !monitor_conf.is_active))
        return;

    /* FIXME: We assume @gdisplay.dpy (*Display) is still open, though it may
     * have been closed by the time @main calls XOpenDisplay

     -----

     Use gdisplay.monitors[index].dpyID instead of mon
    */

    do {
        XNVCTRLSetTargetAttribute(gdisplay.dpy, /* Dpy */
                                NV_CTRL_TARGET_TYPE_DISPLAY,
                                affect_all == false ? monitor_conf.dpyId : mon, /* dpyID */
                                0, /* Leave it zero */
                                NV_CTRL_DIGITAL_VIBRANCE,
                                vibrance_level);
    } while (affect_all && mon++ <= nm);
    XFlush(gdisplay.dpy); /* We want it right after the setting */

    // get_monitor_digital_vibrance(&ret_vibrance, monitor_conf.dpyId);
    // if (ret_vibrance != vibrance_level) {
    //     DEBUG_PRINTF("failed to set vibrance lvl to: %d (%d) on display: %s\n",
    //          vibrance_level, ret_vibrance, XDisplayName(NULL));
    //     return;
    // }
}

/* Query the valid range of vibrance level for the specified monitor */
static int query_valid_vibrance_levels(monitor_config_t *monitor, int ndisplay)
{
    NVCTRLAttributeValidValuesRec valid_values;
    int ret;

    ret = XNVCTRLQueryValidTargetAttributeValues(gdisplay.dpy,
                NV_CTRL_TARGET_TYPE_DISPLAY,
                ndisplay,
                0,
                NV_CTRL_DIGITAL_VIBRANCE,
                &valid_values);
    if (!ret)
        return ret;
    if (valid_values.type != ATTRIBUTE_TYPE_RANGE)
        return -1;

    monitor->min_vibrance = valid_values.u.range.min;
    monitor->max_vibrance = valid_values.u.range.max;

    return 0;
}

/* Initalize display configuration and data on program launch */
static int device_config_init(int nvscreen)
{
    int i;

    XNVCTRLQueryTargetBinaryData(gdisplay.dpy,
                                NV_CTRL_TARGET_TYPE_X_SCREEN,
                                nvscreen,
                                0,
                                NV_CTRL_BINARY_DATA_DISPLAYS_ENABLED_ON_XSCREEN,
                                (unsigned char **)&gdisplay.data,
                                NULL);
    gdisplay.ndisplays = gdisplay.data[0];

    gdisplay.monitors = malloc(sizeof(*gdisplay.monitors) * gdisplay.ndisplays);
    if (!gdisplay.monitors)
        DIE("Unable to allocate memory for internal structure\n");
    memset(gdisplay.monitors, 0, sizeof(*gdisplay.monitors) * gdisplay.ndisplays);

    for (i = 1; i <= gdisplay.ndisplays; i++) {
        get_monitor_digital_vibrance(&gdisplay.monitors[i - 1].vibrance_level, i);
        if (gdisplay.monitors[i - 1].vibrance_level != 0)
            gdisplay.monitors[i - 1].is_active = true;
        gdisplay.monitors[i - 1].dpyId = i;
        query_valid_vibrance_levels(&gdisplay.monitors[i - 1], i);
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

    nvscreen = GetNvXScreen(gdisplay.dpy);

    device_config_init(nvscreen);
    do_init_gtk_window(argc, argv);
	return 0;
}