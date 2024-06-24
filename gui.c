// SPDX-License-Identifier: GPL-2.0-only

#include <gtk/gtk.h>
#include <gdk/gdk.h>

#include "vibrancelui.h"

struct g_widgets {
	GtkWidget *pvscale;
	GtkWidget *pcbox;
};

/* Private user configuartion structure */
struct app_user_data {
	GdkDisplay *user_display;
	GListModel *monitors; /* Monitors belong to Display (user_display) */
	guint nm; /* Number of monitors (fetched from GListModel) */
	bool affect_all; /* Affect all monitors or just the default one */
	int default_mon; /* Default monitor set by user */
};

static struct app_user_data user_data = { NULL, NULL, 0, false, 1 };

/* 	FIXME: gtk_combo_box_text_get_active_text crashes with segfault.
	To be implemented in the future.
*/
// static gint __attribute_maybe_unused__
// get_current_combobox_display(GtkComboBoxText *display_cb)
// {
// 	gchar *s_current_display_set = NULL;
// 	gint s_display_int = 0; /* Default display n */

// 	s_current_display_set = /* allocates string and must be freed */
// 			gtk_combo_box_text_get_active_text(display_cb);
// 	if (s_current_display_set == NULL)
// 		return s_display_int;

// 	s_display_int = atoi(s_current_display_set);
// 	g_free(s_current_display_set);

// 	return s_display_int;

// }

/* Sets the application's icon file */
// static void gtk_set_icon_by_path(GtkWidget *window)
// {
// 	GtkIconTheme *ico_th;

// 	ico_th = gtk_icon_theme_get_for_display(user_data.user_display);
//     gtk_icon_theme_add_search_path(ico_th, "img/");
//     gtk_window_set_default_icon_name("vibrancelui.webp");
//     gtk_window_set_icon_name(GTK_WINDOW(window), "vibrancelui.webp");
// }

/* TODO: Add Argument that points to a Gtk Object that containes the
 * monitor number to modify its vibrance lvl.
*/
static void gtk_scale_val_change_callback(struct g_widgets *data)
{
	int integer_vibrance_scale;
	gdouble arg_range_val;
	int monitor_number = 0; /* specified monitor number */

	/* See get_current_combobox_display's FIXME */
	// monitor_number =
	// 	get_current_combobox_display(GTK_COMBO_BOX_TEXT(&data->pcbox));
	integer_vibrance_scale = dv_percentage_to_value(
								gtk_range_get_value(GTK_RANGE(&data->pvscale)),
								&gdisplay.monitors[monitor_number]);
	arg_range_val = gtk_range_get_value(GTK_RANGE(&data->pvscale));
	set_monitor_vibrance(monitor_number, integer_vibrance_scale,
							user_data.affect_all, user_data.nm);

	DEBUG_PRINTF("%g %d %d\n", arg_range_val, integer_vibrance_scale, user_data.affect_all);
}

/* Called when a monitor status is changed, e.g. monitor is plugged in. */
static inline void gtk_monitor_change_callback(GListModel *monitors_list)
{
	user_data.nm = g_list_model_get_n_items(monitors_list);
	if (user_data.nm == 0)
		DIE("No monitors were detected\n");
}

/* Called when the "Affect all monitors" button is pressed.
 * Note that change in vibrance won't happen live after button is checked.
 */
static void checkbtn_toggled(
				GtkCheckButton *check_btn)
{
	user_data.affect_all =
			gtk_check_button_get_active(check_btn) ? (true) : (false);
}

/* Initalize and config the primary @window widgets */
static void do_gtk_widgets_init(GtkWidget *window, GtkBuilder *build)
{
	static struct g_widgets pwidgets;
	GtkWidget *vscale, *checkbtn;
	gint current_vib;

	/* We could also set signals via the UI file itself, but avoid it. */
	pwidgets.pcbox = checkbtn = GTK_WIDGET(gtk_builder_get_object(build, "checkbtn"));
	pwidgets.pvscale = vscale = GTK_WIDGET(gtk_builder_get_object(build, "vscale"));

	get_monitor_digital_vibrance(&current_vib, user_data.default_mon);
	gtk_range_set_value(GTK_RANGE(vscale),
	 		dv_value_to_percentage(current_vib, &gdisplay.monitors[user_data.default_mon - 1]));

	/* Signals */
	g_signal_connect(GTK_RANGE(vscale), "value_changed",
					 	G_CALLBACK(&gtk_scale_val_change_callback),
						&pwidgets);
	g_signal_connect(user_data.monitors, "items-changed",
						G_CALLBACK(gtk_monitor_change_callback), NULL);
	g_signal_connect(checkbtn, "toggled",
					 	G_CALLBACK(checkbtn_toggled), NULL);

	gtk_widget_show(window);
}

static void activate(GtkApplication *app, gpointer data)
{
	GtkWidget *win, *fixed; /* GtkFixed */
	GtkBuilder *build; /* Builder UI */
	GtkCssProvider *cssprov; /* External CSS theme file */
	GtkWidget *checkbtn, *vscale, *credit, *nvi_image;

	const char *credit_text =
		"<a href=\"https://github.com/qodroi\" title=\"&lt;i&gt;Github&lt;/i&gt; Profile\">"
		"Made by Roi</a>";

	user_data.user_display = gdk_display_manager_get_default_display(
								gdk_display_manager_get());
	if (user_data.user_display == NULL)
		DIE("Failed to get current display handle\n");

	build = gtk_builder_new_from_file("gtkui.xml");
	win = GTK_WIDGET(gtk_builder_get_object(build, "win"));
	fixed = GTK_WIDGET(gtk_builder_get_object(build, "fixed"));
	gtk_window_set_application(GTK_WINDOW(win), GTK_APPLICATION(app));

	checkbtn = GTK_WIDGET(gtk_builder_get_object(build, "checkbtn"));
	vscale = GTK_WIDGET(gtk_builder_get_object(build, "vscale"));
	credit = GTK_WIDGET(gtk_builder_get_object(build, "creditlb"));
	nvi_image = gtk_image_new_from_file("assets/nvidia-ico.png");

	/* I don't want to do that. Find a FIX */
	gtk_widget_unparent(checkbtn);
	gtk_widget_unparent(vscale);
	gtk_widget_unparent(credit);

	/* Widget configuration */
	gtk_scale_add_mark(GTK_SCALE(vscale), 50, GTK_POS_TOP,
		 "<span font_size='small' stretch='ultracondensed'>Vibrance Level</span>");
	gtk_label_set_markup(GTK_LABEL(credit), credit_text);
	gtk_image_set_pixel_size(GTK_IMAGE(nvi_image), 170);

	/* Position the elements */
	gtk_fixed_put(GTK_FIXED(fixed), checkbtn, 20, 120);
	gtk_fixed_put(GTK_FIXED(fixed), vscale, 30, 180);
	gtk_fixed_put(GTK_FIXED(fixed), credit, 330, 500);
	gtk_fixed_put(GTK_FIXED(fixed), nvi_image, 230, 10);

	user_data.monitors = gdk_display_get_monitors(user_data.user_display);
	user_data.nm = g_list_model_get_n_items(user_data.monitors);
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
	g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);
	status = g_application_run(G_APPLICATION(app), argc, argv);

   	XCloseDisplay(gdisplay.dpy);
	g_object_unref(app);

	return status;
}