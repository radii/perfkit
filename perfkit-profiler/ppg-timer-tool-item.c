/* ppg-timer-tool-item.c
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

#include "ppg-timer-tool-item.h"

G_DEFINE_TYPE(PpgTimerToolItem, ppg_timer_tool_item, GTK_TYPE_TOOL_ITEM)

struct _PpgTimerToolItemPrivate
{
	GtkWidget *button;
	GtkWidget *ebox;
	GtkWidget *label;
};

static gboolean
ppg_timer_tool_item_ebox_expose (GtkWidget        *ebox,
                                 GdkEventExpose   *expose,
                                 PpgTimerToolItem *item)
{
	PpgTimerToolItemPrivate *priv;
	GtkAllocation alloc;
	GtkStyle *style;

	g_return_val_if_fail(GTK_IS_WIDGET(ebox), FALSE);
	g_return_val_if_fail(expose != NULL, FALSE);
	g_return_val_if_fail(PPG_IS_TIMER_TOOL_ITEM(item), FALSE);

	priv = item->priv;

	/*
	 * XXX: Hackety-hack (don't talk back).
	 */
	gtk_widget_show(priv->button);
	style = gtk_widget_get_style(priv->button);
	gtk_widget_hide(priv->button);

	gtk_widget_get_allocation(ebox, &alloc);
	gtk_paint_box(style,
	              expose->window,
	              GTK_STATE_NORMAL,
	              GTK_SHADOW_OUT,
	              &expose->area,
	              priv->button,
	              "button",
	              0,
	              0,
	              alloc.width,
	              alloc.height);

	return FALSE;
}

/**
 * ppg_timer_tool_item_finalize:
 * @object: (in): A #PpgTimerToolItem.
 *
 * Finalizer for a #PpgTimerToolItem instance.  Frees any resources held by
 * the instance.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
ppg_timer_tool_item_finalize (GObject *object)
{
	G_OBJECT_CLASS(ppg_timer_tool_item_parent_class)->finalize(object);
}

/**
 * ppg_timer_tool_item_class_init:
 * @klass: (in): A #PpgTimerToolItemClass.
 *
 * Initializes the #PpgTimerToolItemClass and prepares the vtable.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
ppg_timer_tool_item_class_init (PpgTimerToolItemClass *klass)
{
	GObjectClass *object_class;

	object_class = G_OBJECT_CLASS(klass);
	object_class->finalize = ppg_timer_tool_item_finalize;
	g_type_class_add_private(object_class, sizeof(PpgTimerToolItemPrivate));
}

/**
 * ppg_timer_tool_item_init:
 * @item: (in): A #PpgTimerToolItem.
 *
 * Initializes the newly created #PpgTimerToolItem instance.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
ppg_timer_tool_item_init (PpgTimerToolItem *item)
{
	PpgTimerToolItemPrivate *priv;
	GtkWidget *align;
	GtkWidget *hbox;
	PangoAttrList *attrs;

	priv = G_TYPE_INSTANCE_GET_PRIVATE(item, PPG_TYPE_TIMER_TOOL_ITEM,
	                                   PpgTimerToolItemPrivate);
	item->priv = priv;

	align = g_object_new(GTK_TYPE_ALIGNMENT,
	                     "xalign", 0.5f,
	                     "xscale", 0.0f,
	                     "yalign", 0.5f,
	                     "yscale", 0.0f,
	                     "visible", TRUE,
	                     NULL);
	gtk_container_add(GTK_CONTAINER(item), align);

	hbox = g_object_new(GTK_TYPE_HBOX,
	                    "visible", TRUE,
	                    NULL);
	gtk_container_add(GTK_CONTAINER(align), hbox);

	priv->ebox = g_object_new(GTK_TYPE_EVENT_BOX,
	                          "app-paintable", TRUE,
	                          "visible", TRUE,
	                          NULL);
	gtk_container_add(GTK_CONTAINER(hbox), priv->ebox);
	g_signal_connect(priv->ebox, "expose-event",
	                 G_CALLBACK(ppg_timer_tool_item_ebox_expose), item);

	/* dummy button for styling */
	priv->button = g_object_new(GTK_TYPE_BUTTON, NULL);
	gtk_container_add(GTK_CONTAINER(hbox), priv->button);

	attrs = pango_attr_list_new();
	pango_attr_list_insert(attrs, pango_attr_size_new(16 * PANGO_SCALE));
	pango_attr_list_insert(attrs, pango_attr_family_new("Monospace"));
	pango_attr_list_insert(attrs, pango_attr_weight_new(PANGO_WEIGHT_BOLD));
	priv->label = g_object_new(GTK_TYPE_LABEL,
	                           "attributes", attrs,
	                           "label", "00:00:00.00",
	                           "selectable", TRUE,
	                           //"single-line-mode", TRUE,
	                           "use-markup", FALSE,
	                           "visible", TRUE,
	                           "xalign", 0.5f,
	                           "yalign", 0.5f,
	                           "xpad", 12,
	                           "ypad", 6,
	                           NULL);
	gtk_container_add(GTK_CONTAINER(priv->ebox), priv->label);
	pango_attr_list_unref(attrs);
}
