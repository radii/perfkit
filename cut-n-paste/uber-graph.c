/* uber-graph.c
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

#if HAVE_CONFIG_H
#include "config.h"
#endif

#include <math.h>

#include "uber-graph.h"
#include "uber-buffer.h"

#define BASE_CLASS   (GTK_WIDGET_CLASS(uber_graph_parent_class))
#define DEFAULT_SIZE (64)
#define GET_PRIVATE  G_TYPE_INSTANCE_GET_PRIVATE

#ifndef g_malloc0_n
#define g_malloc0_n(a,b) g_malloc0(a * b)
#endif

#define KIBIBYTE     (1024)
#define KIBIBYTE_STR ("Ki")
#define MEBIBYTE     (1048576)
#define MEBIBYTE_STR ("Mi")
#define GIBIBYTE     (1073741824)
#define GIBIBYTE_STR ("Gi")

#define SCALE_FACTOR (1.3334)

#define GET_PIXEL_RANGE(pr, rect)                \
    G_STMT_START {                               \
        (pr).begin = (rect).y + 1;               \
        (pr).end = (rect).y + (rect).height - 1; \
        (pr).range = (rect).height - 2;          \
    } G_STMT_END

#ifdef UBER_TRACE
#define TRACE(_f,...) \
    G_STMT_START { \
        g_log(G_LOG_DOMAIN, 1 << G_LOG_LEVEL_USER_SHIFT, _f, ## __VA_ARGS__); \
    } G_STMT_END
#define ENTRY TRACE("ENTRY: %s():%d", G_STRFUNC, __LINE__)
#define EXIT \
    G_STMT_START { \
        TRACE(" EXIT: %s():%d", G_STRFUNC, __LINE__); \
        return; \
    } G_STMT_END
#define RETURN(_r) \
    G_STMT_START { \
        TRACE(" EXIT: %s():%d", G_STRFUNC, __LINE__); \
        return (_r); \
	} G_STMT_END
#define GOTO(_l) \
    G_STMT_START { \
        TRACE(" GOTO: %s():%d %s", G_STRFUNC, __LINE__, #_l); \
        goto _l; \
	} G_STMT_END
#define CASE(_l) \
    case _l: \
        TRACE(" CASE: %s():%d %s", G_STRFUNC, __LINE__, #_l)
#else
#define ENTRY
#define EXIT       return
#define RETURN(_r) return (_r)
#define GOTO(_l)   goto _l
#define CASE(_l)   case _l:
#endif

/**
 * SECTION:uber-graph
 * @title: UberGraph
 * @short_description: Realtime, side-scrolling graph that is low on CPU.
 *
 * #UberGraph is a graphing widget that provides live scrolling features
 * for realtime graphs.  It uses server-side pixmaps to reduce the rendering
 * overhead.  Multiple pixmaps are used which can be blitted for additional
 * speed boost.
 *
 * When adding new values to the graph, the contents of the pixmap are shifted
 * and the new sliver of content added to the pixmap.  This helps reduce the
 * amount of data to send to the X-server.
 */

G_DEFINE_TYPE(UberGraph, uber_graph, GTK_TYPE_DRAWING_AREA)

typedef struct
{
	GdkPixmap   *bg_pixmap;   /* Server-side pixmap for background. */
	GdkPixmap   *fg_pixmap;   /* Server-side pixmap for foreground. */
	cairo_t     *bg_cairo;    /* Cairo context for foreground pixmap. */
	cairo_t     *fg_cairo;    /* Cairo context for background pixmap. */
	PangoLayout *tick_layout; /* Pango layout for tick labels. */
} GraphInfo;

typedef struct
{
	UberBuffer *buffer;
	UberBuffer *scaled;
	GdkColor    color;
} LineInfo;

struct _UberGraphPrivate
{
	GraphInfo         info[2];         /* Two GraphInfo's for swapping. */
	gboolean          flipped;         /* Which GraphInfo is active. */
	gint              tick_len;        /* Length of axis ticks in pixels. */
	gint              fps;             /* Frames per second. */
	gint              fps_off;         /* Offset in frame-slide */
	gint              fps_to;          /* Frames per second timeout (in MS) */
	gint              stride;          /* Number of data points to store. */
	gfloat            fps_each;        /* How much each frame skews. */
	gfloat            x_each;          /* Precalculated space between points.  */
	UberGraphFormat   format;          /* The graph format. */
	guint             fps_handler;     /* GSource identifier for invalidating rect. */
	guint             down_handler;    /* Downscale timeout handler. */
	UberScale         scale;           /* Scaling of values to pixels. */
	UberRange         yrange;          /* Y-Axis range in for raw values. */
	GArray           *lines;           /* Lines to draw. */
	gboolean          bg_dirty;        /* Do we need to update the background. */
	gboolean          fg_dirty;        /* Do we need to update the foreground. */
	gboolean          yautoscale;      /* Should the graph autoscale to handle values
	                                    * outside the current range. */
	GdkGC            *bg_gc;           /* Drawing context for blitting background */
	GdkGC            *fg_gc;           /* Drawing context for blitting foreground */
	GdkRectangle      x_tick_rect;     /* Pre-calculated X tick area. */
	GdkRectangle      y_tick_rect;     /* Pre-calculated Y tick area. */
	GdkRectangle      content_rect;    /* Main content area. */
	gchar           **colors;          /* Array of colors to assign. */
	gint              colors_len;      /* Length of colors array. */
	gint              color;           /* Next color to hand out. */
	UberGraphFunc     value_func;      /* Callback to retrieve next value. */
	gpointer          value_user_data; /* User data for callback. */
	GDestroyNotify    value_notify;    /* Cleanup callback for value_user_data. */
};

typedef struct
{
	UberGraph *graph;
	GraphInfo *info;
	UberScale  scale;
	UberRange  pixel_range;
	UberRange  value_range;
	gdouble    last_y;
	gdouble    last_x;
	gdouble    x_epoch;
	gint       offset;
	gboolean   first;
} RenderClosure;

enum
{
	LAYOUT_TICK,
};

static const gchar *default_colors[] = {
	"#3465a4",
	"#73d216",
	"#75507b",
	"#a40000",
};

static void gdk_cairo_rectangle_clean         (cairo_t      *cr,
                                               GdkRectangle *rect);
static void pango_layout_get_pixel_rectangle  (PangoLayout  *layout,
                                               GdkRectangle *rect);
static void uber_graph_calculate_rects        (UberGraph    *graph);
static void uber_graph_init_graph_info        (UberGraph    *graph,
                                               GraphInfo    *info);
static void uber_graph_render_fg_shifted_task (UberGraph    *graph,
                                               GraphInfo    *src,
                                               GraphInfo    *dst);
static void uber_graph_render_fg_task         (UberGraph    *graph,
                                               GraphInfo    *info);
static void uber_graph_render_bg_task         (UberGraph    *graph,
                                               GraphInfo    *info);
static void uber_graph_scale_changed          (UberGraph    *graph);
static void uber_graph_update_scaled          (UberGraph    *graph);

/**
 * uber_graph_new:
 *
 * Creates a new instance of #UberGraph.
 *
 * Returns: the newly created instance of #UberGraph.
 * Side effects: None.
 */
GtkWidget*
uber_graph_new (void)
{
	UberGraph *graph;

	ENTRY;
	graph = g_object_new(UBER_TYPE_GRAPH, NULL);
	RETURN(GTK_WIDGET(graph));
}

static inline void
uber_graph_copy_background (UberGraph *graph, /* IN */
                            GraphInfo *src,   /* IN */
                            GraphInfo *dst)   /* IN */
{
	UberGraphPrivate *priv;
	GtkAllocation alloc;

	g_return_if_fail(UBER_IS_GRAPH(graph));

	ENTRY;
	priv = graph->priv;
	gtk_widget_get_allocation(GTK_WIDGET(graph), &alloc);
	gdk_draw_drawable(dst->bg_pixmap, priv->bg_gc, src->bg_pixmap,
	                  0, 0, 0, 0, alloc.width, alloc.height);
	EXIT;
}

static void
uber_graph_scale_changed (UberGraph *graph) /* IN */
{
	UberGraphPrivate *priv;

	g_return_if_fail(UBER_IS_GRAPH(graph));

	ENTRY;
	priv = graph->priv;
	uber_graph_update_scaled(graph);
	uber_graph_init_graph_info(graph, &priv->info[0]);
	uber_graph_init_graph_info(graph, &priv->info[1]);
	uber_graph_calculate_rects(graph);
	uber_graph_render_bg_task(graph, &priv->info[0]);
	uber_graph_copy_background(graph, &priv->info[0], &priv->info[1]);
	priv->fg_dirty = TRUE;
	gtk_widget_queue_draw(GTK_WIDGET(graph));
	EXIT;
}

/**
 * uber_graph_set_scale:
 * @graph: An #UberGraph.
 * @scale: The scale function.
 *
 * Sets the transformation scale from input values to pixels.
 *
 * Returns: None.
 * Side effects: None.
 */
void
uber_graph_set_scale (UberGraph *graph, /* IN */
                      UberScale  scale) /* IN */
{
	UberGraphPrivate *priv;

	g_return_if_fail(UBER_IS_GRAPH(graph));
	g_return_if_fail(scale != NULL);

	ENTRY;
	priv = graph->priv;
	priv->scale = scale;
	uber_graph_scale_changed(graph);
	EXIT;
}

/**
 * uber_graph_set_value_func:
 * @graph: A UberGraph.
 * @func: The callback function.
 * @user_data: user data for @func.
 * @notify: A #GDestroyNotify to cleanup after user_data.
 *
 * Sets the function callback to retrieve the next value for a particular
 * graph line.
 *
 * Returns: None.
 * Side effects: None.
 */
void
uber_graph_set_value_func (UberGraph      *graph,     /* IN */
                           UberGraphFunc   func,      /* IN */
                           gpointer        user_data, /* IN */
                           GDestroyNotify  notify)    /* IN */
{
	UberGraphPrivate *priv;

	g_return_if_fail(UBER_IS_GRAPH(graph));
	g_return_if_fail(func != NULL);

	ENTRY;
	priv = graph->priv;
	if (priv->value_notify) {
		priv->value_notify(priv->value_user_data);
	}
	priv->value_notify = notify;
	priv->value_user_data = user_data;
	priv->value_func = func;
	EXIT;
}

/**
 * uber_graph_get_next_value:
 * @graph: A #UberGraph.
 *
 * Retrieves the next value for a particular line in the graph.
 *
 * Returns: None.
 * Side effects: None.
 */
static inline void
uber_graph_get_next_value (UberGraph *graph, /* IN */
                           gint       line,  /* IN */
                           LineInfo  *info,  /* IN */
                           gdouble   *value) /* OUT */
{
	UberGraphPrivate *priv;

	g_return_if_fail(UBER_IS_GRAPH(graph));
	g_return_if_fail(info != NULL);
	g_return_if_fail(value != NULL);

	ENTRY;
	priv = graph->priv;
	*value = -INFINITY;
	if (G_LIKELY(priv->value_func)) {
		priv->value_func(graph, line, value, priv->value_user_data);
	}
	EXIT;
}

/**
 * uber_graph_append:
 * @graph: A #UberGraph.
 * @info: A LineInfo.
 *
 * Appends @value to the LineInfo.  If the graph is set to autoscale
 * and the scale was changed, %TRUE will be returned.
 *
 * Returns: %TRUE if the scale changed; otherwise %FALSE.
 * Side effects: None.
 */
static inline gboolean
uber_graph_append (UberGraph *graph, /* IN */
                   LineInfo  *info,  /* IN */
                   gdouble    value) /* IN */
{
	UberGraphPrivate *priv;
	UberRange pixel_range;
	gboolean scale_changed = FALSE;

	g_return_val_if_fail(UBER_IS_GRAPH(graph), FALSE);
	g_return_val_if_fail(info != NULL, FALSE);

	ENTRY;
	priv = graph->priv;
	GET_PIXEL_RANGE(pixel_range, priv->content_rect);
	uber_buffer_append(info->buffer, value);
	if (value != -INFINITY) {
		if (priv->yautoscale) {
			if (value > priv->yrange.end) {
				priv->yrange.end = value + ABS((SCALE_FACTOR - 1.) * value);
				priv->yrange.range = priv->yrange.end - priv->yrange.begin;
				scale_changed = TRUE;
			} else if (value < priv->yrange.begin) {
				priv->yrange.begin = value - ABS((SCALE_FACTOR - 1.) * value);
				priv->yrange.range = priv->yrange.end - priv->yrange.begin;
				scale_changed = TRUE;
			}
		}
		if (!priv->scale(graph, &priv->yrange, &pixel_range, &value)) {
			value = -INFINITY;
		}
	}
	uber_buffer_append(info->scaled, value);
	RETURN(scale_changed);
}

/**
 * uber_graph_set_stride:
 * @graph: A UberGraph.
 *
 * Sets the number of x-axis points allowed in the circular buffer.
 *
 * Returns: None.
 * Side effects: None.
 */
void
uber_graph_set_stride (UberGraph *graph, /* IN */
                       gint       stride) /* IN */
{
	UberGraphPrivate *priv;
	LineInfo *line;
	gint i;

	g_return_if_fail(UBER_IS_GRAPH(graph));
	g_return_if_fail(stride > 0);

	ENTRY;
	priv = graph->priv;
	priv->stride = stride;
	for (i = 0; i < priv->lines->len; i++) {
		line = &g_array_index(priv->lines, LineInfo, i);
		uber_buffer_set_size(line->buffer, stride);
		uber_buffer_set_size(line->scaled, stride);
	}
	uber_graph_init_graph_info(graph, &priv->info[0]);
	uber_graph_init_graph_info(graph, &priv->info[1]);
	uber_graph_calculate_rects(graph);
	EXIT;
}

/**
 * uber_graph_set_yrange:
 * @graph: A UberGraph.
 * @range: An #UberRange.
 *
 * Sets the vertical range the graph should contain.
 *
 * Returns: None.
 * Side effects: None.
 */
void
uber_graph_set_yrange (UberGraph       *graph,  /* IN */
                       const UberRange *yrange) /* IN */
{
	UberGraphPrivate *priv;

	g_return_if_fail(UBER_IS_GRAPH(graph));

	ENTRY;
	priv = graph->priv;
	priv->yrange = *yrange;
	if (priv->yrange.range == 0.) {
		priv->yrange.range = priv->yrange.end - priv->yrange.begin;
	}
	gtk_widget_queue_draw(GTK_WIDGET(graph));
	EXIT;
}

/**
 * uber_graph_extend_range:
 * @graph: A #UberGraph.
 *
 * Callback for determining the range of an #UberBuffer.
 *
 * Returns: always %FALSE to continue iteration.
 * Side effects: None.
 */
static inline gboolean
uber_graph_extend_range (UberBuffer *buffer, /* IN */
                         gdouble     value,  /* IN */
                         UberRange  *range)  /* IN */
{
	g_return_if_fail(buffer != NULL);
	g_return_if_fail(range != NULL);

	range->begin = MIN(range->begin, value);
	range->end = MAX(range->end, value);
	return FALSE;
}

/**
 * uber_graph_downscale_timeout:
 * @data: An #UberGraph.
 *
 * Timeout handler called when we need to recalculate if we can shrink
 * the range of the graph.  If the graph y-axis range can be shrunk,
 * the the graph contents are marked dirty and re-rendered.
 *
 * Returns: %TRUE always to keep the timeout continuing.
 * Side effects: None.
 */
static gboolean
uber_graph_downscale_timeout (gpointer data) /* IN */
{
	UberGraph *graph = data;
	UberGraphPrivate *priv;
	UberRange range = { 0 };
	LineInfo *line;
	gint i;

	g_return_if_fail(UBER_IS_GRAPH(graph));

	ENTRY;
	priv = graph->priv;
	for (i = 0; i < priv->lines->len; i++) {
		line = &g_array_index(priv->lines, LineInfo, i);
		uber_buffer_foreach(line->buffer, uber_graph_extend_range, &range);
	}
	if (range.begin == range.end) {
		RETURN(TRUE);
	}
	if ((range.end * SCALE_FACTOR) < priv->yrange.end) {
		priv->yrange.end = range.end * SCALE_FACTOR;
		priv->yrange.range = priv->yrange.end - priv->yrange.begin;
		uber_graph_scale_changed(graph);
	}
	/* TODO: Scale yrange.begin */
	RETURN(TRUE);
}

/**
 * uber_graph_set_yautoscale:
 * @graph: A UberGraph.
 * @yautoscale: If y-axis should autoscale to handle values outside of
 *    its current range.
 *
 * Sets the graph to autoscale to handle the current range.  If @yautoscale
 * is %TRUE, new values outside the current y range will cause the range to
 * grow and the graph redrawn to match the new scale.
 *
 * In the future, the graph may compact the scale when the larger values
 * have moved off the graph.  However, this has not yet been implemented.
 *
 * Returns: None.
 * Side effects: None.
 */
void
uber_graph_set_yautoscale (UberGraph *graph,      /* IN */
                           gboolean   yautoscale) /* IN */
{
	UberGraphPrivate *priv;

	g_return_if_fail(UBER_IS_GRAPH(graph));

	ENTRY;
	priv = graph->priv;
	priv->yautoscale = yautoscale;
	if (priv->down_handler) {
		g_source_remove(priv->down_handler);
	}
	if (yautoscale) {
		priv->down_handler =
			g_timeout_add_seconds(5, uber_graph_downscale_timeout, graph);
	}
	EXIT;
}

/**
 * uber_graph_get_yautoscale:
 * @graph: A UberGraph.
 *
 * Retrieves if the graph is set to autoscale the y-axis.
 *
 * Returns: None.
 * Side effects: None.
 */
gboolean
uber_graph_get_yautoscale (UberGraph *graph) /* IN */
{
	g_return_val_if_fail(UBER_IS_GRAPH(graph), FALSE);

	ENTRY;
	RETURN(graph->priv->yautoscale);
}

/**
 * uber_graph_fps_timeout:
 * @graph: A #UberGraph.
 *
 * GSourceFunc that is called when the amount of time has passed between each
 * frame that needs to be rendered.
 *
 * Returns: %TRUE always.
 * Side effects: None.
 */
static gboolean
uber_graph_fps_timeout (gpointer data) /* IN */
{
	UberGraphPrivate *priv;
	UberGraph *graph = data;
	LineInfo *info;
	GdkWindow *window;
	gdouble value;
	gboolean scale_changed = FALSE;
	gint i;

	g_return_val_if_fail(UBER_IS_GRAPH(graph), FALSE);

	priv = graph->priv;
	/*
	 * Retrieve the next value for the graph if necessary.
	 */
	if (G_UNLIKELY(priv->fps_off >= priv->fps)) {
		for (i = 0; i < priv->lines->len; i++) {
			info = &g_array_index(priv->lines, LineInfo, i);
			uber_graph_get_next_value(graph, i + 1, info, &value);
			if (uber_graph_append(graph, info, value)) {
				scale_changed = TRUE;
			}
		}
		if (scale_changed) {
			uber_graph_scale_changed(graph);
		} else {
			uber_graph_render_fg_shifted_task(graph,
											  &priv->info[priv->flipped],
											  &priv->info[!priv->flipped]);
			priv->flipped = !priv->flipped;
		}
		priv->fps_off = 0;
	}
	/*
	 * Update the content area.
	 */
	window = gtk_widget_get_window(GTK_WIDGET(graph));
	gdk_window_invalidate_rect(window, &graph->priv->content_rect, FALSE);
	return TRUE;
}

/**
 * uber_graph_set_fps:
 * @graph: A UberGraph.
 * @fps: The number of frames-per-second.
 *
 * Sets the frames-per-second which should be rendered by UberGraph.
 *
 * Returns: None.
 * Side effects: None.
 */
void
uber_graph_set_fps (UberGraph *graph, /* IN */
                    gint       fps)   /* IN */
{
	UberGraphPrivate *priv;

	g_return_if_fail(UBER_IS_GRAPH(graph));
	g_return_if_fail(fps > 0 && fps <= 60);

	ENTRY;
	priv = graph->priv;
	priv->fps = fps;
	priv->fps_to = 1000. / fps;
	priv->fps_each = (gfloat)priv->content_rect.width /
	                 (gfloat)priv->stride /
	                 (gfloat)priv->fps;
	if (priv->fps_handler) {
		g_source_remove(priv->fps_handler);
	}
	priv->fps_handler = g_timeout_add(priv->fps_to,
	                                  uber_graph_fps_timeout,
	                                  graph);
	EXIT;
}

/**
 * uber_graph_prepare_layout:
 * @graph: A #UberGraph.
 * @layout: A #PangoLayout.
 * @mode: The layout mode.
 *
 * Prepares the #PangoLayout with the settings required to render the given
 * mode, such as LAYOUT_TICK.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
uber_graph_prepare_layout (UberGraph   *graph,  /* IN */
                           PangoLayout *layout, /* IN */
                           gint         mode)   /* IN */
{
	UberGraphPrivate *priv;
	PangoFontDescription *desc;

	ENTRY;
	priv = graph->priv;
	desc = pango_font_description_new();
	switch (mode) {
	case LAYOUT_TICK:
		pango_font_description_set_family(desc, "MONOSPACE");
		pango_font_description_set_size(desc, 8 * PANGO_SCALE);
		break;
	default:
		g_assert_not_reached();
	}
	pango_layout_set_font_description(layout, desc);
	pango_font_description_free(desc);
	EXIT;
}

/**
 * pango_layout_get_pixel_rectangle:
 * @layout; A PangoLayout.
 * @rect: A GdkRectangle.
 *
 * Helper to retrieve the area of a layout using a GdkRectangle.
 *
 * Returns: None.
 * Side effects: None.
 */
static inline void
pango_layout_get_pixel_rectangle (PangoLayout  *layout, /* IN */
                                  GdkRectangle *rect)   /* IN */
{
	g_return_if_fail(PANGO_IS_LAYOUT(layout));
	g_return_if_fail(rect != NULL);

	ENTRY;
	rect->x = 0;
	rect->y = 0;
	pango_layout_get_pixel_size(layout, &rect->width, &rect->height);
	EXIT;
}

/**
 * uber_graph_calculate_rects:
 * @graph: A #UberGraph.
 *
 * Calculates the locations of various features within the graph.  The various
 * rendering methods use these calculations quickly place items in the correct
 * location.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
uber_graph_calculate_rects (UberGraph *graph) /* IN */

{
	UberGraphPrivate *priv;
	GtkAllocation alloc;
	GdkWindow *window;
	PangoLayout *pl;
	cairo_t *cr;
	gint tick_w;
	gint tick_h;

	g_return_if_fail(UBER_IS_GRAPH(graph));

	ENTRY;
	priv = graph->priv;
	if (!(window = gtk_widget_get_window(GTK_WIDGET(graph)))) {
		return;
	}
	gtk_widget_get_allocation(GTK_WIDGET(graph), &alloc);
	/*
	 * Create a cairo context and PangoLayout to calculate the sizing
	 * of various strings.
	 */
	cr = gdk_cairo_create(GDK_DRAWABLE(window));
	pl = pango_cairo_create_layout(cr);
	/*
	 * Determine largest size of tick labels.
	 */
	uber_graph_prepare_layout(graph, pl, LAYOUT_TICK);
	pango_layout_set_text(pl, "XXXXXXX", -1);
	pango_layout_get_pixel_size(pl, &tick_w, &tick_h);
	/*
	 * Calculate the X-Axis tick area.
	 */
	priv->x_tick_rect.height = priv->tick_len + tick_h;
	priv->x_tick_rect.x = priv->tick_len + tick_w;
	priv->x_tick_rect.width = alloc.width - priv->x_tick_rect.x;
	priv->x_tick_rect.y = alloc.height - priv->x_tick_rect.height;
	/*
	 * Calculate the Y-Axis tick area.
	 */
	priv->y_tick_rect.x = 0;
	priv->y_tick_rect.y = tick_h / 2;
	priv->y_tick_rect.width = tick_w + priv->tick_len;
	priv->y_tick_rect.height = priv->x_tick_rect.y - priv->y_tick_rect.y;
	/*
	 * Calculate the content region.
	 */
	priv->content_rect.x = priv->y_tick_rect.x + priv->y_tick_rect.width + 1;
	priv->content_rect.y = tick_h / 2 + 1;
	priv->content_rect.width = alloc.width - priv->content_rect.x - 2;
	priv->content_rect.height = priv->x_tick_rect.y - priv->content_rect.y - 2;
	gdk_gc_set_clip_rectangle(priv->fg_gc, &priv->content_rect);
	/*
	 * Cleanup after allocations.
	 */
	g_object_unref(pl);
	cairo_destroy(cr);
	EXIT;
}

/**
 * uber_graph_render_bg_x_ticks:
 * @graph: A #UberGraph.
 * @info: A GraphInfo.
 *
 * Draws the ticks and labels for the Y-Axis grid lines of the graph.
 *
 * Returns: None.
 * Side effects: None.
 */
static inline void
uber_graph_render_bg_x_ticks (UberGraph *graph, /* IN */
                              GraphInfo *info)  /* IN */
{
	UberGraphPrivate *priv;
	gfloat fraction;
	gint n_lines;
	gint i;
	gint w;
	gint h;

	g_return_if_fail(UBER_IS_GRAPH(graph));

	ENTRY;
	priv = graph->priv;

	#define DRAW_TICK_LABEL(v, o)                                            \
	    G_STMT_START {                                                       \
	        gint _v = (v);                                                   \
	        gchar *_v_str;                                                   \
	        _v_str = g_markup_printf_escaped(                                \
	            "<span size='smaller'>%d</span>",                            \
	            _v);                                                         \
	        pango_layout_set_markup(info->tick_layout, _v_str, -1);          \
	        pango_layout_get_pixel_size(info->tick_layout, &w, &h);          \
	        if (o == 0) { \
	             cairo_move_to(info->bg_cairo, \
	                           priv->content_rect.x + priv->content_rect.width - w, \
	                           priv->content_rect.y + priv->content_rect.height + priv->tick_len + 5); \
	        } else { \
	             cairo_move_to(info->bg_cairo,                                    \
		                       priv->content_rect.x + priv->content_rect.width - (int)(o * (priv->x_tick_rect.width / (gfloat)n_lines)) + .5 - (w / 2), \
	                           priv->content_rect.y + priv->content_rect.height + priv->tick_len + 5); \
	        } \
	        pango_cairo_show_layout(info->bg_cairo, info->tick_layout);      \
	        g_free(_v_str);                                                  \
		} G_STMT_END

	n_lines = priv->stride / 10;
	DRAW_TICK_LABEL(0, 0);
	for (i = 1; i < n_lines; i++) {
		cairo_move_to(info->bg_cairo,
		              priv->content_rect.x + (int)(i * (priv->x_tick_rect.width / (gfloat)n_lines)) + .5,
		              priv->content_rect.y);
		cairo_line_to(info->bg_cairo,
		              priv->content_rect.x + (int)(i * (priv->x_tick_rect.width / (gfloat)n_lines)) + .5,
		              priv->x_tick_rect.y + priv->tick_len);
		cairo_stroke(info->bg_cairo);
		fraction = (1. / (gfloat)n_lines) * priv->stride;
		DRAW_TICK_LABEL(fraction * i, i);
	}
	DRAW_TICK_LABEL(priv->stride, n_lines);
	EXIT;
	#undef DRAW_TICK_LABEL
}

/**
 * uber_graph_get_ylabel_at_pos:
 * @graph: A #UberGraph.
 * @y: the pixel where the line will be placed.
 * @lineno: This is line X of @lines.
 * @lines: total number of lines to be drawn.
 *
 * Formats the value at pixel offset @y based on the current format operation.
 *
 * Returns: A string containing the label.  This value should be freed with
 *   g_free().
 * Side effects: None.
 */
static gchar*
uber_graph_get_ylabel_at_pos (UberGraph *graph,  /* IN */
                              gdouble    y,      /* IN */
                              gint       lineno, /* IN */
                              gint       lines)  /* IN */
{
	UberGraphPrivate *priv;
	UberRange range;
	const gchar *a = "";
	gchar *ret = NULL;
	gfloat f;

	g_return_val_if_fail(UBER_IS_GRAPH(graph), NULL);

	ENTRY;
	priv = graph->priv;
	GET_PIXEL_RANGE(range, priv->content_rect);
	switch (priv->format) {
	CASE(UBER_GRAPH_DIRECT); {
		f = (gfloat)(range.end - y) / (gfloat)range.range * (gfloat)priv->yrange.range;
		if ( f >= 1000000000 || f <= -1000000000) {
			f /= 1000000000;
			a = "g ";
		} else if (f >= 1000000 || f <= -1000000) {
			f /= 1000000;
			a = "m ";
		} else if (f >= 1000 || f <= -1000) {
			f /= 1000;
			a = "k ";
		}
		ret = g_markup_printf_escaped("<span size='smaller'>%.1f %s</span>", f, a);
		break;
	}
	CASE(UBER_GRAPH_DIRECT1024); {
		f = (gfloat)(range.end - y) / (gfloat)range.range * (gfloat)priv->yrange.range;
		if (f >= GIBIBYTE || f <= -GIBIBYTE) {
			f /= GIBIBYTE;
			a = GIBIBYTE_STR;
		} else if (f >= MEBIBYTE || f <= -MEBIBYTE) {
			f /= MEBIBYTE;
			a = MEBIBYTE_STR;
		} else if (f >= KIBIBYTE || f <= -KIBIBYTE) {
			f /= KIBIBYTE;
			a = KIBIBYTE_STR;
		}
		ret = g_markup_printf_escaped("<span size='smaller'>%.1f %s</span>", f, a);
		break;
	}
	CASE(UBER_GRAPH_PERCENT); {
		f = (gfloat)(lines - lineno) / (gfloat)lines * 100.;
		ret = g_markup_printf_escaped("<span size='smaller'>%d %% </span>", (gint)f);
		break;
	}
	default:
		g_assert_not_reached();
	}
	RETURN(ret);
}

/**
 * uber_graph_render_bg_y_ticks:
 * @graph: A #UberGraph.
 * @info: A GraphInfo.
 *
 * Draws the ticks and labels for the Y-Axis grid lines of the graph.
 *
 * Returns: None.
 * Side effects: None.
 */
static inline void
uber_graph_render_bg_y_ticks (UberGraph *graph, /* IN */
                              GraphInfo *info)  /* IN */
{
	UberGraphPrivate *priv;
	UberRange range;
	gfloat y;
	gint n_lines;
	gint i;

	g_return_if_fail(UBER_IS_GRAPH(graph));

	ENTRY;
	priv = graph->priv;
	GET_PIXEL_RANGE(range, priv->content_rect);

	#define DRAW_Y_LABEL(y, i, n)                                        \
	    G_STMT_START {                                                   \
	        gchar *v = uber_graph_get_ylabel_at_pos(graph, y, i, n);     \
	        gint h, w;                                                   \
	        pango_layout_set_markup(info->tick_layout, v, -1);           \
	        pango_layout_get_pixel_size(info->tick_layout, &w, &h);      \
	        cairo_move_to(info->bg_cairo,                                \
	                      priv->content_rect.x - priv->tick_len - w,     \
			              ((gint)y) - (h / 2) + .5);                     \
			pango_cairo_show_layout(info->bg_cairo, info->tick_layout);  \
			g_free(v);                                                   \
	    } G_STMT_END

	n_lines = MIN(priv->content_rect.height / 20, 5);
	DRAW_Y_LABEL(range.begin, 0, n_lines);
	for (i = 1; i < n_lines; i++) {
		y = priv->y_tick_rect.y + (priv->y_tick_rect.height / n_lines * i);
		cairo_move_to(info->bg_cairo,
		              priv->content_rect.x - priv->tick_len,
		              y + .5);
		cairo_line_to(info->bg_cairo,
		              priv->content_rect.x + priv->content_rect.width,
		              y + .5);
		cairo_stroke(info->bg_cairo);
		DRAW_Y_LABEL(y, i, n_lines);
	}
	DRAW_Y_LABEL(range.end, n_lines, n_lines);
	EXIT;
	#undef DRAW_Y_LABEL
}

/**
 * uber_graph_render_bg_task:
 * @graph: A #UberGraph.
 * @info: A GraphInfo.
 *
 * Handles rendering the background to the server-side pixmap.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
uber_graph_render_bg_task (UberGraph *graph, /* IN */
                           GraphInfo *info)  /* IN */
{
	static const gdouble dashes[] = { 1., 2. };
	UberGraphPrivate *priv;
	GtkAllocation alloc;
	GdkColor bg_color;
	GdkColor white;

	g_return_if_fail(UBER_IS_GRAPH(graph));
	g_return_if_fail(info != NULL);

	ENTRY;
	priv = graph->priv;
	cairo_save(info->bg_cairo);
	/*
	 * Retrieve required data for rendering.
	 */
	gtk_widget_get_allocation(GTK_WIDGET(graph), &alloc);
	bg_color = gtk_widget_get_style(GTK_WIDGET(graph))->bg[GTK_STATE_NORMAL];
	gdk_color_parse("#fff", &white);
	/*
	 * Clear the background to default widget background color.
	 */
	cairo_rectangle(info->bg_cairo, 0, 0, alloc.width, alloc.height);
	gdk_cairo_set_source_color(info->bg_cairo, &bg_color);
	cairo_fill(info->bg_cairo);
	/*
	 * Fill in the content rectangle and stroke edge.
	 */
	gdk_cairo_rectangle_clean(info->bg_cairo, &priv->content_rect);
	cairo_set_source_rgb(info->bg_cairo, 1, 1, 1);
	cairo_fill_preserve(info->bg_cairo);
	cairo_set_dash(info->bg_cairo, dashes, G_N_ELEMENTS(dashes), 0);
	cairo_set_line_width(info->bg_cairo, 1.0);
	cairo_set_source_rgb(info->bg_cairo, 0, 0, 0);
	cairo_stroke(info->bg_cairo);
	/*
	 * Render the X-Axis ticks.
	 */
	uber_graph_render_bg_x_ticks(graph, info);
	/*
	 * Render the Y-Axis ticks.
	 */
	uber_graph_render_bg_y_ticks(graph, info);
	/*
	 * Cleanup.
	 */
	cairo_restore(info->bg_cairo);
	EXIT;
}

/**
 * uber_graph_render_fg_each:
 * @graph: A #UberGraph.
 * @value: The translated value in graph coordinates.
 * @user_data: A RenderClosure.
 *
 * Callback for each data point in the buffer.  Renders the value to the
 * foreground pixmap on the server-side.
 *
 * Returns: %FALSE always.
 * Side effects: None.
 */
static inline gboolean
uber_graph_render_fg_each (UberBuffer *buffer,    /* IN */
                           gdouble     value,     /* IN */
                           gpointer    user_data) /* IN */
{
	UberGraphPrivate *priv;
	RenderClosure *closure = user_data;
	gdouble y;
	gdouble x;

	g_return_val_if_fail(closure->graph != NULL, FALSE);

	priv = closure->graph->priv;
	x = closure->x_epoch - (closure->offset++ * priv->x_each);
	if (isnan(value) || isinf(value)) {
		goto skip;
	}
	y = closure->pixel_range.end - value;
	if (G_UNLIKELY(closure->first)) {
		closure->first = FALSE;
		cairo_move_to(closure->info->fg_cairo, x, y);
		goto finish;
	}
	cairo_curve_to(closure->info->fg_cairo,
	               closure->last_x - (priv->x_each / 2.),
	               closure->last_y,
	               closure->last_x - (priv->x_each / 2.),
	               y, x, y);
  skip:
  finish:
  	closure->last_x = x;
  	closure->last_y = y;
	return FALSE;
}

/**
 * gdk_color_parse_xor:
 * @color: A #GdkColor.
 * @spec: A color string ("#fff, #000000").
 * @xor: The value to xor against.
 *
 * Parses a color spec and xor's it against @xor.  The value is stored
 * in @color.
 *
 * Returns: %TRUE if successful; otherwise %FALSE.
 * Side effects: None.
 */
static inline gboolean
gdk_color_parse_xor (GdkColor    *color, /* IN */
                     const gchar *spec,  /* IN */
                     gint         xor)   /* IN */
{
	gboolean ret;

	if ((ret = gdk_color_parse(spec, color))) {
		color->red   ^= xor;
		color->green ^= xor;
		color->blue  ^= xor;
	}
	return ret;
}

/**
 * uber_graph_stylize_line:
 * @graph: A #UberGraph.
 * @line: A LineInfo.
 * @cr: A cairo context.
 *
 * Stylizes the cairo context according to the lines settings.
 *
 * Returns: None.
 * Side effects: None.
 */
static inline void
uber_graph_stylize_line (UberGraph *graph, /* IN */
                         LineInfo  *line,  /* IN */
                         cairo_t   *cr)    /* IN */
{
	g_return_if_fail(UBER_IS_GRAPH(graph));
	g_return_if_fail(line != NULL);
	g_return_if_fail(cr != NULL);

	cairo_set_line_width(cr, 2.0);
	gdk_cairo_set_source_color(cr, &line->color);
	cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);
	cairo_set_line_join(cr, CAIRO_LINE_JOIN_ROUND);
}

/**
 * uber_graph_render_fg_task:
 * @graph: A #UberGraph.
 * @info: A GraphInfo.
 *
 * XXX
 *
 * Returns: None.
 * Side effects: None.
 */
static void
uber_graph_render_fg_task (UberGraph *graph, /* IN */
                           GraphInfo *info)  /* IN */
{
	UberGraphPrivate *priv;
	GtkAllocation alloc;
	RenderClosure closure = { 0 };
	LineInfo *line;
	gint i;

	g_return_if_fail(UBER_IS_GRAPH(graph));
	g_return_if_fail(info != NULL);

	ENTRY;
	priv = graph->priv;
	gtk_widget_get_allocation(GTK_WIDGET(graph), &alloc);
	/*
	 * Prepare graph closure.
	 */
	closure.graph = graph;
	closure.info = info;
	closure.scale = priv->scale;
	GET_PIXEL_RANGE(closure.pixel_range, priv->content_rect);
	closure.value_range = priv->yrange;
	closure.x_epoch = priv->content_rect.x + priv->content_rect.width + priv->x_each;
	/*
	 * Clear the background.
	 */
	cairo_save(info->fg_cairo);
	cairo_set_operator(info->fg_cairo, CAIRO_OPERATOR_CLEAR);
	cairo_rectangle(info->fg_cairo, 0, 0, alloc.width + priv->x_each, alloc.height);
	cairo_fill(info->fg_cairo);
	cairo_restore(info->fg_cairo);
	/*
	 * Render data point contents.
	 */
	cairo_save(info->fg_cairo);
	cairo_rectangle(info->fg_cairo,
	                priv->content_rect.x,
	                priv->content_rect.y,
	                priv->content_rect.width + priv->x_each,
	                priv->content_rect.height);
	cairo_clip(info->fg_cairo);
	for (i = 0; i < priv->lines->len; i++) {
		line = &g_array_index(priv->lines, LineInfo, i);
		closure.last_x = -INFINITY;
		closure.last_y = -INFINITY;
		closure.first = TRUE;
		closure.offset = 0;
		cairo_move_to(info->fg_cairo,
		              closure.x_epoch,
		              priv->content_rect.y + priv->content_rect.height - 1);
		uber_graph_stylize_line(graph, line, info->fg_cairo);
		uber_buffer_foreach(line->scaled, uber_graph_render_fg_each, &closure);
		cairo_stroke(info->fg_cairo);
	}
	cairo_restore(info->fg_cairo);
	priv->fg_dirty = FALSE;
	priv->fps_off++;
	EXIT;
}

/**
 * uber_graph_render_fg_shifted_task:
 * @graph: A #UberGraph.
 * @src: A GraphInfo with contents to copy.
 * @dst: A GraphInfo to copy shifted src contents to.
 *
 * Renders a portion of @src pixmap to @dst pixmap at the shifting
 * rate.  It is assumed that the most recent value in the circular buffer
 * is the value that needs to be rendered and added to the pixmap.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
uber_graph_render_fg_shifted_task (UberGraph    *graph,  /* IN */
                                   GraphInfo    *src,    /* IN */
                                   GraphInfo    *dst)    /* IN */
{
	UberGraphPrivate *priv;
	LineInfo *line;
	GtkAllocation alloc;
	gdouble last_y;
	gdouble x_epoch;
	gdouble y_end;
	gdouble y;
	gint i;

	g_return_if_fail(UBER_IS_GRAPH(graph));
	g_return_if_fail(src != NULL);
	g_return_if_fail(dst != NULL);

	ENTRY;
	priv = graph->priv;
	/*
	 * Clear the old pixmap contents.
	 */
	cairo_save(dst->fg_cairo);
	cairo_set_operator(dst->fg_cairo, CAIRO_OPERATOR_CLEAR);
	cairo_set_source_rgb(dst->fg_cairo, 1, 1, 1);
	cairo_rectangle(dst->fg_cairo, 0, 0, alloc.width, alloc.height);
	cairo_paint(dst->fg_cairo);
	cairo_restore(dst->fg_cairo);
	/*
	 * Shift contents of source onto destination pixmap.  The unused
	 * data point is lost and contents shifted over.
	 */
	gdk_gc_set_function(priv->fg_gc, GDK_COPY);
	gdk_draw_drawable(dst->fg_pixmap, priv->fg_gc, src->fg_pixmap,
	                  priv->content_rect.x + priv->x_each,
	                  priv->content_rect.y,
	                  priv->content_rect.x,
	                  priv->content_rect.y,
	                  priv->content_rect.width,
	                  priv->content_rect.height);
	gdk_gc_set_function(priv->fg_gc, GDK_XOR);
	/*
	 * Render the lines of data.  Clip the region to the new area only.
	 */
	y_end = priv->content_rect.y + priv->content_rect.height - 1;
	x_epoch = priv->content_rect.x + priv->content_rect.width + priv->x_each;
	cairo_save(dst->fg_cairo);
	cairo_rectangle(dst->fg_cairo,
	                priv->content_rect.x + priv->content_rect.width,
	                priv->content_rect.y,
	                priv->x_each,
	                priv->content_rect.height);
	cairo_clip(dst->fg_cairo);
	for (i = 0; i < priv->lines->len; i++) {
		line = &g_array_index(priv->lines, LineInfo, i);
		y = uber_buffer_get_index(line->scaled, 0);
		last_y = uber_buffer_get_index(line->scaled, 1);
		/*
		 * Don't try to draw before we have real values.
		 */
		if ((isnan(y) || isinf(y)) || (isnan(last_y) || isinf(last_y))) {
			continue;
		}
		y = y_end - y;
		last_y = y_end - last_y;
		/*
		 * Convert relative position to fixed from bottom pixel.
		 */
		uber_graph_stylize_line(graph, line, dst->fg_cairo);
		cairo_move_to(dst->fg_cairo, x_epoch, y);
		cairo_curve_to(dst->fg_cairo,
		               x_epoch - (priv->x_each / 2.),
		               y,
		               x_epoch - (priv->x_each / 2.),
		               last_y,
		               priv->content_rect.x + priv->content_rect.width,
		               last_y);
		cairo_stroke(dst->fg_cairo);
	}
	cairo_restore(dst->fg_cairo);
	EXIT;
}

/**
 * uber_graph_init_graph_info:
 * @graph: A #UberGraph.
 * @info: A GraphInfo.
 *
 * Initializes the GraphInfo structure to match the current settings of the
 * #UberGraph.  If @info has existing server-side pixmaps, they will be scaled
 * to match the new size of the widget.
 *
 * The renderer will perform a redraw of the entire area on its next pass as
 * the contents will potentially be lossy and skewed.  But this is still far
 * better than incurring the wrath of layout rendering in the GUI thread.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
uber_graph_init_graph_info (UberGraph *graph, /* IN */
                            GraphInfo *info)  /* IN/OUT */
{
	UberGraphPrivate *priv;
	GtkAllocation alloc;
	GdkDrawable *drawable;
	GdkPixmap *bg_pixmap;
	GdkPixmap *fg_pixmap;
	GdkColor bg_color;
	cairo_t *cr;

	g_return_if_fail(UBER_IS_GRAPH(graph));
	g_return_if_fail(info != NULL);

	ENTRY;
	priv = graph->priv;
	gtk_widget_get_allocation(GTK_WIDGET(graph), &alloc);
	drawable = GDK_DRAWABLE(gtk_widget_get_window(GTK_WIDGET(graph)));
	bg_pixmap = gdk_pixmap_new(drawable, alloc.width, alloc.height, -1);
	fg_pixmap = gdk_pixmap_new(drawable, alloc.width + 30, alloc.height, -1);
	/*
	 * Set background to default widget background.
	 */
	bg_color = gtk_widget_get_style(GTK_WIDGET(graph))->bg[GTK_STATE_NORMAL];
	cr = gdk_cairo_create(GDK_DRAWABLE(bg_pixmap));
	gdk_cairo_set_source_color(cr, &bg_color);
	cairo_rectangle(cr, 0, 0, alloc.width, alloc.height);
	cairo_fill(cr);
	cairo_destroy(cr);
	/*
	 * Clear contents of foreground.
	 */
	cr = gdk_cairo_create(GDK_DRAWABLE(fg_pixmap));
	cairo_set_operator(cr, CAIRO_OPERATOR_CLEAR);
	cairo_rectangle(cr, 0, 0, alloc.width, alloc.height);
	cairo_set_source_rgb(cr, 1, 1, 1);
	cairo_paint(cr);
	cairo_destroy(cr);
	/*
	 * Cleanup after any previous cairo contexts.
	 */
	if (info->bg_cairo) {
		if (info->tick_layout) {
			g_object_unref(info->tick_layout);
		}
		cairo_destroy(info->bg_cairo);
	}
	if (info->fg_cairo) {
		cairo_destroy(info->fg_cairo);
	}
	/*
	 * If there is an existing pixmap, we will scale it to the new size
	 * so that there is data to render until the render thread has had
	 * a chance to pass over and re-render the updated content.
	 */
	if (info->bg_pixmap) {
		g_object_unref(info->bg_pixmap);
	}
	if (info->fg_pixmap) {
		g_object_unref(info->fg_pixmap);
	}
	info->bg_pixmap = bg_pixmap;
	info->fg_pixmap = fg_pixmap;
	/*
	 * Update cached cairo contexts.
	 */
	info->bg_cairo = gdk_cairo_create(GDK_DRAWABLE(info->bg_pixmap));
	info->fg_cairo = gdk_cairo_create(GDK_DRAWABLE(info->fg_pixmap));
	/*
	 * Create PangoLayouts for rendering text.
	 */
	info->tick_layout = pango_cairo_create_layout(info->bg_cairo);
	/*
	 * Update the layouts to reflect proper styling.
	 */
	uber_graph_prepare_layout(graph, info->tick_layout, LAYOUT_TICK);
	EXIT;
}

/**
 * uber_graph_destroy_graph_info:
 * @graph: A #UberGraph.
 * @info: A GraphInfo.
 *
 * Cleans up and frees resources allocated to the GraphInfo.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
uber_graph_destroy_graph_info (UberGraph *graph, /* IN */
                               GraphInfo *info)  /* IN */
{
	ENTRY;
	if (info->tick_layout) {
		g_object_unref(info->tick_layout);
	}
	if (info->bg_cairo) {
		cairo_destroy(info->bg_cairo);
	}
	if (info->fg_cairo) {
		cairo_destroy(info->fg_cairo);
	}
	if (info->bg_pixmap) {
		g_object_unref(info->bg_pixmap);
	}
	if (info->fg_pixmap) {
		g_object_unref(info->fg_pixmap);
	}
	EXIT;
}

/**
 * uber_graph_update_scaled:
 * @graph: A #UberGraph.
 *
 * Rescales all cached values using their raw value.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
uber_graph_update_scaled (UberGraph *graph) /* IN */
{
	UberGraphPrivate *priv;
	UberRange pixel_range = { 0 };
	LineInfo *line;
	gdouble value;
	gint i;
	gint j;

	ENTRY;
	priv = graph->priv;
	GET_PIXEL_RANGE(pixel_range, priv->content_rect);
	for (i = 0; i < priv->lines->len; i++) {
		line = &g_array_index(priv->lines, LineInfo, i);
		for (j = 0; j < line->buffer->len; j++) {
			if (line->buffer->buffer[j] != -INFINITY) {
				value = line->buffer->buffer[j];
				if (!priv->scale(graph, &priv->yrange, &pixel_range, &value)) {
					value = -INFINITY;
				}
				line->scaled->buffer[j] = value;
			}
		}
	}
	EXIT;
}

/**
 * uber_graph_size_allocate:
 * @widget: A GtkWidget.
 *
 * Handles the "size-allocate" event.  Pixmaps are re-initialized
 * and rendering can proceed.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
uber_graph_size_allocate (GtkWidget     *widget, /* IN */
                          GtkAllocation *alloc)  /* IN */
{
	UberGraph *graph;
	UberGraphPrivate *priv;

	g_return_if_fail(UBER_IS_GRAPH(widget));

	ENTRY;
	graph = UBER_GRAPH(widget);
	priv = graph->priv;
	BASE_CLASS->size_allocate(widget, alloc);
	/*
	 * Adjust the sizing of the blit pixmaps.
	 */
	priv->fg_dirty = TRUE;
	priv->bg_dirty = TRUE;
	uber_graph_calculate_rects(UBER_GRAPH(widget));
	uber_graph_set_fps(graph, priv->fps); /* Re-calculate */
	priv->x_each = ((gdouble)priv->content_rect.width - 2)
	             / ((gdouble)priv->stride - 2.);
	uber_graph_scale_changed(graph);
	EXIT;
}

/**
 * gdk_cairo_rectangle_clean:
 * @cr: A #cairo_t.
 * @rect: A #GdkRectangle.
 *
 * Like gdk_cairo_rectangle(), except it attempts to make sure that the
 * values are lined up according to their "half" value to make cleaner
 * lines.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
gdk_cairo_rectangle_clean (cairo_t      *cr,   /* IN */
                           GdkRectangle *rect) /* IN */
{
	gdouble x;
	gdouble y;
	gdouble w;
	gdouble h;

	g_return_if_fail(cr != NULL);
	g_return_if_fail(rect != NULL);

	x = rect->x + .5;
	y = rect->y + .5;
	w = rect->width - 1.0;
	h = rect->height - 1.0;
	cairo_rectangle(cr, x, y, w, h);
}

/**
 * uber_graph_expose_event:
 * @widget: A #UberGraph.
 * @expose: A #GdkEventExpose.
 *
 * Handles the "expose-event" for the GtkWidget.  The current server-side
 * pixmaps are blitted as necessary.
 *
 * Returns: %TRUE if handler chain should stop; otherwise %FALSE.
 * Side effects: None.
 */
static gboolean
uber_graph_expose_event (GtkWidget      *widget, /* IN */
                         GdkEventExpose *expose) /* IN */
{
	UberGraphPrivate *priv;
	GdkDrawable *dst;
	GraphInfo *info;
	GdkRectangle clip;

	g_return_val_if_fail(UBER_IS_GRAPH(widget), FALSE);
	g_return_val_if_fail(expose != NULL, FALSE);

	priv = UBER_GRAPH(widget)->priv;
	dst = expose->window;
	info = &priv->info[priv->flipped];
	/*
	 * Render the background to the pixmap again if needed.
	 */
	if (G_UNLIKELY(priv->bg_dirty)) {
		uber_graph_render_bg_task(UBER_GRAPH(widget), info);
		uber_graph_copy_background(UBER_GRAPH(widget), info,
		                           &priv->info[!priv->flipped]);
		priv->bg_dirty = FALSE;
	}
	/*
	 * Blit the background for the exposure area.
	 */
	g_assert(info->bg_pixmap);
	gdk_draw_drawable(dst, priv->bg_gc, GDK_DRAWABLE(info->bg_pixmap),
	                  expose->area.x, expose->area.y,
	                  expose->area.x, expose->area.y,
	                  expose->area.width, expose->area.height);
	/*
	 * Set the foreground clip area.
	 */
	gdk_rectangle_intersect(&priv->content_rect, &expose->area, &clip);
	gdk_gc_set_clip_rectangle(priv->fg_gc, &clip);
	/*
	 * If the foreground is dirty, we need to re-render its entire
	 * contents.
	 */
	g_assert(info->fg_pixmap);
	if (G_UNLIKELY(priv->fg_dirty)) {
		uber_graph_render_fg_task(UBER_GRAPH(widget), info);
		gdk_draw_drawable(dst, priv->fg_gc, GDK_DRAWABLE(info->fg_pixmap),
		                  priv->content_rect.x,
		                  priv->content_rect.y,
		                  priv->content_rect.x,
		                  priv->content_rect.y,
		                  priv->content_rect.width,
		                  priv->content_rect.height);
	} else {
		gdk_draw_drawable(dst, priv->fg_gc, GDK_DRAWABLE(info->fg_pixmap),
		                  priv->content_rect.x,
		                  priv->content_rect.y,
		                  priv->content_rect.x - (priv->fps_each * priv->fps_off),
		                  priv->content_rect.y,
		                  priv->content_rect.width + priv->x_each,
		                  priv->content_rect.height);
	}
	priv->fps_off++;
	return FALSE;
}

/**
 * uber_graph_style_set:
 * @widget: A GtkWidget.
 *
 * Callback upon the changing of the active GtkStyle of @widget.  The styling
 * for the various pixmaps are updated.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
uber_graph_style_set (GtkWidget *widget,     /* IN */
                      GtkStyle  *old_style)  /* IN */
{
	UberGraphPrivate *priv;

	g_return_if_fail(UBER_IS_GRAPH(widget));

	ENTRY;
	priv = UBER_GRAPH(widget)->priv;
	BASE_CLASS->style_set(widget, old_style);
	if (!gtk_widget_get_window(widget)) {
		return;
	}
	uber_graph_init_graph_info(UBER_GRAPH(widget), &priv->info[0]);
	uber_graph_init_graph_info(UBER_GRAPH(widget), &priv->info[1]);
	EXIT;
}

/**
 * uber_graph_add_line:
 * @graph: A UberGraph.
 *
 * Adds a new line to the graph.
 *
 * Returns: the line-id.
 * Side effects: None.
 */
guint
uber_graph_add_line (UberGraph *graph) /* IN */
{
	UberGraphPrivate *priv;
	LineInfo line = { 0 };

	g_return_val_if_fail(UBER_IS_GRAPH(graph), -1);

	ENTRY;
	priv = graph->priv;
	line.buffer = uber_buffer_new();
	line.scaled = uber_buffer_new();
	uber_buffer_set_size(line.buffer, priv->stride);
	uber_buffer_set_size(line.scaled, priv->stride);
	gdk_color_parse_xor(&line.color, priv->colors[priv->color], 0xFFFF);
	priv->color = (priv->color + 1) % priv->colors_len;
	g_array_append_val(priv->lines, line);
	RETURN(priv->lines->len);
}

/**
 * uber_graph_set_format:
 * @graph: A UberGraph.
 * @format: The label format.
 *
 * Sets the format for how labels should be drawn.  If @format is
 * %UBER_GRAPH_PERCENT, then the labels will be drawn using a percent
 * from 0 to 100.  If @format is %UBER_GRAPH_DIRECT, then the value
 * will be composed from its offset in the yrange.
 *
 * Returns: None.
 * Side effects: None.
 */
void
uber_graph_set_format (UberGraph       *graph, /* IN */
                       UberGraphFormat  format) /* IN */
{
	UberGraphPrivate *priv;

	g_return_if_fail(UBER_IS_GRAPH(graph));
	g_return_if_fail(format >= UBER_GRAPH_DIRECT);
	g_return_if_fail(format <= UBER_GRAPH_PERCENT);

	ENTRY;
	priv = graph->priv;
	priv->format = format;
	priv->bg_dirty = TRUE;
	gtk_widget_queue_draw(GTK_WIDGET(graph));
	EXIT;
}

/**
 * uber_graph_realize:
 * @widget: A #UberGraph.
 *
 * Handles the "realize" signal for the #UberGraph.  Required server side
 * contexts that are needed for the entirety of the widgets life are created.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
uber_graph_realize (GtkWidget *widget) /* IN */
{
	UberGraphPrivate *priv;
	GdkDrawable *dst;

	g_return_if_fail(UBER_IS_GRAPH(widget));

	ENTRY;
	BASE_CLASS->realize(widget);
	priv = UBER_GRAPH(widget)->priv;
	dst = GDK_DRAWABLE(gtk_widget_get_window(widget));
	priv->bg_gc = gdk_gc_new(dst);
	priv->fg_gc = gdk_gc_new(dst);
	gdk_gc_set_function(priv->fg_gc, GDK_XOR);
	EXIT;
}

/**
 * uber_graph_size_request:
 * @widget: A #GtkWidget.
 * @req: A #GtkRequisition.
 *
 * Handles a request for the size of the widget.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
uber_graph_size_request (GtkWidget      *widget, /* IN */
                         GtkRequisition *req)    /* OUT */
{
	g_return_if_fail(UBER_IS_GRAPH(widget));

	req->width = 150;
	req->height = 50;
}

/**
 * uber_scale_linear:
 * @graph: A #UberGraph.
 * @values: An #UberRange for the range of values.
 * @pixels: An #UberRange for the range of pixels.
 * @value: A location to the current value, which will be scaled.
 *
 * Scales the value found at @value from the source scale to the graphs
 * pixel scale.
 *
 * Returns: %TRUE if successful; otherwise %FALSE.
 * Side effects: None.
 */
gboolean
uber_scale_linear (UberGraph       *graph,  /* IN */
                   const UberRange *values, /* IN */
                   const UberRange *pixels, /* IN */
                   gdouble         *value)  /* IN/OUT */
{
	#define A (values->range)
	#define B (pixels->range)
	#define C (*value)
	if (*value != 0.) {
		*value = C * B / A;
	}
	#undef A
	#undef B
	#undef C
	return TRUE;
}

/**
 * uber_graph_finalize:
 * @object: A #UberGraph.
 *
 * Finalizer for a #UberGraph instance.  Frees any resources held by
 * the instance.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
uber_graph_finalize (GObject *object) /* IN */
{
	UberGraphPrivate *priv;
	LineInfo *line;
	gint i;

	ENTRY;
	priv = UBER_GRAPH(object)->priv;
	uber_graph_destroy_graph_info(UBER_GRAPH(object), &priv->info[0]);
	uber_graph_destroy_graph_info(UBER_GRAPH(object), &priv->info[1]);
	if (priv->fg_gc) {
		g_object_unref(priv->fg_gc);
	}
	if (priv->bg_gc) {
		g_object_unref(priv->bg_gc);
	}
	if (priv->fps_handler) {
		g_source_remove(priv->fps_handler);
	}
	if (priv->value_notify) {
		priv->value_notify(priv->value_user_data);
	}
	for (i = 0; i < priv->lines->len; i++) {
		line = &g_array_index(priv->lines, LineInfo, i);
		uber_buffer_unref(line->buffer);
		uber_buffer_unref(line->scaled);
	}
	g_array_unref(priv->lines);
	G_OBJECT_CLASS(uber_graph_parent_class)->finalize(object);
	EXIT;
}

/**
 * uber_graph_class_init:
 * @klass: A #UberGraphClass.
 *
 * Initializes the #UberGraphClass and prepares the vtable.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
uber_graph_class_init (UberGraphClass *klass) /* IN */
{
	GObjectClass *object_class;
	GtkWidgetClass *widget_class;

	ENTRY;
	/*
	 * Prepare GObjectClass.
	 */
	object_class = G_OBJECT_CLASS(klass);
	object_class->finalize = uber_graph_finalize;
	g_type_class_add_private(object_class, sizeof(UberGraphPrivate));
	/*
	 * Prepare GtkWidgetClass.
	 */
	widget_class = GTK_WIDGET_CLASS(klass);
	widget_class->expose_event = uber_graph_expose_event;
	widget_class->realize = uber_graph_realize;
	widget_class->size_allocate = uber_graph_size_allocate;
	widget_class->style_set = uber_graph_style_set;
	widget_class->size_request = uber_graph_size_request;
	EXIT;
}

/**
 * uber_graph_init:
 * @graph: A #UberGraph.
 *
 * Initializes the newly created #UberGraph instance.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
uber_graph_init (UberGraph *graph) /* IN */
{
	UberGraphPrivate *priv;

	g_return_if_fail(UBER_IS_GRAPH(graph));

	ENTRY;
	graph->priv = GET_PRIVATE(graph, UBER_TYPE_GRAPH, UberGraphPrivate);
	priv = graph->priv;
	priv->stride = 60;
	priv->tick_len = 5;
	priv->scale = uber_scale_linear;
	priv->yrange.begin = 0.;
	priv->yrange.end = 1.;
	priv->yrange.range = 1.;
	priv->format = UBER_GRAPH_DIRECT;
	priv->lines = g_array_sized_new(FALSE, TRUE, sizeof(LineInfo), 2);
	priv->colors = g_strdupv((gchar **)default_colors);
	priv->colors_len = G_N_ELEMENTS(default_colors);
	uber_graph_set_fps(graph, 20);
	EXIT;
}
