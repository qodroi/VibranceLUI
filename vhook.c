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

#include <X11/Xatom.h>

#include "vibrancelui.h"
#include "ghashtable.h"
#include "vhook.h"

unsigned char *prop;

#define _NET_WM_WINDOW_TYPE_DESKTOP     427LU /* FIXME: It might not be a hardcoded value. */

static __always_inline void check_function_status(int status, Window window)
{
	if (status == BadWindow)
		DEBUG_PRINTF("window id # 0x%lx does not exist!\n", window);
	if (status != Success)
		DEBUG_PRINTF("XGetWindowProperty failed!\n");
}

/* This function takes a graphical window id (e.g. terminal window id) 
	and compares its width and height against the current monitor width and height
	in order to determine if this application is in full-screen mode.
*/
static Status is_window_full_screen(unsigned long window_id)
{
	XWindowAttributes win_attributes;
	Status status;

	status = XGetWindowAttributes(gdisplay.dpy, (Window)window_id, &win_attributes);
	if (status == BadWindow) {
		DEBUG_PRINTF("Couldn't fetch window (%lx) properties (status code: %d)\n",
				 window_id, status);
		return false;
	}
	
	if (win_attributes.width == gdisplay.monitors_conf[user_data.dropd_def_mon].width
				 && win_attributes.height == gdisplay.monitors_conf[user_data.dropd_def_mon].height)
		return true;
	return false;
}

static unsigned char *get_string_property(const char *property_name, Window window)
{
	Atom actual_type, filter_atom;
	int actual_format, status;
	unsigned long nitems, bytes_after;

	filter_atom = XInternAtom(gdisplay.dpy, property_name, True);
	status = XGetWindowProperty(gdisplay.dpy, window, filter_atom, 0, MAXSTR, False, AnyPropertyType,
								&actual_type, &actual_format, &nitems, &bytes_after, &prop);
	check_function_status(status, window);

	return prop;
}

static unsigned long __always_inline get_long_property(const char *property_name, Window window)
{
	unsigned long long_property;

	get_string_property(property_name, window);

	if (prop == NULL)
		return false;

	long_property = (unsigned long)(prop[0] + (prop[1] << 8) + (prop[2] << 16) + (prop[3] << 24));

	return long_property;
}

static __always_inline const unsigned char *get_window_name(unsigned long active_window)
{
	return get_string_property("WM_CLASS", active_window);
}

static unsigned long __always_inline get_active_window_pid(unsigned long active_window)
{
	return get_long_property("_NET_WM_PID", active_window);
}

static unsigned long __always_inline get_active_window_id(Window root_window)
{
	return get_long_property("_NET_ACTIVE_WINDOW", root_window);
}

static bool __always_inline is_window_type_desktop(unsigned long active_window)
{
	return get_long_property("_NET_WM_WINDOW_TYPE", active_window) ==
			_NET_WM_WINDOW_TYPE_DESKTOP ? true : false;
}

static bool __attribute__((hot))
handle_active_window(unsigned long active_window)
{
	unsigned long window_pid;
	char pid_buf[21 + 1] = { 0 };
	const char *dv_level_for_pid;

	__reset_monitor_vibrance(0, true); /* Focus was changed.
				Reset all monitors digital vibrance */

	/* Did we focus on our desktop? */
	if (is_window_type_desktop(active_window))
		return false;

	/* Note that @active_window is actually just the Window's ID */
	window_pid = get_active_window_pid(active_window);
	if (window_pid == 0)
		return false;

#ifdef DEBUG
	DEBUG_PRINTF("%s %lx %lu\n", get_window_name(active_window), active_window, window_pid);
#endif

	/* FIXME: Currently we set vibrance on _all_ monitors;
		Find the monitor that the application was opened on */
	
	snprintf(pid_buf, sizeof(pid_buf), "%lu", window_pid);

	if ((dv_level_for_pid = fetch_vlevel_for_spid_ht(pid_buf)) == NULL)
		return false;
	if (is_window_full_screen(active_window)) 
	{
		int _v = 0;
	
		_v = strtol(dv_level_for_pid, (char **)NULL, 10);
		_v = dv_percentage_to_value(_v > 100 ? 100 : _v, NULL);

		set_monitor_vibrance(0, _v, true); /* FIXME: Change "true" to current primary monitor */
		return true;
	}
	
	return false;
}

void *vib_app_hook_thread_start(void *)
{
	Window w;
	XEvent e;
	unsigned long active_window;    

	w = DefaultRootWindow(gdisplay.dpy);
	if (w == 0)
		return NULL;

	XSelectInput(gdisplay.dpy, w, PropertyChangeMask);

	/* Use spinlock to prevent TOCTOU race condition with @gdisplay.dpy being null
		after application close, with this function possibly runs one last time after close.
		Though quite heavy, that's the only solution I found.

		I think we could use atomic operations on @gdisplay.dpy, but I need to figure out
		how to implement it correctly.
	*/
	for (;;) {
		/* Expect positive return value,
			 as until close no one wil take the lock. */
		if (__builtin_expect(pthread_spin_trylock(&gdisplay.lock), 0))
			break;
		XNextEvent(gdisplay.dpy, &e);
		if (e.type == PropertyNotify)
		{
			if (!strcmp(XGetAtomName(gdisplay.dpy, e.xproperty.atom), "_NET_ACTIVE_WINDOW")) {
				active_window = get_active_window_id(w);
				handle_active_window(active_window);
			}
		}
		pthread_spin_unlock(&gdisplay.lock);
	}

	return NULL;
}
