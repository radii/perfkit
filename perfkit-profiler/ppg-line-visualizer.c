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

#include "ppg-line-visualizer.h"

G_DEFINE_TYPE(PpgLineVisualizer, ppg_line_visualizer, PPG_TYPE_VISUALIZER)

typedef struct
{
	gint      row;
	PpgModel *model;
	gchar    *title;
	GdkColor  color;
	gboolean  fill;
} Line;

struct _PpgLineVisualizerPrivate
{
	GArray *lines;

	ClutterActor *actor;

	guint paint_handler;
	guint resize_handler;
};

void
ppg_line_visualizer_append (PpgLineVisualizer *visualizer,
                            const gchar *name,
                            GdkColor *color,
                            gboolean fill,
                            PpgModel *model,
                            gint row)
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
	line.row = row;
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
ppg_line_visualizer_paint (PpgLineVisualizer *visualizer)
{
	PpgLineVisualizerPrivate *priv;
	//PpgModelIter iter;
	//Line *line;
	gdouble begin;
	gdouble end;
	//gdouble lower;
	//gdouble upper;
	//gdouble offset;
	//gdouble value;
	gfloat height;
	gfloat width;
	cairo_t *cr;
	//gint i;

	g_return_if_fail(PPG_IS_LINE_VISUALIZER(visualizer));

	priv = visualizer->priv;

	g_object_get(priv->actor,
	             "height", &height,
	             "width", &width,
	             NULL);

	/*
	 * FIXME: Get real time range.
	 */
#if 0
	g_object_get(visualizer,
	             "begin", &begin,
	             "end", &end,
	             NULL);
#endif

	begin = 0.0;
	end = clutter_actor_get_width(priv->actor);

	cr = clutter_cairo_texture_create(CLUTTER_CAIRO_TEXTURE(priv->actor));

	cairo_save(cr);
	cairo_set_operator(cr, CAIRO_OPERATOR_CLEAR);
	cairo_rectangle(cr, 0, 0, width, height);
	cairo_fill(cr);
	cairo_restore(cr);

	cairo_set_line_width(cr, 1.0);

#if 0
	cairo_rectangle(cr, 0, 0, width, height);
	cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, 0.95);
	cairo_fill(cr);
#endif

#if 0
	for (i = 0; i < priv->lines->len; i++) {
		line = &g_array_index(priv->lines, Line, i);
		gdk_cairo_set_source_color(cr, &line->color);

		if (!ppg_model_get_iter_at(line->model, &iter, begin, end, PPG_RESOLUTION_FULL)) {
			continue;
		}

		//ppg_model_get_bounds(line->model, &iter, &lower, &upper);

		do {
			//ppg_model_get(line->model, &iter, line->row, &offset, &value);
			//cairo_line_to(cr, offset, value);
		} while (ppg_model_iter_next(line->model, &iter));

		if (line->fill) {
			cairo_line_to(cr, offset, height);
			cairo_line_to(cr, 0.0, height);
			cairo_close_path(cr);
			cairo_fill_preserve(cr);
		}

		cairo_stroke(cr);
	}
#endif

	cairo_destroy(cr);
}

static gboolean
ppg_line_visualizer_paint_timeout (gpointer user_data)
{
	PpgLineVisualizer *visualizer = (PpgLineVisualizer *)user_data;
	PpgLineVisualizerPrivate *priv;

	g_return_val_if_fail(PPG_IS_LINE_VISUALIZER(visualizer), FALSE);

	priv = visualizer->priv;

	priv->paint_handler = 0;
	ppg_line_visualizer_paint(user_data);

	return FALSE;
}

static void
ppg_line_visualizer_queue_paint (PpgLineVisualizer *visualizer)
{
	PpgLineVisualizerPrivate *priv;

	g_return_if_fail(PPG_IS_LINE_VISUALIZER(visualizer));

	priv = visualizer->priv;

	if (priv->paint_handler) {
		return;
	}

	priv->paint_handler =
		g_timeout_add(0, ppg_line_visualizer_paint_timeout, visualizer);
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

	ppg_line_visualizer_queue_paint(visualizer);
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

	ppg_line_visualizer_queue_paint(visualizer);
}
