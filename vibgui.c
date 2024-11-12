/*
 *   Copyright (c) 2024 Roi

 *   Responsible for all the graphical user interface things, such as callbacks.

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

#include <gtk/gtk.h>
#include <gdk/gdk.h>

#include "vibrancelui.h"

/* Used as a "package" for @g_signal_connect for
	the passed fuction's data. */
struct g_signal_widgets {
	union {
		GtkWidget *pvscale;
		GtkWidget *pcbox;
	};
} pwidgets;

/* Private user configuartion structure */
typedef struct app_user_data {
	GdkDisplay *user_display;
	GListModel *monitors; /* Monitors belong to Display (user_display) */
	guint nm; /* Number of monitors (fetched from GListModel) */
	bool affect_all; /* Affect all monitors or just the default one */
	int default_mon; /* Default monitor set by user */
} user_data_t;

user_data_t user_data = { NULL, NULL, 0, false, 0 };

/* Sets the application's icon file */
// static void gtk_set_icon_by_path(GtkWidget *window)
// {
// 	GtkIconTheme *ico_th;

// 	ico_th = gtk_icon_theme_get_for_display(user_data.user_display);
//     gtk_icon_theme_add_search_path(ico_th, "img/");
//     gtk_window_set_default_icon_name("vibrancelui.webp");
//     gtk_window_set_icon_name(GTK_WINDOW(window), "vibrancelui.webp");
// }

static void gtk_scale_callback(void)
{
	int integer_vibrance_scale;
	gdouble arg_range_val;
	int monitor_number = user_data.default_mon; /* specified monitor number */

	integer_vibrance_scale = dv_percentage_to_value(
								gtk_range_get_value(GTK_RANGE(pwidgets.pvscale)),
								&gdisplay.monitors[monitor_number]);
	arg_range_val = gtk_range_get_value(GTK_RANGE(pwidgets.pvscale));
	set_monitor_vibrance(monitor_number, integer_vibrance_scale,
							user_data.affect_all, user_data.nm);

	DEBUG_PRINTF("%g %d %d\n", arg_range_val, integer_vibrance_scale, user_data.affect_all);
}

/* Called when the "Affect all" button is pressed.
 * Note that monitors vibrance won't change live after button is checked.
 */
static void affect_all_callback(
				GtkCheckButton *check_btn)
{
	user_data.affect_all =
			gtk_check_button_get_active(check_btn) ? (true) : (false);
}

/* FIXME: Can be inefficent to call `get_monitor_digital_vibrance'
		each time a monitor is selected. We can setup a config struct to be loaded instead.
	*/
static void dropdown_selected_callback(GtkDropDown *self)
{
	int dv;

	user_data.default_mon = gtk_drop_down_get_selected(self);
	get_monitor_digital_vibrance(&dv, user_data.default_mon + 1);
	dv = dv_value_to_percentage(dv, &gdisplay.monitors[user_data.default_mon]);
	gtk_range_set_value(GTK_RANGE(pwidgets.pvscale), dv);
}

/* Initalize and config the primary @window widgets */
static void do_gtk_widgets_init(GtkWidget *window, GtkBuilder *build)
{
	GtkWidget *vscale, *checkbtn;
	gint current_vib;

	/* We could also set signals via the UI file itself, but avoid it. */
	pwidgets.pcbox = checkbtn = GTK_WIDGET(gtk_builder_get_object(build, "checkbtn"));
	pwidgets.pvscale = vscale = GTK_WIDGET(gtk_builder_get_object(build, "vscale"));

	get_monitor_digital_vibrance(&current_vib, user_data.default_mon + 1);
	gtk_range_set_value(GTK_RANGE(pwidgets.pvscale),
	 		dv_value_to_percentage(current_vib, &gdisplay.monitors[user_data.default_mon]));

	/* Signals */
	g_signal_connect(GTK_RANGE(vscale), "value_changed",
					 	G_CALLBACK(&gtk_scale_callback),
						&pwidgets);
	g_signal_connect(checkbtn, "toggled",
					 	G_CALLBACK(affect_all_callback), NULL);

	gtk_widget_show(window);
}

static void gtk_activate(GtkApplication *app, gpointer data)
{
	int i = 0;
	GtkWidget *win, *fixed; /* GtkFixed */
	GtkBuilder *build; /* Builder UI */
	GtkCssProvider *cssprov; /* External CSS theme file */
	GtkWidget *checkbtn, *vscale, *credit,
				 *nvi_image, *dropdown_display;
	const char *str_arr_displays[MAXIMUM_MONITOR_ARRAY_COUNT];
	// GtkListItemFactory *factory_list_dropdown;

	const char *credit_text =
		"<a href=\"https://github.com/qodroi\" title=\"&lt;i&gt;Github&lt;/i&gt; Profile\">"
		"Made by Roi</a>";

	user_data.user_display = gdk_display_manager_get_default_display(
								gdk_display_manager_get());
	if (user_data.user_display == NULL)
		DIE("Failed to get current display handle\n");

	user_data.monitors = gdk_display_get_monitors(user_data.user_display);
	user_data.nm = g_list_model_get_n_items(user_data.monitors);

	build = gtk_builder_new_from_file("gtkui.xml");
	win = GTK_WIDGET(gtk_builder_get_object(build, "win"));
	fixed = GTK_WIDGET(gtk_builder_get_object(build, "fixed"));
	gtk_window_set_application(GTK_WINDOW(win), GTK_APPLICATION(app));

	for (; i < g_list_model_get_n_items(user_data.monitors); i++)
	{
		str_arr_displays[i] =
			gdk_monitor_get_model(g_list_model_get_item(user_data.monitors, i));
	}

	checkbtn = GTK_WIDGET(gtk_builder_get_object(build, "checkbtn"));
	vscale = GTK_WIDGET(gtk_builder_get_object(build, "vscale"));
	credit = GTK_WIDGET(gtk_builder_get_object(build, "creditlb"));
	dropdown_display = gtk_drop_down_new_from_strings(str_arr_displays);
	nvi_image = gtk_image_new_from_file("assets/nvidia-ico.png");

	/* FIXME: Avoid this */
	gtk_widget_unparent(checkbtn);
	gtk_widget_unparent(vscale);
	gtk_widget_unparent(credit);

	/* Widget configuration */
	gtk_scale_add_mark(GTK_SCALE(vscale), 50, GTK_POS_TOP,
		 "<span font_size='small' stretch='ultracondensed'>Vibrance Level</span>");
	gtk_label_set_markup(GTK_LABEL(credit), credit_text);
	gtk_image_set_pixel_size(GTK_IMAGE(nvi_image), 170);
	g_signal_connect_after(dropdown_display, "notify::selected",
				 G_CALLBACK(dropdown_selected_callback), NULL);

	/* Position the elements */
	gtk_fixed_put(GTK_FIXED(fixed), checkbtn, 20, 120);
	gtk_fixed_put(GTK_FIXED(fixed), vscale, 20, 180);
	gtk_fixed_put(GTK_FIXED(fixed), credit, 330, 500);
	gtk_fixed_put(GTK_FIXED(fixed), nvi_image, 230, 10);
	gtk_fixed_put(GTK_FIXED(fixed), dropdown_display, 20, 80);

	cssprov = gtk_css_provider_new();

	/* Load external CSS theme file */
	gtk_css_provider_load_from_file(cssprov, g_file_new_for_path("css/gtk.css"));
	gtk_style_context_add_provider_for_display(gtk_widget_get_display(GTK_WIDGET(win)),
			GTK_STYLE_PROVIDER(cssprov), GTK_STYLE_PROVIDER_PRIORITY_USER);

	gtk_window_present(GTK_WINDOW(win));
	do_gtk_widgets_init(win, build);
}

int do_init_gtk_window(
		int argc, char **argv)
{
	GtkApplication *app;
	int status = 0;

	app = gtk_application_new("com.github.qodroi.vibrancelui", G_APPLICATION_DEFAULT_FLAGS);
	g_signal_connect(app, "activate", G_CALLBACK(gtk_activate), NULL);
	status = g_application_run(G_APPLICATION(app), argc, argv);

   	XCloseDisplay(gdisplay.dpy);
	g_object_unref(app);

	return status;
}
