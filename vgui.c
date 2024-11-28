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

#include "vibrancelui.h"
#include "vhook.h"

/* Used as a "package" for @g_signal_connect for
	the passed fuction's data. */
struct g_signal_widgets {
	union {
		GtkWidget *pvscale;
		GtkWidget *pcbox;
	};
} pwidgets;

user_data_t user_data = { NULL, NULL, 0, false, 0 };

struct g_app_config {
	int vibrance_level; /* Vibrance level to use on this app. */
	const char *spid; /* Process ID of app */
};

GHashTable *g_hash_table_apps;
GThread *gthread_p;

/* FIXME: 
	poll_for_event: Assertion 
		`dpy->xcb->event_owner == XlibOwnsEventQueue &&
			!dpy->xcb->event_waiter' failed
	
	Makes the program abort. Figure out the issue.
 */
static void vibrance_scale_callback()
{
	int integer_vibrance_scale;
	gdouble arg_range_val;
	int monitor_number = user_data.dropd_def_mon; /* specified monitor number */

	integer_vibrance_scale = dv_percentage_to_value(
								gtk_range_get_value(GTK_RANGE(pwidgets.pvscale)),
								&gdisplay.monitors_conf[monitor_number]);
	arg_range_val = gtk_range_get_value(GTK_RANGE(pwidgets.pvscale));
	set_monitor_vibrance(monitor_number, integer_vibrance_scale, user_data.affect_all);

	DEBUG_PRINTF("%g %d %d\n", arg_range_val, integer_vibrance_scale, user_data.affect_all);
}

/* Called when the "Affect all" button is pressed.
 * Note that monitors vibrance won't change live after button is checked.
 */
static void __always_inline affect_all_callback(GtkCheckButton *check_btn)
{
	user_data.affect_all =
			gtk_check_button_get_active(check_btn) ? (true) : (false);
}

/* This function is called when the selects a monitor to affect;
	It fetches the vibrance level of this mon and updates the scale accordingly */
static void dropdown_selected_callback(GtkDropDown *self)
{
	int dv;

	user_data.dropd_def_mon = gtk_drop_down_get_selected(self);
	get_monitor_vibrance(&dv, gdisplay.monitors_conf[user_data.dropd_def_mon].dpyId);
	dv = dv_value_to_percentage(dv, &gdisplay.monitors_conf[user_data.dropd_def_mon]);
	gtk_range_set_value(GTK_RANGE(pwidgets.pvscale), dv);
}

static void __always_inline g_hash_table_printall(void *key, void *value, void *userdata)
{
	DEBUG_PRINTF("%s %d\n", (char *)key, *(int *)value);
}

static void __always_inline g_app_config_struct_free(void *g_acfg)
{
	g_free(g_acfg); /* typeof struct g_app_config */
}

static struct g_app_config *g_hash_app_config_struct(const char *spid)
{
	struct g_app_config *p_config_app;
	
	p_config_app =  g_malloc(sizeof(*p_config_app));
	if (p_config_app == NULL)
		return NULL;
	
	p_config_app->spid = spid;
	p_config_app->vibrance_level = DEFAULT_MAX_VIBRANCE_LEVEL; /* TBD */

	return p_config_app;

}

static void pid_entry_submit_callback(GtkEntry *self)
{
	const char *spid;

	/* Get the user supplied process ID */
	spid = gtk_editable_get_text(GTK_EDITABLE(self));
	if (*spid == 0)
		return;

	/* In case of a duplicate, this function will remove the already existing spid
		It does so to add the option to delete entries in case the user wants to.
	*/
	if (g_hash_table_contains(g_hash_table_apps, spid))
		g_hash_table_remove(g_hash_table_apps, spid);
	else
		g_hash_table_insert(g_hash_table_apps, g_strdup(spid), g_hash_app_config_struct(spid));
	
	g_hash_table_foreach(g_hash_table_apps, g_hash_table_printall, NULL);
}

/* Accept only numbers as proper input */
static __always_inline void pid_entry_insert_callback(GtkEditable *self,
		 const gchar *text, gint length, gint *position, gpointer data)
{
	if (!(text[0] >= '0' && text[0] <= '9' ))
		gtk_editable_delete_text(self, 0, -1);
}

static void initalize_gtk_signals(GtkWidget *vscale, GtkWidget *checkbtn,
			GtkWidget *dropdown_display, GtkWidget *process_id_entry)
{
	g_signal_connect(GTK_RANGE(vscale), "value_changed",
					 	G_CALLBACK(&vibrance_scale_callback),
						&pwidgets);
	g_signal_connect(checkbtn, "toggled",
					 	G_CALLBACK(affect_all_callback), NULL);
	g_signal_connect_after(dropdown_display, "notify::selected",
				 G_CALLBACK(dropdown_selected_callback), NULL);
	g_signal_connect(process_id_entry, "activate",
				G_CALLBACK(pid_entry_submit_callback), NULL);
	g_signal_connect_after(gtk_editable_get_delegate(GTK_EDITABLE(process_id_entry)),
		"insert-text", G_CALLBACK(pid_entry_insert_callback), NULL);
}

/* Initalize and config the primary @window widgets */
static void do_gtk_widgets_init_values(GtkWidget *window, GtkBuilder *build,
			GtkWidget *vscale, GtkWidget *checkbtn,
			GtkWidget *credit, GtkWidget *nvi_image)
{
	const char *credit_text =
		"<a href=\"https://github.com/qodroi\" title=\"&lt;i&gt;Github&lt;/i&gt; Profile\">"
		"Made by Roi</a>";
	gint current_vib;

	pwidgets.pcbox = checkbtn;
	pwidgets.pvscale = vscale;

	get_monitor_vibrance(&current_vib,
			gdisplay.monitors_conf[user_data.dropd_def_mon].dpyId);
	gtk_range_set_value(GTK_RANGE(pwidgets.pvscale),
	 		dv_value_to_percentage(current_vib,
			&gdisplay.monitors_conf[user_data.dropd_def_mon]));

	gtk_scale_add_mark(GTK_SCALE(vscale), 50, GTK_POS_TOP,
		 "<span font_size='small' stretch='ultracondensed'>Vibrance Level</span>");
	gtk_label_set_markup(GTK_LABEL(credit), credit_text);
	gtk_image_set_pixel_size(GTK_IMAGE(nvi_image), 170);

	gthread_p = g_thread_new("vib_app_hook_thread", vib_app_hook_thread_start, NULL);
	gtk_widget_show(window);
}

static void gtk_activate(GtkApplication *app, gpointer data)
{
	int mon;
	GtkWidget *win, *fixed; /* GtkFixed */
	GtkBuilder *build; /* Builder UI */
	GtkCssProvider *cssprov; /* External CSS theme file */
	GtkWidget *checkbtn, *vscale, *credit,
				 *nvi_image, *dropdown_display, *process_id_entry;
	const char *str_arr_displays[MAXIMUM_MONITOR_ARRAY_COUNT];

	user_data.user_gdk_display = gdk_display_manager_get_default_display(
								gdk_display_manager_get());
	if (user_data.user_gdk_display == NULL)
		DIE("Failed to get current display handle\n");

	/* Retrieve GListModel object of GdkDisplay and fetch monitor count */
	user_data.glist_monitors = gdk_display_get_monitors(user_data.user_gdk_display);
	user_data.nm = g_list_model_get_n_items(user_data.glist_monitors);

	for (mon = 0; mon < user_data.nm; mon++) {
		str_arr_displays[mon] =
			gdk_monitor_get_model(g_list_model_get_item(user_data.glist_monitors, mon)); 
	}

	g_hash_table_apps = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, g_app_config_struct_free);

	/* load GtkWidget objects created by the external XML file */
	build = gtk_builder_new_from_file("gtkui.xml");
	win = GTK_WIDGET(gtk_builder_get_object(build, "win"));
	fixed = GTK_WIDGET(gtk_builder_get_object(build, "fixed"));	
	process_id_entry = GTK_WIDGET(gtk_builder_get_object(build, "process_list"));
	checkbtn = GTK_WIDGET(gtk_builder_get_object(build, "checkbtn"));
	vscale = GTK_WIDGET(gtk_builder_get_object(build, "vscale"));
	credit = GTK_WIDGET(gtk_builder_get_object(build, "creditlb"));

	/* Create DropDown displayed object and load the Nvidia logo */
	dropdown_display = gtk_drop_down_new_from_strings(str_arr_displays);
	nvi_image = gtk_image_new_from_file("assets/nvidia-ico.png");

	initalize_gtk_signals(vscale, checkbtn, dropdown_display, process_id_entry);

	/* FIXME: Avoid this */
	gtk_widget_unparent(checkbtn);
	gtk_widget_unparent(vscale);
	gtk_widget_unparent(credit);
	gtk_widget_unparent(process_id_entry);

	/* Position the elements */
	gtk_fixed_put(GTK_FIXED(fixed), checkbtn, 20, 120);
	gtk_fixed_put(GTK_FIXED(fixed), vscale, 20, 180);
	gtk_fixed_put(GTK_FIXED(fixed), credit, 325, 500);
	gtk_fixed_put(GTK_FIXED(fixed), nvi_image, 230, 10);
	gtk_fixed_put(GTK_FIXED(fixed), dropdown_display, 20, 80);
	gtk_fixed_put(GTK_FIXED(fixed), process_id_entry, 20, 280);
	
	cssprov = gtk_css_provider_new();

	/* Load external CSS theme file */
	gtk_css_provider_load_from_file(cssprov, g_file_new_for_path("css/gtk.css"));
	gtk_style_context_add_provider_for_display(gtk_widget_get_display(GTK_WIDGET(win)),
			GTK_STYLE_PROVIDER(cssprov), GTK_STYLE_PROVIDER_PRIORITY_USER);

	gtk_window_set_application(GTK_WINDOW(win), GTK_APPLICATION(app));
	gtk_window_present(GTK_WINDOW(win));
	do_gtk_widgets_init_values(win, build, vscale, checkbtn, credit, nvi_image);
}

int do_init_gtk_window(
		int argc, char **argv)
{
	GtkApplication *app;
	int status;

	app = gtk_application_new("com.github.qodroi.vibrancelui", 0);
	g_signal_connect(app, "activate", G_CALLBACK(gtk_activate), NULL);
	status = g_application_run(G_APPLICATION(app), argc, argv);

	/* Not reached until application is closed */
	pthread_spin_lock(&gdisplay.lock);
	XCloseDisplay(gdisplay.dpy);
	gdisplay.dpy = NULL;
	g_hash_table_destroy(g_hash_table_apps);
	g_thread_unref(gthread_p);
	g_object_unref(app);
	free(gdisplay.monitors_conf);
	pthread_spin_unlock(&gdisplay.lock);

	return status;
}
