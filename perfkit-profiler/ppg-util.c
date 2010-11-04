/* ppg-util.c
 *
 * Copyright (C) 2010 Christian Hergert <chris@dronelabs.com>
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <sys/utsname.h>

#include "ppg-actions.h"
#include "ppg-util.h"

enum
{
	PALETTE_BASE,
	PALETTE_BG,
	PALETTE_FG,
};

const gchar*
ppg_util_uname (void)
{
	static gsize initialized = FALSE;
	static gchar *uname_str = NULL;
	struct utsname u;
	
	if (g_once_init_enter(&initialized)) {
		uname(&u);
		uname_str = g_strdup_printf("%s", u.release);
		g_once_init_leave(&initialized, TRUE);
	}

	return uname_str;
}

gsize
ppg_util_get_total_memory (void)
{
	return sysconf(_SC_PHYS_PAGES) * getpagesize();
}

void
ppg_util_load_ui (GtkWidget       *widget,
                  GtkActionGroup **actions,
                  const gchar     *ui_data,
                  const gchar     *first_widget,
                  ...)
{
	GtkUIManager *ui_manager;
	GtkActionGroup *action_group;
	GtkAccelGroup *accel_group;
	const gchar *name;
	GtkWidget **dst_widget;
	GError *error = NULL;
	GType type;
	va_list args;

	g_return_if_fail(GTK_IS_WIDGET(widget));
	g_return_if_fail(ui_data != NULL);
	g_return_if_fail(first_widget != NULL);

	ui_manager = gtk_ui_manager_new();
	if (!gtk_ui_manager_add_ui_from_string(ui_manager, ui_data, -1, &error)) {
		g_error("%s", error->message); /* Fatal */
	}

	type = G_TYPE_FROM_INSTANCE(widget);
	accel_group = gtk_ui_manager_get_accel_group(ui_manager);
	action_group = gtk_action_group_new(g_type_name(type));
	ppg_actions_load(widget, accel_group, action_group);
	gtk_ui_manager_insert_action_group(ui_manager, action_group, 0);

	name = first_widget;
	va_start(args, first_widget);

	do {
		dst_widget = va_arg(args, GtkWidget**);
		*dst_widget = gtk_ui_manager_get_widget(ui_manager, name);
	} while ((name = va_arg(args, const gchar*)));

	if (actions) {
		*actions = action_group;
	}
}

static gboolean
ppg_util_real_expose_event (GtkWidget      *widget,
                            GdkEventExpose *event,
                            gint            palette)
{
	GtkAllocation alloc;
	GtkStateType state;
	GtkStyle *style;
	GdkGC *gc = NULL;

	if (GTK_WIDGET_DRAWABLE(widget)) {
		gtk_widget_get_allocation(widget, &alloc);
		style = gtk_widget_get_style(widget);
		state = gtk_widget_get_state(widget);

		switch (palette) {
		case PALETTE_BASE:
			gc = style->base_gc[state];
			break;
		case PALETTE_BG:
			gc = style->bg_gc[state];
			break;
		case PALETTE_FG:
			gc = style->fg_gc[state];
			break;
		default:
			g_assert_not_reached();
		}

		gdk_draw_rectangle(event->window, gc, TRUE, alloc.x, alloc.y,
		                   alloc.width, alloc.height);
	}

	return FALSE;
}

gboolean
ppg_util_base_expose_event (GtkWidget      *widget,
                            GdkEventExpose *event)
{
	return ppg_util_real_expose_event(widget, event, PALETTE_BASE);
}

gboolean
ppg_util_bg_expose_event (GtkWidget      *widget,
                          GdkEventExpose *event)
{
	return ppg_util_real_expose_event(widget, event, PALETTE_BG);
}

gboolean
ppg_util_fg_expose_event (GtkWidget      *widget,
                          GdkEventExpose *event)
{
	return ppg_util_real_expose_event(widget, event, PALETTE_FG);
}

static void
ppg_util_header_item_on_style_set (GtkWidget *widget,
                                   GtkStyle  *old_style,
                                   GtkWidget *child)
{
	GtkStyle *style;

	style = gtk_widget_get_style(widget);
	gtk_widget_set_style(child, style);
}

GtkWidget*
ppg_util_header_item_new (const gchar *label)
{
	GtkWidget *item;
	GtkLabel *child;

	child = g_object_new(GTK_TYPE_LABEL,
	                     "label", label,
	                     "visible", TRUE,
	                     "xalign", 0.0f,
	                     "yalign", 0.5f,
	                     NULL);

	item = g_object_new(GTK_TYPE_MENU_ITEM,
	                    "child", child,
	                    NULL);

	g_signal_connect(item, "style-set",
	                 G_CALLBACK(ppg_util_header_item_on_style_set),
	                 child);

	gtk_menu_item_select(GTK_MENU_ITEM(item));

	return item;
}
