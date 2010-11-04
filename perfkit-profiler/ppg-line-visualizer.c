/* ppg-line-visualizer.c
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

#include "ppg-color.h"
#include "ppg-line-visualizer.h"

G_DEFINE_TYPE(PpgLineVisualizer, ppg_line_visualizer, PPG_TYPE_VISUALIZER)

typedef struct
{
	gint      key;
	PpgModel *model;
	gchar    *title;
	GdkColor  color;
	gboolean  fill;
} Line;

struct _PpgLineVisualizerPrivate
{
	GArray       *lines;
	ClutterActor *actor;
	guint         paint_handler;
	guint         resize_handler;
};

void
ppg_line_visualizer_append (PpgLineVisualizer *visualizer,
                            const gchar *name,
                            GdkColor *color,
                            gboolean fill,
                            PpgModel *model,
                            gint key)
{
	PpgLineVisualizerPrivate *priv;
	Line line = { 0 };

	g_return_if_fail(PPG_IS_LINE_VISUALIZER(visualizer));

	priv = visualizer->priv;

	if (color) {
		line.color = *color;
	} else {
		gdk_color_parse("#000", &line.color);
	}

	line.title = g_strdup(name);
	line.model = model;
	line.key = key;
	line.fill = fill;

	g_array_append_val(priv->lines, line);
}

static ClutterActor*
ppg_line_visualizer_get_actor (PpgVisualizer *visualizer)
{
	g_return_val_if_fail(PPG_IS_LINE_VISUALIZER(visualizer), NULL);
	return PPG_LINE_VISUALIZER(visualizer)->priv->actor;
}

static void
ppg_line_visualizer_resize_surface (PpgLineVisualizer *visualizer)
{
	PpgLineVisualizerPrivate *priv;
	gfloat width;
	gfloat height;

	g_return_if_fail(PPG_IS_LINE_VISUALIZER(visualizer));

	priv = visualizer->priv;

	g_object_get(priv->actor,
	             "height", &height,
	             "width", &width,
	             NULL);

	g_object_set(priv->actor,
	             "surface-height", (gint)height,
	             "surface-width", (gint)width,
	             NULL);
}

static gboolean
ppg_line_visualizer_resize_timeout (gpointer user_data)
{
	PpgLineVisualizer *visualizer = (PpgLineVisualizer *)user_data;

	g_return_val_if_fail(PPG_IS_LINE_VISUALIZER(visualizer), FALSE);

	visualizer->priv->resize_handler = 0;
	ppg_line_visualizer_resize_surface(visualizer);

	return FALSE;
}

static void
ppg_line_visualizer_queue_resize (PpgLineVisualizer *visualizer)
{
	PpgLineVisualizerPrivate *priv;

	g_return_if_fail(PPG_IS_LINE_VISUALIZER(visualizer));

	priv = visualizer->priv;

	if (priv->resize_handler) {
		return;
	}

	priv->resize_handler =
		g_timeout_add(0, ppg_line_visualizer_resize_timeout, visualizer);
}

static void
ppg_line_visualizer_notify_allocation (ClutterActor *actor,
                                       GParamSpec *pspec,
                                       PpgLineVisualizer *visualizer)
{
	ppg_line_visualizer_queue_resize(visualizer);
}

static inline gdouble
get_x_offset (gdouble begin,
              gdouble end,
              gdouble width,
              gdouble value)
{
	return (value - begin) / (end - begin) * width;
}

static inline gdouble
get_y_offset (gdouble lower,
              gdouble upper,
              gdouble height,
              gdouble value)
{
	return height - ((value - lower) / (upper - lower) * height);
}

static void
ppg_line_visualizer_draw (PpgVisualizer *visualizer)
{
	PpgLineVisualizerPrivate *priv;
	PpgModelIter iter;
	Line *line;
	PpgColorIter color;
	cairo_t *cr;
	GValue value = { 0 };
	gfloat height;
	gfloat width;
	gdouble x;
	gdouble y;
	gdouble begin;
	gdouble end;
	gdouble lower;
	gdouble upper;
	gdouble val = 0;
	gint i;

	g_return_if_fail(PPG_IS_LINE_VISUALIZER(visualizer));

	priv = PPG_LINE_VISUALIZER(visualizer)->priv;

	clutter_cairo_texture_clear(CLUTTER_CAIRO_TEXTURE(priv->actor));
	cr = clutter_cairo_texture_create(CLUTTER_CAIRO_TEXTURE(priv->actor));

	g_object_get(visualizer,
	             "begin", &begin,
	             "end", &end,
	             NULL);

	g_object_get(priv->actor,
	             "width", &width,
	             "height", &height,
	             NULL);

	/* FIXME: */
	lower = 0;
	upper = 200;

	ppg_color_iter_init(&color);

	cairo_set_line_width(cr, 1.0);

	for (i = 0; i < priv->lines->len; i++) {
		line = &g_array_index(priv->lines, Line, i);
		cairo_move_to(cr, 0, height);
		gdk_cairo_set_source_color(cr, &color.color);

		if (!ppg_model_get_iter_first(line->model, &iter)) {
			goto next;
		}

		do {
			ppg_model_get_value(line->model, &iter, line->key, &value);
			if (G_VALUE_HOLDS(&value, G_TYPE_DOUBLE)) {
				val = g_value_get_double(&value);
			} else if (G_VALUE_HOLDS(&value, G_TYPE_INT)) {
				val = g_value_get_int(&value);
			} else if (G_VALUE_HOLDS(&value, G_TYPE_UINT)) {
				val = g_value_get_uint(&value);
			} else {
				g_critical("HOLDS %s", g_type_name(value.g_type));
				g_assert_not_reached();
			}
			x = get_x_offset(begin, end, width, iter.time);
			y = get_y_offset(lower, upper, height, val);
			cairo_line_to(cr, x, y);
			g_value_unset(&value);
		} while (ppg_model_iter_next(line->model, &iter));

		cairo_stroke(cr);

	  next:
		ppg_color_iter_next(&color);
	}

	cairo_destroy(cr);
}

/**
 * ppg_line_visualizer_finalize:
 * @object: (in): A #PpgLineVisualizer.
 *
 * Finalizer for a #PpgLineVisualizer instance.  Frees any resources held by
 * the instance.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
ppg_line_visualizer_finalize (GObject *object)
{
	PpgLineVisualizerPrivate *priv = PPG_LINE_VISUALIZER(object)->priv;

	g_array_unref(priv->lines);

	G_OBJECT_CLASS(ppg_line_visualizer_parent_class)->finalize(object);
}

/**
 * ppg_line_visualizer_class_init:
 * @klass: (in): A #PpgLineVisualizerClass.
 *
 * Initializes the #PpgLineVisualizerClass and prepares the vtable.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
ppg_line_visualizer_class_init (PpgLineVisualizerClass *klass)
{
	GObjectClass *object_class;
	PpgVisualizerClass *visualizer_class;

	object_class = G_OBJECT_CLASS(klass);
	object_class->finalize = ppg_line_visualizer_finalize;
	g_type_class_add_private(object_class, sizeof(PpgLineVisualizerPrivate));

	visualizer_class = PPG_VISUALIZER_CLASS(klass);
	visualizer_class->get_actor = ppg_line_visualizer_get_actor;
	visualizer_class->draw = ppg_line_visualizer_draw;
}

/**
 * ppg_line_visualizer_init:
 * @visualizer: (in): A #PpgLineVisualizer.
 *
 * Initializes the newly created #PpgLineVisualizer instance.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
ppg_line_visualizer_init (PpgLineVisualizer *visualizer)
{
	PpgLineVisualizerPrivate *priv;

	priv = G_TYPE_INSTANCE_GET_PRIVATE(visualizer, PPG_TYPE_LINE_VISUALIZER,
	                                   PpgLineVisualizerPrivate);
	visualizer->priv = priv;

	priv->lines = g_array_new(FALSE, FALSE, sizeof(Line));

	priv->actor = g_object_new(CLUTTER_TYPE_CAIRO_TEXTURE,
	                           "surface-width", 1,
	                           "surface-height", 1,
	                           "natural-height", 32.0f,
	                           NULL);

	g_signal_connect_after(priv->actor, "notify::allocation",
	                       G_CALLBACK(ppg_line_visualizer_notify_allocation),
	                       visualizer);
}
