/* ppg-actions.c
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

#include <string.h>

#include "ppg-actions.h"

typedef struct
{
	GType                 widget_type;
	GType                 action_type;
	PpgActionFunc         action_func;
	GtkActionEntry       *action_entries;
	GtkToggleActionEntry *toggle_action_entries;
	guint                 action_n_entries;
} PpgActionFactory;

static GArray *factories = NULL;

void
ppg_actions_init (void)
{
	factories = g_array_new(FALSE, FALSE, sizeof(PpgActionFactory));
}

void
ppg_action_factory_create (PpgActionFactory *factory,
                           GtkWidget        *widget,
                           GtkActionGroup   *action_group)
{
	GtkAction *action;

	g_return_if_fail(factory != NULL);
	g_return_if_fail(factory->action_type ||
	                 factory->action_func ||
	                 factory->action_entries ||
	                 factory->toggle_action_entries);
	g_return_if_fail(GTK_IS_WIDGET(widget));
	g_return_if_fail(GTK_IS_ACTION_GROUP(action_group));

	if (factory->action_type) {
		action = g_object_new(factory->action_type, NULL);
		gtk_action_group_add_action(action_group, action);
		g_object_unref(action);
	} else if (factory->action_func) {
		action = factory->action_func(widget);
		gtk_action_group_add_action(action_group, action);
		g_object_unref(action);
	} else if (factory->action_entries) {
		gtk_action_group_add_actions(action_group,
		                             factory->action_entries,
		                             factory->action_n_entries,
		                             widget);
	} else if (factory->toggle_action_entries) {
		gtk_action_group_add_toggle_actions(action_group,
		                                    factory->toggle_action_entries,
		                                    factory->action_n_entries,
		                                    widget);
	} else {
		g_assert_not_reached();
	}

}

void
ppg_actions_load (GtkWidget      *widget,
                  GtkActionGroup *action_group)
{
	PpgActionFactory *factory;
	GtkAccelGroup *accel_group = NULL;
	GType widget_type;
	GList *list;
	GList *iter;
	gint i;

	g_return_if_fail(GTK_IS_WIDGET(widget));
	g_return_if_fail(GTK_IS_ACTION_GROUP(action_group));
	g_return_if_fail(factories != NULL);

	widget_type = G_TYPE_FROM_INSTANCE(widget);

	if (g_type_is_a(widget_type, GTK_TYPE_WINDOW)) {
		g_debug("Adding accel group to window");
		accel_group = gtk_accel_group_new();
		gtk_window_add_accel_group(GTK_WINDOW(widget), accel_group);
	}

	for (i = 0; i < factories->len; i++) {
		factory = &g_array_index(factories, PpgActionFactory, i);
		ppg_action_factory_create(factory, widget, action_group);
	}

	/*
	 * FIXME: For some reason, accelerators don't seem to be activating.
	 */

	if (accel_group) {
		list = gtk_action_group_list_actions(action_group);
		for (iter = list; iter; iter = iter->next) {
			gtk_action_set_accel_group(iter->data, accel_group);
			if (gtk_accel_map_lookup_entry(gtk_action_get_accel_path(iter->data), NULL)) {
			}
		}
		g_list_free(list);
	}
}

void
ppg_actions_register (GType widget_type,
                      GType action_type)
{
	PpgActionFactory factory;

	g_return_if_fail(factories != NULL);

	memset(&factory, 0, sizeof(factory));
	factory.widget_type = widget_type;
	factory.action_type = action_type;
	g_array_append_val(factories, factory);
}

void
ppg_actions_register_func (GType         widget_type,
                           PpgActionFunc func)
{
	PpgActionFactory factory;

	g_return_if_fail(factories != NULL);

	memset(&factory, 0, sizeof(factory));
	factory.widget_type = widget_type;
	factory.action_func = func;
	g_array_append_val(factories, factory);
}

void
ppg_actions_register_entries (GType           widget_type,
                              GtkActionEntry *entries,
                              guint           n_entries)
{
	PpgActionFactory factory;

	g_return_if_fail(factories != NULL);
	g_return_if_fail(entries != NULL);

	memset(&factory, 0, sizeof(factory));
	factory.widget_type = widget_type;
	factory.action_entries = entries;
	factory.action_n_entries = n_entries;
	g_array_append_val(factories, factory);
}

void
ppg_actions_register_toggle_entries (GType                 widget_type,
                                     GtkToggleActionEntry *entries,
                                     guint                 n_entries)
{
	PpgActionFactory factory;

	g_return_if_fail(factories != NULL);
	g_return_if_fail(entries != NULL);

	memset(&factory, 0, sizeof(factory));
	factory.widget_type = widget_type;
	factory.toggle_action_entries = entries;
	factory.action_n_entries = n_entries;
	g_array_append_val(factories, factory);
}
