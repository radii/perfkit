/* ppg-ruler.c
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

#include <math.h>

#include "ppg-ruler.h"

#define ARROW_SIZE 17

G_DEFINE_TYPE(PpgRuler, ppg_ruler, PPG_TYPE_HEADER)

struct _PpgRulerPrivate
{
	PangoFontDescription *font_desc;

	gdouble lower;
	gdouble upper;
	gdouble pos;

	gboolean dirty;

	GdkPixmap *ruler;
	GdkPixmap *arrow;
};

enum
{
	PROP_0,
	PROP_LOWER,
	PROP_UPPER,
	PROP_POSITION,
};

static inline gboolean
ppg_ruler_contains (PpgRuler *ruler,
                    gdouble   value)
{
	PpgRulerPrivate *priv = ruler->priv;
	return ((value >= priv->lower) && (value <= priv->upper));
}

static inline void
ppg_ruler_update_layout_text (PpgRuler *ruler,
                              PangoLayout *layout,
                              gdouble t)
{
	gchar *markup;

	markup = g_strdup_printf("%02d:%02d:%02d",
	                         (gint)(t / 3600.0),
	                         (gint)(((gint)t % 3600) / 60.0),
	                         (gint)((gint)t % 60));
	pango_layout_set_text(layout, markup, -1);
}

void
ppg_ruler_set_range (PpgRuler *ruler,
                     gdouble   lower,
                     gdouble   upper,
                     gdouble   position)
{
	PpgRulerPrivate *priv;

	g_return_if_fail(PPG_IS_RULER(ruler));

	priv = ruler->priv;

	priv->lower = lower;
	priv->upper = upper;
	priv->pos = position;
	priv->dirty = TRUE;

	gtk_widget_queue_draw(GTK_WIDGET(ruler));

	g_object_notify(G_OBJECT(ruler), "lower");
	g_object_notify(G_OBJECT(ruler), "upper");
	g_object_notify(G_OBJECT(ruler), "position");
}

static void
ppg_ruler_set_position (PpgRuler *ruler,
                        gdouble   position)
{
	g_return_if_fail(PPG_IS_RULER(ruler));

	ruler->priv->pos = position;
	gtk_widget_queue_draw(GTK_WIDGET(ruler));
	g_object_notify(G_OBJECT(ruler), "position");
}

static void
ppg_ruler_set_lower (PpgRuler *ruler,
                     gdouble   lower)
{
	g_return_if_fail(PPG_IS_RULER(ruler));

	ruler->priv->lower = lower;
	gtk_widget_queue_draw(GTK_WIDGET(ruler));
	g_object_notify(G_OBJECT(ruler), "lower");
}

static void
ppg_ruler_set_upper (PpgRuler *ruler,
                     gdouble   upper)
{
	g_return_if_fail(PPG_IS_RULER(ruler));

	ruler->priv->upper = upper;
	gtk_widget_queue_draw(GTK_WIDGET(ruler));
	g_object_notify(G_OBJECT(ruler), "upper");
}

static gboolean
ppg_ruler_motion_notify_event (GtkWidget *widget,
                               GdkEventMotion *motion)
{
	PpgRulerPrivate *priv;
	PpgRuler *ruler = (PpgRuler *)widget;
	GtkAllocation alloc;
	gdouble pos;

	g_return_val_if_fail(PPG_IS_RULER(ruler), FALSE);

	priv = ruler->priv;

	gtk_widget_get_allocation(widget, &alloc);
	pos = priv->lower + (motion->x / alloc.width * (priv->upper - priv->lower));
	ppg_ruler_set_position(ruler, pos);

	return FALSE;
}

static void
ppg_ruler_draw_arrow (PpgRuler *ruler)
{
	PpgRulerPrivate *priv;
	GtkStyle *style;
	GdkColor base_light;
	GdkColor base_dark;
	GdkColor hl_light;
	GdkColor hl_dark;
	cairo_t *cr;
	gint half;
	gint line_width;
	gint center;
	gint middle;
	gdouble top;
	gdouble bottom;
	gdouble left;
	gdouble right;

	g_return_if_fail(PPG_IS_RULER(ruler));

	priv = ruler->priv;
	style = gtk_widget_get_style(GTK_WIDGET(ruler));

	cr = gdk_cairo_create(priv->arrow);

	cairo_save(cr);
	cairo_set_operator(cr, CAIRO_OPERATOR_CLEAR);
	cairo_rectangle(cr, 0, 0, ARROW_SIZE, ARROW_SIZE);
	cairo_fill(cr);
	cairo_restore(cr);

	center = middle = half = ARROW_SIZE / 2;
	line_width = half / 6;

	base_light = style->light[GTK_STATE_SELECTED];
	base_dark = style->dark[GTK_STATE_SELECTED];
	hl_light = style->light[GTK_STATE_SELECTED];
	hl_dark = style->mid[GTK_STATE_SELECTED];

	top = middle - half + line_width + 0.5;
	bottom = middle + half - line_width + 0.5;
	left = center - half + line_width + 0.5;
	right = center +half - line_width - 0.5;

	cairo_set_line_width(cr, line_width);

	cairo_move_to(cr, left + line_width, top + line_width);
	cairo_line_to(cr, right + line_width, top + line_width);
	cairo_line_to(cr, right + line_width, middle + line_width);
	cairo_line_to(cr, center + line_width, bottom + line_width);
	cairo_line_to(cr, left + line_width, middle + line_width);
	cairo_line_to(cr, left + line_width, top + line_width);
	cairo_close_path(cr);
	cairo_set_source_rgba(cr, 0, 0, 0, 0.5);
	cairo_fill(cr);

	cairo_move_to(cr, left, top);
	cairo_line_to(cr, center, top);
	cairo_line_to(cr, center, bottom);
	cairo_line_to(cr, left, middle);
	cairo_line_to(cr, left, top);
	cairo_close_path(cr);
	gdk_cairo_set_source_color(cr, &base_light);
	cairo_fill(cr);

	cairo_move_to(cr, center, top);
	cairo_line_to(cr, right, top);
	cairo_line_to(cr, right, middle);
	cairo_line_to(cr, center, bottom);
	cairo_line_to(cr, center, top);
	cairo_close_path(cr);
	gdk_cairo_set_source_color(cr, &base_light);
	cairo_fill_preserve(cr);
	cairo_set_source_rgba(cr,
	                      base_dark.red / 65535.0,
	                      base_dark.green / 65535.0,
	                      base_dark.blue / 65535.0,
	                      0.5);
	cairo_fill(cr);

	cairo_move_to(cr, left + line_width, top + line_width);
	cairo_line_to(cr, right - line_width, top + line_width);
	cairo_line_to(cr, right - line_width, middle);
	cairo_line_to(cr, center, bottom - line_width - 0.5);
	cairo_line_to(cr, left + line_width, middle);
	cairo_line_to(cr, left + line_width, top + line_width);
	cairo_close_path(cr);
	gdk_cairo_set_source_color(cr, &hl_light);
	cairo_stroke(cr);

	cairo_move_to(cr, left, top);
	cairo_line_to(cr, right, top);
	cairo_line_to(cr, right, middle);
	cairo_line_to(cr, center, bottom);
	cairo_line_to(cr, left, middle);
	cairo_line_to(cr, left, top);
	cairo_close_path(cr);
	gdk_cairo_set_source_color(cr, &base_dark);
	cairo_stroke(cr);

	cairo_destroy(cr);
}

static inline gdouble
get_x_offset (PpgRulerPrivate *priv,
              GtkAllocation *alloc,
              gdouble value)
{
	gdouble a;
	gdouble b;

	if (value < priv->lower) {
		return 0.0;
	} else if (value > priv->upper) {
		return alloc->width;
	}

	a = priv->upper - priv->lower;
	b = value - priv->lower;
	return floor((b / a) * alloc->width);
}

static void
ppg_ruler_draw_ruler (PpgRuler *ruler)
{
	PpgRulerPrivate *priv;
	GtkAllocation alloc;
	PangoLayout *layout;
	cairo_t *cr;
	GtkStyle *style;
	GdkColor text_color;
	gint text_width;
	gint text_height;
	gdouble every = 1.0;
	gdouble n_seconds;
	gdouble v;
	gdouble p;
	gint x;
	gint xx;
	gint n;
	gint z = 0;

	g_return_if_fail(PPG_IS_RULER(ruler));

	priv = ruler->priv;

	gtk_widget_get_allocation(GTK_WIDGET(ruler), &alloc);
	style = gtk_widget_get_style(GTK_WIDGET(ruler));
	cr = gdk_cairo_create(priv->ruler);

	cairo_save(cr);
	cairo_set_operator(cr, CAIRO_OPERATOR_CLEAR);
	cairo_rectangle(cr, 0, 0, alloc.width, alloc.height);
	cairo_fill(cr);
	cairo_restore(cr);

	text_color = style->text[GTK_STATE_NORMAL];
	cairo_set_line_width(cr, 1.0);
	gdk_cairo_set_source_color(cr, &text_color);

	layout = pango_cairo_create_layout(cr);
	pango_layout_set_font_description(layout, priv->font_desc);
	pango_layout_set_markup(layout, "00:00:00", -1);
	pango_layout_get_pixel_size(layout, &text_width, &text_height);
	text_width += 5;

	n_seconds = priv->upper - priv->lower;
	if ((alloc.width / n_seconds) < text_width) {
		every = ceil(text_width / (alloc.width / n_seconds));
	}

	for (v = ceil(priv->lower); v < priv->upper; v += every) {
		gdk_cairo_set_source_color(cr, &text_color);
		x = get_x_offset(priv, &alloc, v);
		cairo_move_to(cr, x + 0.5, alloc.height - 1.5);
		cairo_line_to(cr, x + 0.5, 0.5);

		/*
		 * TODO: Mini lines.
		 */
		for (p = v, n = 0, z = 0;
		     p < v + every;
		     p += (every / 10), n++, z++)
		{
			if (n == 0 || n == 10) {
				continue;
			}

			xx = get_x_offset(priv, &alloc, p);
			cairo_move_to(cr, xx + 0.5, alloc.height - 1.5);
			if (z % 2 == 0) {
				cairo_line_to(cr, xx + 0.5, text_height + 8.5);
			} else {
				cairo_line_to(cr, xx + 0.5, text_height + 5.5);
			}
		}

		cairo_stroke(cr);

		cairo_move_to(cr, x + 1.5, 1.5);
		ppg_ruler_update_layout_text(ruler, layout, v);
		pango_cairo_show_layout(cr, layout);
	}

	g_object_unref(layout);
	cairo_destroy(cr);
}

static void
ppg_ruler_realize (GtkWidget *widget) 
{
	PpgRuler *ruler = (PpgRuler *)widget;
	PpgRulerPrivate *priv;
	GdkColormap *colormap;
	GdkVisual *visual;

	g_return_if_fail(PPG_IS_RULER(ruler));

	priv = ruler->priv;

	GTK_WIDGET_CLASS(ppg_ruler_parent_class)->realize(widget);
	gtk_widget_queue_resize(widget);

	/*
	 * Create pixmap for arrow.
	 */
	if (priv->arrow) {
		g_object_unref(priv->arrow);
	}
	priv->arrow = gdk_pixmap_new(NULL, ARROW_SIZE, ARROW_SIZE, 32);
	visual = gdk_visual_get_best_with_depth(32);
	colormap = gdk_colormap_new(visual, FALSE);
	gdk_drawable_set_colormap(priv->arrow, colormap);
}


static gboolean
ppg_ruler_expose_event (GtkWidget *widget,
                        GdkEventExpose *expose)
{
	PpgRuler *ruler = (PpgRuler *)widget;
	PpgRulerPrivate *priv;
	GtkAllocation alloc;
	cairo_t *cr;
	gint x;
	gint y;

	g_return_val_if_fail(PPG_IS_RULER(ruler), FALSE);

	GTK_WIDGET_CLASS(ppg_ruler_parent_class)->expose_event(widget, expose);

	priv = ruler->priv;

	gtk_widget_get_allocation(widget, &alloc);
	cr = gdk_cairo_create(expose->window);

	if (priv->dirty) {
		ppg_ruler_draw_arrow(ruler);
		ppg_ruler_draw_ruler(ruler);
		priv->dirty = FALSE;
	}

	cairo_rectangle(cr, 0, 0, alloc.width, alloc.height);
	gdk_cairo_set_source_pixmap(cr, priv->ruler, 0, 0);
	cairo_fill(cr);

	x = (gint)(((priv->pos - priv->lower) /
                (priv->upper - priv->lower) *
                alloc.width) - (ARROW_SIZE / 2.0));
	y = alloc.height - ARROW_SIZE - 1;
	gdk_cairo_set_source_pixmap(cr, priv->arrow, x, y);
	cairo_rectangle(cr, x, y, ARROW_SIZE, ARROW_SIZE);
	cairo_fill(cr);

	cairo_destroy(cr);

	return FALSE;
}

/**
 * ppg_ruler_finalize:
 * @object: (in): A #PpgRuler.
 *
 * Finalizer for a #PpgRuler instance.  Frees any resources held by
 * the instance.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
ppg_ruler_finalize (GObject *object)
{
	PpgRulerPrivate *priv = PPG_RULER(object)->priv;

	pango_font_description_free(priv->font_desc);

	G_OBJECT_CLASS(ppg_ruler_parent_class)->finalize(object);
}

static void
ppg_ruler_size_request (GtkWidget *widget,
                        GtkRequisition *req)
{
	PpgRuler *ruler = (PpgRuler *)widget;
	PpgRulerPrivate *priv;
	GdkWindow *window;
	PangoLayout *layout;
	cairo_t *cr;
	gint width;
	gint height;

	g_return_if_fail(PPG_IS_RULER(ruler));

	priv = ruler->priv;

	GTK_WIDGET_CLASS(ppg_ruler_parent_class)->size_request(widget, req);

	if ((window = gtk_widget_get_window(widget))) {
		cr = gdk_cairo_create(window);
		layout = pango_cairo_create_layout(cr);
		pango_layout_set_text(layout, "00:00:00", -1);

		pango_layout_get_pixel_size(layout, &width, &height);
		height += 12;

		g_object_unref(layout);
		cairo_destroy(cr);

		if (req->height < height) {
			req->height = height;
		}
	}
}

static void
ppg_ruler_size_allocate (GtkWidget *widget,
                         GtkAllocation *alloc)
{
	PpgRuler *ruler = (PpgRuler *)widget;
	PpgRulerPrivate *priv;
	GdkColormap *colormap;
	GdkVisual *visual;

	g_return_if_fail(PPG_IS_RULER(ruler));

	priv = ruler->priv;

	GTK_WIDGET_CLASS(ppg_ruler_parent_class)->size_allocate(widget, alloc);

	if (priv->ruler) {
		g_object_unref(priv->ruler);
	}

	priv->ruler = gdk_pixmap_new(NULL, alloc->width, alloc->height, 32);
	visual = gdk_visual_get_best_with_depth(32);
	colormap = gdk_colormap_new(visual, FALSE);
	gdk_drawable_set_colormap(priv->ruler, colormap);

	if (GTK_WIDGET_DRAWABLE(widget)) {
		ppg_ruler_draw_ruler(ruler);
	}
}

static void
ppg_ruler_style_set (GtkWidget *widget,
                     GtkStyle *old_style)
{
	PpgRulerPrivate *priv;
	PpgRuler *ruler = (PpgRuler *)widget;

	g_return_if_fail(PPG_IS_RULER(ruler));

	priv = ruler->priv;

	GTK_WIDGET_CLASS(ppg_ruler_parent_class)->style_set(widget, old_style);

	priv->dirty = TRUE;
	gtk_widget_queue_draw(widget);
}

/**
 * ppg_ruler_set_property:
 * @object: (in): A #GObject.
 * @prop_id: (in): The property identifier.
 * @value: (out): The given property.
 * @pspec: (in): A #ParamSpec.
 *
 * Get a given #GObject property.
 */
static void
ppg_ruler_get_property (GObject    *object,
                        guint       prop_id,
                        GValue     *value,
                        GParamSpec *pspec)
{
	PpgRuler *ruler = PPG_RULER(object);

	switch (prop_id) {
	case PROP_LOWER:
		g_value_set_double(value, ruler->priv->lower);
		break;
	case PROP_UPPER:
		g_value_set_double(value, ruler->priv->upper);
		break;
	case PROP_POSITION:
		g_value_set_double(value, ruler->priv->pos);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
	}
}

/**
 * ppg_ruler_set_property:
 * @object: (in): A #GObject.
 * @prop_id: (in): The property identifier.
 * @value: (in): The given property.
 * @pspec: (in): A #ParamSpec.
 *
 * Set a given #GObject property.
 */
static void
ppg_ruler_set_property (GObject      *object,
                        guint         prop_id,
                        const GValue *value,
                        GParamSpec   *pspec)
{
	PpgRuler *ruler = PPG_RULER(object);

	switch (prop_id) {
	case PROP_LOWER:
		ppg_ruler_set_lower(ruler, g_value_get_double(value));
		break;
	case PROP_UPPER:
		ppg_ruler_set_upper(ruler, g_value_get_double(value));
		break;
	case PROP_POSITION:
		ppg_ruler_set_position(ruler, g_value_get_double(value));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
	}
}

/**
 * ppg_ruler_class_init:
 * @klass: (in): A #PpgRulerClass.
 *
 * Initializes the #PpgRulerClass and prepares the vtable.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
ppg_ruler_class_init (PpgRulerClass *klass)
{
	GObjectClass *object_class;
	GtkWidgetClass *widget_class;

	object_class = G_OBJECT_CLASS(klass);
	object_class->finalize = ppg_ruler_finalize;
	object_class->get_property = ppg_ruler_get_property;
	object_class->set_property = ppg_ruler_set_property;
	g_type_class_add_private(object_class, sizeof(PpgRulerPrivate));

	widget_class = GTK_WIDGET_CLASS(klass);
	widget_class->expose_event = ppg_ruler_expose_event;
	widget_class->motion_notify_event = ppg_ruler_motion_notify_event;
	widget_class->realize = ppg_ruler_realize;
	widget_class->size_allocate = ppg_ruler_size_allocate;
	widget_class->size_request = ppg_ruler_size_request;
	widget_class->style_set = ppg_ruler_style_set;

	g_object_class_install_property(object_class,
	                                PROP_LOWER,
	                                g_param_spec_double("lower",
	                                                    "lower",
	                                                    "loser",
	                                                    0,
	                                                    G_MAXDOUBLE,
	                                                    0,
	                                                    G_PARAM_READWRITE));

	g_object_class_install_property(object_class,
	                                PROP_UPPER,
	                                g_param_spec_double("upper",
	                                                    "upper",
	                                                    "upper",
	                                                    0,
	                                                    G_MAXDOUBLE,
	                                                    0,
	                                                    G_PARAM_READWRITE));

	g_object_class_install_property(object_class,
	                                PROP_POSITION,
	                                g_param_spec_double("position",
	                                                    "position",
	                                                    "position",
	                                                    0,
	                                                    G_MAXDOUBLE,
	                                                    0,
	                                                    G_PARAM_READWRITE));
}

/**
 * ppg_ruler_init:
 * @ruler: (in): A #PpgRuler.
 *
 * Initializes the newly created #PpgRuler instance.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
ppg_ruler_init (PpgRuler *ruler)
{
	PpgRulerPrivate *priv;

	priv = ruler->priv =
		G_TYPE_INSTANCE_GET_PRIVATE(ruler, PPG_TYPE_RULER, PpgRulerPrivate);

	gtk_widget_add_events(GTK_WIDGET(ruler), GDK_POINTER_MOTION_MASK);

	priv->font_desc = pango_font_description_new();
	pango_font_description_set_family_static(priv->font_desc, "Monospace");
	pango_font_description_set_size(priv->font_desc, 8 * PANGO_SCALE);
}
