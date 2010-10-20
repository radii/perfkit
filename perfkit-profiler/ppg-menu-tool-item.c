/* ppg-menu-tool-item.c
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

#include "ppg-menu-tool-item.h"

G_DEFINE_TYPE(PpgMenuToolItem, ppg_menu_tool_item, GTK_TYPE_TOOL_ITEM)

struct _PpgMenuToolItemPrivate
{
	GtkWidget *button;
	GtkWidget *label;
	GtkWidget *menu;
	guint      menu_deactivate;
};

enum
{
	PROP_0,
	PROP_LABEL,
	PROP_MENU,
};

static void
ppg_menu_tool_item_menu_position_func (GtkMenu  *menu,
                                       gint     *x,
                                       gint     *y,
                                       gboolean *push_in,
                                       gpointer  user_data)
{
	PpgMenuToolItemPrivate *priv;
	PpgMenuToolItem *item = (PpgMenuToolItem *)user_data;
	GtkAllocation alloc;

	g_return_if_fail(PPG_IS_MENU_TOOL_ITEM(item));

	priv = item->priv;

	gtk_widget_get_allocation(priv->button, &alloc);
	gdk_window_get_root_coords(gtk_widget_get_window(priv->button),
	                           alloc.x, alloc.y + alloc.height, x, y);
	*push_in = TRUE;
}

static void
ppg_menu_tool_item_button_clicked (GtkWidget       *button,
                                   PpgMenuToolItem *item)
{
	PpgMenuToolItemPrivate *priv;
	GtkWindow *window;

	g_return_if_fail(PPG_IS_MENU_TOOL_ITEM(item));
	g_return_if_fail(GTK_IS_BUTTON(button));

	priv = item->priv;

	if (priv->menu) {
		gtk_menu_popup(GTK_MENU(priv->menu), NULL, NULL,
		               ppg_menu_tool_item_menu_position_func,
		               item, 1, gtk_get_current_event_time());
	} else {
		/*
		 * Don't allow the button to get focus.
		 */
		window = GTK_WINDOW(gtk_widget_get_toplevel(button));
		gtk_window_set_focus(window, NULL);
	}
}

static void
ppg_menu_tool_item_menu_deactivate (GtkMenu         *menu,
                                    PpgMenuToolItem *item)
{
	PpgMenuToolItemPrivate *priv;
	GtkWindow *window;

	g_return_if_fail(GTK_IS_MENU(menu));
	g_return_if_fail(PPG_IS_MENU_TOOL_ITEM(item));

	priv = item->priv;

	/*
	 * Don't allow the button to get focus.
	 */
	window = GTK_WINDOW(gtk_widget_get_toplevel(priv->button));
	gtk_window_set_focus(window, NULL);
}

static void
ppg_menu_tool_item_set_menu (PpgMenuToolItem *item,
                             GtkMenu         *menu)
{
	PpgMenuToolItemPrivate *priv;

	g_return_if_fail(PPG_IS_MENU_TOOL_ITEM(item));
	g_return_if_fail(!menu || GTK_IS_MENU(menu));

	priv = item->priv;

	if (priv->menu) {
		g_signal_handler_disconnect(priv->menu, priv->menu_deactivate);
		priv->menu = NULL;
		priv->menu_deactivate = 0;
	}

	if (menu) {
		priv->menu = GTK_WIDGET(menu);
		priv->menu_deactivate =
			g_signal_connect(menu, "deactivate",
			                 G_CALLBACK(ppg_menu_tool_item_menu_deactivate),
			                 item);
	}
}

static void
ppg_menu_tool_item_set_label (PpgMenuToolItem *item,
                              const gchar     *label)
{
	PpgMenuToolItemPrivate *priv;

	g_return_if_fail(PPG_IS_MENU_TOOL_ITEM(item));

	priv = item->priv;
	g_object_set(priv->label, "label", label, NULL);
}

/**
 * ppg_menu_tool_item_finalize:
 * @object: (in): A #PpgMenuToolItem.
 *
 * Finalizer for a #PpgMenuToolItem instance.  Frees any resources held by
 * the instance.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
ppg_menu_tool_item_finalize (GObject *object)
{
	G_OBJECT_CLASS(ppg_menu_tool_item_parent_class)->finalize(object);
}

/**
 * ppg_menu_tool_item_set_property:
 * @object: (in): A #GObject.
 * @prop_id: (in): The property identifier.
 * @value: (in): The given property.
 * @pspec: (in): A #ParamSpec.
 *
 * Set a given #GObject property.
 */
static void
ppg_menu_tool_item_set_property (GObject      *object,
                                 guint         prop_id,
                                 const GValue *value,
                                 GParamSpec   *pspec)
{
	PpgMenuToolItem *item = PPG_MENU_TOOL_ITEM(object);

	switch (prop_id) {
	case PROP_MENU:
		ppg_menu_tool_item_set_menu(item, g_value_get_object(value));
		break;
	case PROP_LABEL:
		ppg_menu_tool_item_set_label(item, g_value_get_string(value));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
	}
}

/**
 * ppg_menu_tool_item_class_init:
 * @klass: (in): A #PpgMenuToolItemClass.
 *
 * Initializes the #PpgMenuToolItemClass and prepares the vtable.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
ppg_menu_tool_item_class_init (PpgMenuToolItemClass *klass)
{
	GObjectClass *object_class;

	object_class = G_OBJECT_CLASS(klass);
	object_class->finalize = ppg_menu_tool_item_finalize;
	object_class->set_property = ppg_menu_tool_item_set_property;
	g_type_class_add_private(object_class, sizeof(PpgMenuToolItemPrivate));

	g_object_class_install_property(object_class,
	                                PROP_LABEL,
	                                g_param_spec_string("label",
	                                                    "label",
	                                                    "label",
	                                                    NULL,
	                                                    G_PARAM_WRITABLE));

	g_object_class_install_property(object_class,
	                                PROP_MENU,
	                                g_param_spec_object("menu",
	                                                    "menu",
	                                                    "menu",
	                                                    GTK_TYPE_MENU,
	                                                    G_PARAM_WRITABLE));
}

/**
 * ppg_menu_tool_item_init:
 * @item: (in): A #PpgMenuToolItem.
 *
 * Initializes the newly created #PpgMenuToolItem instance.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
ppg_menu_tool_item_init (PpgMenuToolItem *item)
{
	PpgMenuToolItemPrivate *priv;
	GtkWidget *hbox;
	GtkWidget *arrow;
	GtkWidget *align;

	priv = G_TYPE_INSTANCE_GET_PRIVATE(item, PPG_TYPE_MENU_TOOL_ITEM,
	                                   PpgMenuToolItemPrivate);
	item->priv = priv;

	align = g_object_new(GTK_TYPE_ALIGNMENT,
	                     "visible", TRUE,
	                     "xalign", 0.5f,
	                     "xscale", 1.0f,
	                     "yalign", 0.5f,
	                     "yscale", 0.0f,
	                     NULL);
	gtk_container_add(GTK_CONTAINER(item), align);

	priv->button = g_object_new(GTK_TYPE_BUTTON,
	                            "visible", TRUE,
	                            NULL);
	gtk_container_add(GTK_CONTAINER(align), priv->button);
	g_signal_connect(priv->button, "clicked",
	                 G_CALLBACK(ppg_menu_tool_item_button_clicked),
	                 item);

	hbox = g_object_new(GTK_TYPE_HBOX,
	                    "spacing", 3,
	                    "visible", TRUE,
	                    NULL);
	gtk_container_add(GTK_CONTAINER(priv->button), hbox);

	priv->label = g_object_new(GTK_TYPE_LABEL,
	                           "ellipsize", PANGO_ELLIPSIZE_MIDDLE,
	                           "single-line-mode", TRUE,
	                           "visible", TRUE,
	                           "xalign", 0.0f,
	                           NULL);
	gtk_container_add(GTK_CONTAINER(hbox), priv->label);

	arrow = g_object_new(GTK_TYPE_ARROW,
	                     "arrow-type", GTK_ARROW_DOWN,
	                     "visible", TRUE,
	                     NULL);
	gtk_container_add_with_properties(GTK_CONTAINER(hbox), arrow,
	                                  "expand", FALSE,
	                                  NULL);
}
