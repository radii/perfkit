/* uber-graph.h
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

#ifndef __UBER_GRAPH_H__
#define __UBER_GRAPH_H__

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define UBER_TYPE_GRAPH            (uber_graph_get_type())
#define UBER_GRAPH(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), UBER_TYPE_GRAPH, UberGraph))
#define UBER_GRAPH_CONST(obj)      (G_TYPE_CHECK_INSTANCE_CAST ((obj), UBER_TYPE_GRAPH, UberGraph const))
#define UBER_GRAPH_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  UBER_TYPE_GRAPH, UberGraphClass))
#define UBER_IS_GRAPH(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), UBER_TYPE_GRAPH))
#define UBER_IS_GRAPH_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  UBER_TYPE_GRAPH))
#define UBER_GRAPH_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  UBER_TYPE_GRAPH, UberGraphClass))

typedef struct _UberGraph        UberGraph;
typedef struct _UberGraphClass   UberGraphClass;
typedef struct _UberGraphPrivate UberGraphPrivate;

/**
 * UberRange:
 *
 * #UberRange is a structure that encapsulates the range of a particular
 * scale.  It contains the beginning value, ending value, and a pre-calculated
 * range between the values.
 */
typedef struct
{
	gdouble begin;
	gdouble end;
	gdouble range;
} UberRange;

/**
 * UberScale:
 * @graph: An #UberGraph.
 * @values: The range of raw values in the graph.
 * @pixels: The range of pixel values in the graph.
 * @value: The value to scale.
 *
 * #UberScale is a transformation function that transforms a value from the
 * raw scale into a value within the pixel scale.
 *
 * Returns: %TRUE if successful; otherwise %FALSE.
 * Side effects: Implementation specific.
 */
typedef gboolean (*UberScale) (UberGraph       *graph,
                               const UberRange *values,
                               const UberRange *pixels,
                               gdouble         *value);

/**
 * UberGraphFunc:
 * @graph: A #UberGraph.
 * @line: The line of which to retrieve the value.
 * @value: A location for the resulting value.
 * @user_data: User data supplied to uber_graph_set_value_func().
 *
 * Callback to retrieve the next value for the graph.  Uber graph uses a
 * callback model so that the graph can be drawn in a way that doesn't cause
 * heavy jitter on the graph rendering.
 *
 * Returns: %TRUE if successful; otherwise %FALSE.
 * Side effects: Implementation specific.
 */
typedef gboolean (*UberGraphFunc) (UberGraph *graph,
                                   gint       line,
                                   gdouble   *value,
                                   gpointer   user_data);

/**
 * UberGraphFormat:
 * @UBER_GRAPH_DIRECT: The raw values should be used.
 * @UBER_GRAPH_DIRECT1024: The raw values should be used, using the
 *   kibibyte scale.
 * @UBER_GRAPH_PERCENT: The labels are from 0 to 100 percent.
 *
 * #UberGraphFormat describes how the label values should be determined
 * for the graph.
 */
typedef enum
{
	UBER_GRAPH_DIRECT,
	UBER_GRAPH_DIRECT1024,
	UBER_GRAPH_PERCENT,
} UberGraphFormat;

struct _UberGraph
{
	GtkDrawingArea parent;

	/*< private >*/
	UberGraphPrivate *priv;
};

struct _UberGraphClass
{
	GtkDrawingAreaClass parent_class;
};

guint          uber_graph_add_line        (UberGraph       *graph);
GType          uber_graph_get_type        (void) G_GNUC_CONST;
gboolean       uber_graph_get_yautoscale  (UberGraph       *graph);
GtkWidget*     uber_graph_new             (void);
void           uber_graph_set_format      (UberGraph       *graph,
                                           UberGraphFormat  format);
void           uber_graph_set_fps         (UberGraph       *graph,
                                           gint             fps);
void           uber_graph_set_scale       (UberGraph       *graph,
                                           UberScale        scale);
void           uber_graph_set_stride      (UberGraph       *graph,
                                           gint             stride);
void           uber_graph_set_value_func  (UberGraph       *graph,
                                           UberGraphFunc    func,
                                           gpointer         user_data,
                                           GDestroyNotify   notify);
void           uber_graph_set_yautoscale  (UberGraph       *graph,
                                           gboolean         yautoscale);
void           uber_graph_set_yrange      (UberGraph       *graph,
                                           const UberRange *range);
gboolean       uber_scale_linear          (UberGraph       *graph,
                                           const UberRange *values,
                                           const UberRange *pixels,
                                           gdouble         *value);

G_END_DECLS

#endif /* __UBER_GRAPH_H__ */
