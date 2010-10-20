/* ppg-visualizer-menu.c
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

#include <glib/gi18n.h>

#include "ppg-instrument.h"
#include "ppg-visualizer.h"
#include "ppg-visualizer-menu.h"

G_DEFINE_TYPE(PpgVisualizerMenu, ppg_visualizer_menu, GTK_TYPE_MENU)

struct _PpgVisualizerMenuPrivate
{
	PpgInstrument *instrument;
};

enum
{
	PROP_0,
	PROP_INSTRUMENT,
};

static void
ppg_visualizer_menu_add_dummy (PpgVisualizerMenu *menu)
{
	GtkWidget *menu_item;

	g_return_if_fail(PPG_IS_VISUALIZER_MENU(menu));

	menu_item = g_object_new(GTK_TYPE_MENU_ITEM,
	                         "label", _("No visualizers available"),
	                         "sensitive", FALSE,
	                         "visible", TRUE,
	                         NULL);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
}

static void
ppg_visualizer_menu_set_instrument (PpgVisualizerMenu *menu,
                                    PpgInstrument     *instrument)
{
	PpgVisualizerMenuPrivate *priv;
	PpgVisualizerEntry *entry;
	GtkWidget *menu_item;
	GList *list;
	GList *iter;

	g_return_if_fail(PPG_IS_VISUALIZER_MENU(menu));
	g_return_if_fail(PPG_IS_INSTRUMENT(instrument));

	priv = menu->priv;

	g_debug("%s():%d", G_STRFUNC, __LINE__);

	/*
	 * Remove all existing entries.
	 */
	list = gtk_container_get_children(GTK_CONTAINER(menu));
	for (iter = list; iter; iter = iter->next) {
		gtk_container_remove(GTK_CONTAINER(menu), iter->data);
	}
	g_list_free(list);

	/*
	 * If there are no entries, add dummy item back.
	 */
	if (!(list = ppg_instrument_get_visualizer_entries(instrument))) {
		ppg_visualizer_menu_add_dummy(menu);
		return;
	}

	/*
	 * Add visualizer entries.
	 */
	for (iter = list; iter; iter = iter->next) {
		entry = iter->data;
		menu_item = g_object_new(GTK_TYPE_CHECK_MENU_ITEM,
		                         "active", TRUE, /* FIXME: Lookup state */
		                         "label", entry->title,
		                         "visible", TRUE,
		                         NULL);
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
		                         
	}
	g_list_free(list);
}

/**
 * ppg_visualizer_menu_finalize:
 * @object: (in): A #PpgVisualizerMenu.
 *
 * Finalizer for a #PpgVisualizerMenu instance.  Frees any resources held by
 * the instance.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
ppg_visualizer_menu_finalize (GObject *object)
{
	G_OBJECT_CLASS(ppg_visualizer_menu_parent_class)->finalize(object);
}

/**
 * ppg_visualizer_menu_set_property:
 * @object: (in): A #GObject.
 * @prop_id: (in): The property identifier.
 * @value: (out): The given property.
 * @pspec: (in): A #ParamSpec.
 *
 * Get a given #GObject property.
 */
static void
ppg_visualizer_menu_get_property (GObject    *object,
                                  guint       prop_id,
                                  GValue     *value,
                                  GParamSpec *pspec)
{
	PpgVisualizerMenu *menu = PPG_VISUALIZER_MENU(object);

	switch (prop_id) {
	case PROP_INSTRUMENT:
		g_value_set_object(value, menu->priv->instrument);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
	}
}

/**
 * ppg_visualizer_menu_set_property:
 * @object: (in): A #GObject.
 * @prop_id: (in): The property identifier.
 * @value: (in): The given property.
 * @pspec: (in): A #ParamSpec.
 *
 * Set a given #GObject property.
 */
static void
ppg_visualizer_menu_set_property (GObject      *object,
                                  guint         prop_id,
                                  const GValue *value,
                                  GParamSpec   *pspec)
{
	PpgVisualizerMenu *menu = PPG_VISUALIZER_MENU(object);

	switch (prop_id) {
	case PROP_INSTRUMENT:
		ppg_visualizer_menu_set_instrument(menu, g_value_get_object(value));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
	}
}

/**
 * ppg_visualizer_menu_class_init:
 * @klass: (in): A #PpgVisualizerMenuClass.
 *
 * Initializes the #PpgVisualizerMenuClass and prepares the vtable.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
ppg_visualizer_menu_class_init (PpgVisualizerMenuClass *klass)
{
	GObjectClass *object_class;

	object_class = G_OBJECT_CLASS(klass);
	object_class->finalize = ppg_visualizer_menu_finalize;
	object_class->get_property = ppg_visualizer_menu_get_property;
	object_class->set_property = ppg_visualizer_menu_set_property;
	g_type_class_add_private(object_class, sizeof(PpgVisualizerMenuPrivate));

	g_object_class_install_property(object_class,
	                                PROP_INSTRUMENT,
	                                g_param_spec_object("instrument",
	                                                    "instrument",
	                                                    "instrument",
	                                                    PPG_TYPE_INSTRUMENT,
	                                                    G_PARAM_READWRITE));
}

/**
 * ppg_visualizer_menu_init:
 * @menu: (in): A #PpgVisualizerMenu.
 *
 * Initializes the newly created #PpgVisualizerMenu instance.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
ppg_visualizer_menu_init (PpgVisualizerMenu *menu)
{
	PpgVisualizerMenuPrivate *priv;

	priv = G_TYPE_INSTANCE_GET_PRIVATE(menu, PPG_TYPE_VISUALIZER_MENU,
	                                   PpgVisualizerMenuPrivate);
	menu->priv = priv;

	ppg_visualizer_menu_add_dummy(menu);
}
