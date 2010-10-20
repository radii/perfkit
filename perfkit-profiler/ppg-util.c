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

#include "ppg-actions.h"
#include "ppg-util.h"

void
ppg_util_load_ui (GtkWidget       *widget,
                  GtkActionGroup **actions,
                  const gchar     *ui_data,
                  const gchar     *first_widget,
                  ...)
{
	GtkUIManager *ui_manager;
	GtkActionGroup *action_group;
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
	action_group = gtk_action_group_new(g_type_name(type));
	ppg_actions_load(widget, action_group);
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
