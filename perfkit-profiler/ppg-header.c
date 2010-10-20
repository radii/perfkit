/* ppg-header.c
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

#include "ppg-header.h"

G_DEFINE_TYPE(PpgHeader, ppg_header, GTK_TYPE_DRAWING_AREA)

#define TO_CAIRO_RGB(c)    \
    ((c).red   / 65535.0), \
    ((c).green / 65535.0), \
    ((c).blue  / 65535.0)

struct _PpgHeaderPrivate
{
	gboolean right_separator;
	gboolean bottom_separator;
};

enum
{
	PROP_0,
	PROP_BOTTOM_SEPARATOR,
	PROP_RIGHT_SEPARATOR,
};

static gboolean
ppg_header_expose_event (GtkWidget      *widget,
                         GdkEventExpose *expose)
{
	PpgHeader *header = (PpgHeader *)widget;
	PpgHeaderPrivate *priv;
	GtkStateType state = GTK_STATE_NORMAL;
	GtkAllocation alloc;
	GtkStyle *style;
	GdkColor begin;
	GdkColor end;
	GdkColor line;
	GdkColor v_begin;
	GdkColor v_end;
	cairo_pattern_t *p;
	cairo_t *cr;

	g_return_val_if_fail(PPG_IS_HEADER(header), FALSE);

	priv = header->priv;

	gtk_widget_get_allocation(widget, &alloc);
	style = gtk_widget_get_style(widget);
	begin = style->light[state];
	end = style->mid[state];
	line = style->dark[state];
	v_begin = style->mid[state];
	v_end = style->dark[state];

	cr = gdk_cairo_create(expose->window);
	p = cairo_pattern_create_linear(0, 0, 0, alloc.height);

	cairo_pattern_add_color_stop_rgb(p, 0.0, TO_CAIRO_RGB(begin));
	cairo_pattern_add_color_stop_rgb(p, 1.0, TO_CAIRO_RGB(end));
	cairo_set_source(cr, p);
	cairo_rectangle(cr, 0, 0, alloc.width, alloc.height);
	cairo_fill(cr);
	cairo_pattern_destroy(p);


	if (priv->bottom_separator) {
		cairo_set_source_rgb(cr, TO_CAIRO_RGB(line));
		cairo_set_line_width(cr, 1.0);
		cairo_move_to(cr, 0, alloc.height - 0.5);
		cairo_line_to(cr, alloc.width, alloc.height - 0.5);
		cairo_stroke(cr);
	}

	if (priv->right_separator) {
		p = cairo_pattern_create_linear(0, 0, 0, alloc.height);
		cairo_pattern_add_color_stop_rgb(p, 0.0, TO_CAIRO_RGB(v_begin));
		cairo_pattern_add_color_stop_rgb(p, 1.0, TO_CAIRO_RGB(v_end));
		cairo_set_source(cr, p);
		cairo_set_line_width(cr, 1.0);
		cairo_move_to(cr, alloc.width - 0.5, 0);
		cairo_line_to(cr, alloc.width - 0.5, alloc.height);
		cairo_stroke(cr);
		cairo_pattern_destroy(p);
	}

	cairo_destroy(cr);

	return FALSE;
}

/**
 * ppg_header_finalize:
 * @object: (in): A #PpgHeader.
 *
 * Finalizer for a #PpgHeader instance.  Frees any resources held by
 * the instance.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
ppg_header_finalize (GObject *object)
{
	G_OBJECT_CLASS(ppg_header_parent_class)->finalize(object);
}

/**
 * ppg_header_set_property:
 * @object: (in): A #GObject.
 * @prop_id: (in): The property identifier.
 * @value: (out): The given property.
 * @pspec: (in): A #ParamSpec.
 *
 * Get a given #GObject property.
 */
static void
ppg_header_get_property (GObject    *object,
                         guint       prop_id,
                         GValue     *value,
                         GParamSpec *pspec)
{
	PpgHeader *header = PPG_HEADER(object);

	switch (prop_id) {
	case PROP_BOTTOM_SEPARATOR:
		g_value_set_boolean(value, header->priv->right_separator);
		break;
	case PROP_RIGHT_SEPARATOR:
		g_value_set_boolean(value, header->priv->bottom_separator);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
	}
}

/**
 * ppg_header_set_property:
 * @object: (in): A #GObject.
 * @prop_id: (in): The property identifier.
 * @value: (in): The given property.
 * @pspec: (in): A #ParamSpec.
 *
 * Set a given #GObject property.
 */
static void
ppg_header_set_property (GObject      *object,
                         guint         prop_id,
                         const GValue *value,
                         GParamSpec   *pspec)
{
	PpgHeader *header = PPG_HEADER(object);
	GtkWidget *widget = GTK_WIDGET(object);

	switch (prop_id) {
	case PROP_BOTTOM_SEPARATOR:
		header->priv->bottom_separator = g_value_get_boolean(value);
		gtk_widget_queue_draw(widget);
		break;
	case PROP_RIGHT_SEPARATOR:
		header->priv->right_separator = g_value_get_boolean(value);
		gtk_widget_queue_draw(widget);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
	}
}

/**
 * ppg_header_class_init:
 * @klass: (in): A #PpgHeaderClass.
 *
 * Initializes the #PpgHeaderClass and prepares the vtable.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
ppg_header_class_init (PpgHeaderClass *klass)
{
	GObjectClass *object_class;
	GtkWidgetClass *widget_class;

	object_class = G_OBJECT_CLASS(klass);
	object_class->finalize = ppg_header_finalize;
	object_class->get_property = ppg_header_get_property;
	object_class->set_property = ppg_header_set_property;
	g_type_class_add_private(object_class, sizeof(PpgHeaderPrivate));

	widget_class = GTK_WIDGET_CLASS(klass);
	widget_class->expose_event = ppg_header_expose_event;

	g_object_class_install_property(object_class,
	                                PROP_BOTTOM_SEPARATOR,
	                                g_param_spec_boolean("bottom-separator",
	                                                     "bottom-separator",
	                                                     "bottom-separator",
	                                                     FALSE,
	                                                     G_PARAM_READWRITE));

	g_object_class_install_property(object_class,
	                                PROP_RIGHT_SEPARATOR,
	                                g_param_spec_boolean("right-separator",
	                                                     "right-separator",
	                                                     "right-separator",
	                                                     FALSE,
	                                                     G_PARAM_READWRITE));
}

/**
 * ppg_header_init:
 * @header: (in): A #PpgHeader.
 *
 * Initializes the newly created #PpgHeader instance.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
ppg_header_init (PpgHeader *header)
{
	PpgHeaderPrivate *priv;

	priv = header->priv = G_TYPE_INSTANCE_GET_PRIVATE(header, PPG_TYPE_HEADER,
	                                                  PpgHeaderPrivate);
}
