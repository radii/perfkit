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


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "ppg-actions.h"
#include "ppg-window.h"


typedef struct
{
	GType   widget_type;
	GArray *action_types;
} ActionMap;


static GHashTable *actions_map = NULL;


/**
 * action_map_new:
 * @widget_type: The #GType of a particular #GtkWidget.
 *
 * Creates a new ActionMap to contain #GType<!-- -->'s for
 * series of #GtkAction<!-- -->'s.
 *
 * Returns: An ActionMap instance.
 * Side effects: None.
 */
static ActionMap*
action_map_new (GType widget_type)
{
	ActionMap *map;

	g_return_val_if_fail(g_type_is_a(widget_type, GTK_TYPE_WIDGET), NULL);

	map = g_new0(ActionMap, 1);
	map->widget_type = widget_type;
	map->action_types = g_array_new(FALSE, FALSE, sizeof(GType));
	return map;
}


/**
 * action_map_free:
 * @map: An ActionMap.
 *
 * Releases resources allocated for an ActionMap.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
action_map_free (gpointer ptr)
{
	ActionMap *map = ptr;

	g_return_if_fail(map != NULL);

	g_array_unref(map->action_types);
	g_free(map);
}


/**
 * ppg_actions_init:
 *
 * Initializes the actions subsystem.
 *
 * Returns: None.
 * Side effects: Default actions are registered.
 */
void
ppg_actions_init (void)
{
	static gsize initialized = FALSE;

	if (g_once_init_enter(&initialized)) {
		actions_map = g_hash_table_new_full(g_int_hash, g_int_equal,
		                                    NULL, action_map_free);
		g_once_init_leave(&initialized, TRUE);
	}
}


/**
 * ppg_actions_load:
 * @widget: (in): A #GtkWidget.
 * @action_group: (in): A #GtkActionGroup.
 *
 * Creates new instances of all the #GtkAction<!-- -->'s registered for
 * the #GtkWidget<!-- -->'s GType and parent #GType<!-- -->'s.
 *
 * Returns: None.
 * Side effects: None.
 */
void
ppg_actions_load (GtkWidget      *widget,
                  GtkActionGroup *action_group)
{
	GHashTableIter iter;
	GObjectClass *klass;
	GParamSpec *pspec;
	GtkAction *action;
	ActionMap *val;
	GType *key;
	GType type;
	GType action_type;
	gint i;

	g_return_if_fail(GTK_IS_WIDGET(widget));
	g_return_if_fail(GTK_IS_ACTION_GROUP(action_group));

	if (!actions_map) {
		ppg_actions_init();
	}

	type = G_TYPE_FROM_INSTANCE(widget);
	g_hash_table_iter_init(&iter, actions_map);
	while (g_hash_table_iter_next(&iter, (gpointer *)&key, (gpointer *)&val)) {
		if (g_type_is_a(type, *key)) {
			for (i = 0; i < val->action_types->len; i++) {
				action_type = g_array_index(val->action_types, GType, i);
				action = g_object_new(action_type, NULL);
				gtk_action_group_add_action(action_group, action);
				klass = G_OBJECT_GET_CLASS(action);
				if ((pspec = g_object_class_find_property(klass, "widget"))) {
					if (g_type_is_a(G_TYPE_FROM_INSTANCE(widget),
					                pspec->value_type)) {
						if (pspec->flags & G_PARAM_WRITABLE) {
							g_object_set(action, "widget", widget, NULL);
						}
					}
				}
			}
		}
	}
}


/**
 * ppg_actions_load_from_ui_string:
 * @widget: (in): A #GtkWidget subclass.
 * @action_group: (in): A #GtkActionGroup or %NULL.
 * @data: (in): The string contents to load.
 * @data_len: (in): The length of @data or -1 for %NULL terminated string.
 * @first_path: (in): The path of an object to abstract followed by a location
 *   to store it.  Terminate with %NULL.
 *
 * Loads the GtkUIManager data from @data and adds the appropriate actions
 * that were registered for @widget<!-- -->'s #GType.  You can extract
 * widgets from the UI file using a 2-part tuple to define (name, location).
 *
 * [[
 * GtkWidget *menubar;
 * ppg_actions_load_from_ui_string(widget, NULL, ui_str, -1,
 *                                 "/Menubar", &menubar, NULL);
 * ]]
 *
 *
 * Returns: None.
 * Side effects: None.
 */
void
ppg_actions_load_from_ui_string (GtkWidget      *widget,
                                 GtkActionGroup *action_group,
                                 const gchar    *data,
                                 gssize          data_len,
                                 const gchar    *first_path,
                                 ...)
{
	GtkUIManager *ui;
	const gchar *path;
	GtkWidget **ptr;
	va_list args;

	g_return_if_fail(GTK_IS_WIDGET(widget));
	g_return_if_fail(data != NULL);
	g_return_if_fail(data_len >= -1);

	if (!action_group) {
		action_group = gtk_action_group_new(NULL);
	}

	ppg_actions_load(widget, action_group);
	ui = g_object_new(GTK_TYPE_UI_MANAGER, NULL);
	gtk_ui_manager_insert_action_group(ui, action_group, 0);
	gtk_ui_manager_add_ui_from_string(ui, data, data_len, NULL);

	path = first_path;
	va_start(args, first_path);
	while (path) {
		ptr = va_arg(args, GtkWidget**);
		*ptr = gtk_ui_manager_get_widget(ui, path);
		path = va_arg(args, const gchar*);
	}
	va_end(args);
}


/**
 * ppg_actions_register:
 * @widget_type: (in): A #GType of a #GtkWidget subclass.
 * @action_type: (in): A #GType of a #GtkAction subclass.
 *
 * Registers @action_type to be instantiated and added to instances
 * of @widget_type when ppg_actions_load() is called.
 *
 * Returns: None.
 * Side effects: None.
 */
void
ppg_actions_register (GType widget_type,
                      GType action_type)
{
	ActionMap *map;

	g_return_if_fail(g_type_is_a(widget_type, GTK_TYPE_WIDGET));
	g_return_if_fail(g_type_is_a(action_type, GTK_TYPE_ACTION));

	if (!actions_map) {
		ppg_actions_init();
	}

	if (!(map = g_hash_table_lookup(actions_map, &widget_type))) {
		map = action_map_new(widget_type);
		g_hash_table_insert(actions_map, &map->widget_type, map);
	}

	g_array_append_val(map->action_types, action_type);
}


/**
 * ppg_actions_unregister:
 * @widget_type: (in): A #GType of a #GtkWidget subclass.
 * @action_type: (in): A #GType of a #GtkAction subclass.
 *
 * Unregisters a previously registered mapping.
 *
 * Returns: None.
 * Side effects: None.
 */
void
ppg_actions_unregister (GType widget_type,
                        GType action_type)
{
	ActionMap *map;
	GType type;
	gint i;

	if (!actions_map) {
		ppg_actions_init();
	}

	if ((map = g_hash_table_lookup(actions_map, &widget_type))) {
		for (i = 0; i < map->action_types->len; i++) {
			type = g_array_index(map->action_types, GType, i);
			if (type == action_type) {
				g_array_remove_index(map->action_types, i);
				break;
			}
		}
	}
}
