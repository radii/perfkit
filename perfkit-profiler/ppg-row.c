/* ppg-row.c
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

#include <gtk/gtk.h>

#include "ppg-instrument.h"
#include "ppg-row.h"
#include "ppg-window.h"

#define DEFAULT_HEIGHT (45.0f)
#define TO_CAIRO_RGB(g)  \
    (g).red   / 65535.0, \
    (g).green / 65535.0, \
    (g).blue  / 65535.0
#define GDK_TO_CLUTTER(g, c)         \
    G_STMT_START {                   \
        (c).alpha = 0xFF;            \
        (c).red = (g).red / 256;     \
        (c).green = (g).green / 256; \
        (c).blue = (g).blue / 256;   \
    } G_STMT_END

G_DEFINE_TYPE(PpgRow, ppg_row, CLUTTER_TYPE_GROUP)

struct _PpgRowPrivate
{
	PpgInstrument *instrument;

	PpgWindow *window;
	GtkStateType state;
	GPtrArray *rows;
	GtkStyle *style;

	ClutterActor *hbox;
	ClutterActor *header_bg;
	ClutterActor *header_text;
	ClutterActor *data_bg;
	ClutterActor *rows_box;

	ClutterLayoutManager *hbox_layout;
	ClutterLayoutManager *box_layout;
};

enum
{
	PROP_0,
	PROP_INSTRUMENT,
	PROP_SELECTED,
	PROP_STYLE,
	PROP_TITLE,
	PROP_WINDOW,
};

static void
ppg_row_paint_header (PpgRow *row)
{
	PpgRowPrivate *priv;
	GdkColor begin;
	GdkColor end;
	cairo_pattern_t *p;
	cairo_t *cr;
	gint height;

	g_return_if_fail(PPG_IS_ROW(row));

	priv = row->priv;

	if (priv->style) {
		begin = priv->style->mid[priv->state];
		end = priv->style->dark[priv->state];
	} else {
		if (priv->state == GTK_STATE_NORMAL) {
			gdk_color_parse("#d7e866", &begin);
			gdk_color_parse("#9cb838", &end);
		} else {
			gdk_color_parse("#d7e866", &begin);
			gdk_color_parse("#b3cb49", &end);
		}
	}

	height = clutter_actor_get_height(CLUTTER_ACTOR(row));
	cr = clutter_cairo_texture_create(CLUTTER_CAIRO_TEXTURE(priv->header_bg));
	p = cairo_pattern_create_linear(0, 0, 0, height);

	cairo_rectangle(cr, 0, 0, 200.0, height);
	cairo_pattern_add_color_stop_rgb(p, 0.0, TO_CAIRO_RGB(begin));
	cairo_pattern_add_color_stop_rgb(p, 1.0, TO_CAIRO_RGB(end));
	cairo_set_source(cr, p);
	cairo_fill(cr);

	cairo_pattern_destroy(p);
	cairo_destroy(cr);
}

static gboolean
ppg_row_paint (PpgRow *row)
{
	ppg_row_paint_header(row);
	return FALSE;
}

static void
ppg_row_queue_paint (PpgRow *row)
{
	g_timeout_add(0, (GSourceFunc)ppg_row_paint, row);
}

static void
ppg_row_set_style (PpgRow   *row,
                   GtkStyle *style)
{
	PpgRowPrivate *priv;
	ClutterColor color;
	ClutterColor text;

	g_return_if_fail(PPG_IS_ROW(row));

	priv = row->priv;

	row->priv->style = style;
	GDK_TO_CLUTTER(style->bg[priv->state], color);
	GDK_TO_CLUTTER(style->text[priv->state], text);
	g_object_set(priv->data_bg, "color", &color, NULL);
	g_object_set(priv->header_text, "color", &text, NULL);
	ppg_row_queue_paint(row);

	g_object_notify(G_OBJECT(row), "style");
}

static gboolean
ppg_row_update_allocation (PpgRow *row)
{
	PpgRowPrivate *priv;
	gfloat width;

	g_return_val_if_fail(PPG_IS_ROW(row), FALSE);

	priv = row->priv;

	g_object_get(row,
	             "width", &width,
	             NULL);
	g_object_set(priv->rows_box,
	             "width", (width - 200.0f),
	             NULL);

	return FALSE;
}

static void
ppg_row_notify_allocation (PpgRow     *row,
                           GParamSpec *pspec,
                           gpointer    user_data)
{
	g_timeout_add(0, (GSourceFunc)ppg_row_update_allocation, row);
}

static gboolean
ppg_row_do_resize (PpgRow *row)
{
	PpgRowPrivate *priv;
	gfloat height;
	gfloat width;

	g_return_val_if_fail(PPG_IS_ROW(row), FALSE);

	priv = row->priv;

	g_object_get(priv->rows_box,
	             "height", &height,
	             NULL);
	g_object_get(row,
	             "width", &width,
	             NULL);

	g_object_set(priv->data_bg,
	             "height", height,
	             "width", width - 200.0f,
	             NULL);

	g_object_set(priv->rows_box,
	             "width", width - 200.0f,
	             NULL);

	g_object_set(row,
	             "height", height,
	             NULL);

	return FALSE;
}

static void
ppg_row_rows_notify_allocation (ClutterActor *actor,
                                GParamSpec   *pspec,
                                PpgRow       *row)
{
	g_timeout_add(0, (GSourceFunc)ppg_row_do_resize, row);
	ppg_row_queue_paint(row);
}

static void
ppg_row_set_selected (PpgRow   *row,
                      gboolean  selected)
{
	PpgRowPrivate *priv;

	g_return_if_fail(PPG_IS_ROW(row));

	priv = row->priv;
	priv->state = selected ? GTK_STATE_SELECTED : GTK_STATE_NORMAL;

	if (priv->style) {
		ppg_row_set_style(row, priv->style);
	}

	g_object_notify(G_OBJECT(row), "selected");
}

static void
ppg_row_set_title (PpgRow      *row,
                   const gchar *title)
{
	PpgRowPrivate *priv;

	g_return_if_fail(PPG_IS_ROW(row));
	g_return_if_fail(title != NULL);

	priv = row->priv;

	g_object_set(priv->header_text, "text", title, NULL);
	g_object_notify(G_OBJECT(row), "title");
}

static gboolean
ppg_row_show_tooltip (PpgRow *row,
                      ClutterEvent *event,
                      ClutterActor *actor)
{
	PpgRowPrivate *priv;
	PpgVisualizer *viz;
	GList *list;
	GList *iter;
	gchar *title;

	g_return_val_if_fail(PPG_IS_ROW(row), FALSE);

	priv = row->priv;

	list = ppg_instrument_get_visualizers(priv->instrument);
	for (iter = list; iter; iter = iter->next) {
		viz = iter->data;
		g_assert(PPG_IS_VISUALIZER(viz));

		if (actor == ppg_visualizer_get_actor(viz)) {
			g_object_get(viz, "title", &title, NULL);
			g_object_set(priv->window, "status-label", title, NULL);
			g_free(title);
		}
	}

	return FALSE;
}

static gboolean
ppg_row_hide_tooltip (PpgRow *row,
                      ClutterEvent *event,
                      ClutterActor *actor)
{
	PpgRowPrivate *priv;

	g_return_val_if_fail(PPG_IS_ROW(row), FALSE);

	priv = row->priv;
	g_object_set(priv->window, "status-label", "", NULL);
	return FALSE;
}

static void
ppg_row_visualizer_added (PpgRow *row,
                          PpgVisualizer *visualizer,
                          PpgInstrument *instrument)
{
	PpgRowPrivate *priv;
	ClutterActor *actor;

	g_return_if_fail(PPG_IS_ROW(row));
	g_return_if_fail(PPG_IS_VISUALIZER(visualizer));
	g_return_if_fail(PPG_IS_INSTRUMENT(instrument));

	priv = row->priv;
	g_ptr_array_add(priv->rows, visualizer);

	actor = ppg_visualizer_get_actor(visualizer);
	clutter_box_pack(CLUTTER_BOX(priv->rows_box), actor,
	                 "x-align", CLUTTER_BOX_ALIGNMENT_START,
	                 "y-align", CLUTTER_BOX_ALIGNMENT_START,
	                 "x-fill", TRUE,
	                 "y-fill", TRUE,
	                 "expand", FALSE,
	                 NULL);
	clutter_actor_set_reactive(actor, TRUE);
	g_signal_connect_swapped(actor, "enter-event",
	                         G_CALLBACK(ppg_row_show_tooltip),
	                         row);
	g_signal_connect_swapped(actor, "leave-event",
	                         G_CALLBACK(ppg_row_hide_tooltip),
	                         row);
}

static void
ppg_row_visualizer_removed (PpgRow *row,
                            PpgVisualizer *visualizer,
                            PpgInstrument *instrument)
{
	PpgRowPrivate *priv;
	ClutterActor *actor;

	g_return_if_fail(PPG_IS_ROW(row));
	g_return_if_fail(PPG_IS_VISUALIZER(visualizer));
	g_return_if_fail(PPG_IS_INSTRUMENT(instrument));

	priv = row->priv;
	g_ptr_array_remove(priv->rows, visualizer);

	actor = ppg_visualizer_get_actor(visualizer);
	clutter_container_remove(CLUTTER_CONTAINER(priv->rows_box), actor, NULL);
}

PpgInstrument*
ppg_row_get_instrument (PpgRow *row)
{
	g_return_val_if_fail(PPG_IS_ROW(row), NULL);
	return row->priv->instrument;
}

static void
ppg_row_set_instrument (PpgRow *row,
                        PpgInstrument *instrument)
{
	PpgRowPrivate *priv;
	GList *list;
	GList *iter;
	gchar *name;

	g_return_if_fail(PPG_IS_ROW(row));
	g_return_if_fail(PPG_IS_INSTRUMENT(instrument));

	priv = row->priv;
	priv->instrument = instrument;
	g_object_add_weak_pointer(G_OBJECT(priv->instrument),
	                          (gpointer *)&priv->instrument);

	/*
	 * Update row title.
	 */
	g_object_get(instrument, "name", &name, NULL);
	g_object_set(row, "title", name, NULL);
	g_free(name);

	/*
	 * Add currently added visualizers.
	 */
	list = ppg_instrument_get_visualizers(instrument);
	for (iter = list; iter; iter = iter->next) {
		ppg_row_visualizer_added(row, iter->data, instrument);
	}

	g_signal_connect_swapped(instrument, "visualizer-added",
	                         G_CALLBACK(ppg_row_visualizer_added),
	                         row);
	g_signal_connect_swapped(instrument, "visualizer-removed",
	                         G_CALLBACK(ppg_row_visualizer_removed),
	                         row);

	g_object_notify(G_OBJECT(row), "instrument");
}

static void
ppg_row_parent_set (PpgRow *row,
                    ClutterActor *old_parent,
                    gpointer user_data)
{
	PpgRowPrivate *priv;
	ClutterActor *parent;
	gfloat width;

	g_return_if_fail(PPG_IS_ROW(row));

	priv = row->priv;
	parent = clutter_actor_get_parent(CLUTTER_ACTOR(row));

	g_object_get(parent,
	             "width", &width,
	             NULL);

	g_object_set(priv->rows_box,
	             "width", width - 200.0f,
	             NULL);
}

static void
ppg_row_set_window (PpgRow *row,
                    PpgWindow *window)
{
	PpgRowPrivate *priv;

	g_return_if_fail(PPG_IS_ROW(row));

	priv = row->priv;
	priv->window = window;
}

/**
 * ppg_row_finalize:
 * @object: (in): A #PpgRow.
 *
 * Finalizer for a #PpgRow instance.  Frees any resources held by
 * the instance.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
ppg_row_finalize (GObject *object)
{
	PpgRowPrivate *priv = PPG_ROW(object)->priv;

	g_ptr_array_unref(priv->rows);
	g_object_remove_weak_pointer(G_OBJECT(priv->instrument),
	                             (gpointer *)&priv->instrument);

	G_OBJECT_CLASS(ppg_row_parent_class)->finalize(object);
}

/**
 * ppg_row_set_property:
 * @object: (in): A #GObject.
 * @prop_id: (in): The property identifier.
 * @value: (out): The given property.
 * @pspec: (in): A #ParamSpec.
 *
 * Get a given #GObject property.
 */
static void
ppg_row_get_property (GObject    *object,
                      guint       prop_id,
                      GValue     *value,
                      GParamSpec *pspec)
{
	PpgRow *row = PPG_ROW(object);

	switch (prop_id) {
	case PROP_INSTRUMENT:
		g_value_set_object(value, ppg_row_get_instrument(row));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
	}
}

/**
 * ppg_row_set_property:
 * @object: (in): A #GObject.
 * @prop_id: (in): The property identifier.
 * @value: (in): The given property.
 * @pspec: (in): A #ParamSpec.
 *
 * Set a given #GObject property.
 */
static void
ppg_row_set_property (GObject      *object,
                      guint         prop_id,
                      const GValue *value,
                      GParamSpec   *pspec)
{
	PpgRow *row = PPG_ROW(object);

	switch (prop_id) {
	case PROP_INSTRUMENT:
		ppg_row_set_instrument(row, g_value_get_object(value));
		break;
	case PROP_STYLE:
		ppg_row_set_style(row, g_value_get_object(value));
		break;
	case PROP_SELECTED:
		ppg_row_set_selected(row, g_value_get_boolean(value));
		break;
	case PROP_TITLE:
		ppg_row_set_title(row, g_value_get_string(value));
		break;
	case PROP_WINDOW:
		ppg_row_set_window(row, g_value_get_object(value));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
	}
}

/**
 * ppg_row_class_init:
 * @klass: (in): A #PpgRowClass.
 *
 * Initializes the #PpgRowClass and prepares the vtable.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
ppg_row_class_init (PpgRowClass *klass)
{
	GObjectClass *object_class;

	object_class = G_OBJECT_CLASS(klass);
	object_class->finalize = ppg_row_finalize;
	object_class->get_property = ppg_row_get_property;
	object_class->set_property = ppg_row_set_property;
	g_type_class_add_private(object_class, sizeof(PpgRowPrivate));

	g_object_class_install_property(object_class,
	                                PROP_INSTRUMENT,
	                                g_param_spec_object("instrument",
	                                                    "instrument",
	                                                    "instrument",
	                                                    PPG_TYPE_INSTRUMENT,
	                                                    G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));

	g_object_class_install_property(object_class,
	                                PROP_SELECTED,
	                                g_param_spec_boolean("selected",
	                                                     "selected",
	                                                     "selected",
	                                                     FALSE,
	                                                     G_PARAM_WRITABLE));

	g_object_class_install_property(object_class,
	                                PROP_STYLE,
	                                g_param_spec_object("style",
	                                                    "style",
	                                                    "style",
	                                                    GTK_TYPE_STYLE,
	                                                    G_PARAM_WRITABLE));

	g_object_class_install_property(object_class,
	                                PROP_TITLE,
	                                g_param_spec_string("title",
	                                                    "title",
	                                                    "title",
	                                                    NULL,
	                                                    G_PARAM_WRITABLE));

	g_object_class_install_property(object_class,
	                                PROP_WINDOW,
	                                g_param_spec_object("window",
	                                                    "window",
	                                                    "window",
	                                                    PPG_TYPE_WINDOW,
	                                                    G_PARAM_WRITABLE));
}

/**
 * ppg_row_init:
 * @row: (in): A #PpgRow.
 *
 * Initializes the newly created #PpgRow instance.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
ppg_row_init (PpgRow *row)
{
	PpgRowPrivate *priv;
	ClutterColor black;

	priv = row->priv = G_TYPE_INSTANCE_GET_PRIVATE(row, PPG_TYPE_ROW,
	                                               PpgRowPrivate);

	clutter_color_from_string(&black, "#000000");

	priv->state = GTK_STATE_NORMAL;
	priv->rows = g_ptr_array_new();

	priv->hbox_layout = g_object_new(CLUTTER_TYPE_BOX_LAYOUT,
	                                 "pack-start", FALSE,
	                                 "spacing", 0,
	                                 "use-animations", FALSE,
	                                 "vertical", FALSE,
	                                 NULL);
	priv->hbox = g_object_new(CLUTTER_TYPE_BOX,
	                          "layout-manager", priv->hbox_layout,
	                          NULL);
	priv->header_bg = g_object_new(CLUTTER_TYPE_CAIRO_TEXTURE,
	                               "height", DEFAULT_HEIGHT,
	                               "surface-width", 200,
	                               "surface-height", (gint)DEFAULT_HEIGHT,
	                               "width", 200.0f,
	                               NULL);
	clutter_box_pack(CLUTTER_BOX(priv->hbox), priv->header_bg,
	                 "x-align", CLUTTER_BOX_ALIGNMENT_START,
	                 "y-align", CLUTTER_BOX_ALIGNMENT_START,
	                 "x-fill", FALSE,
	                 "y-fill", TRUE,
	                 "expand", TRUE,
	                 NULL);
	priv->header_text = g_object_new(CLUTTER_TYPE_TEXT,
	                                 "color", &black,
	                                 "ellipsize", PANGO_ELLIPSIZE_END,
	                                 "line-wrap", FALSE,
	                                 "text", "XXX",
	                                 "width", 170.0f,
	                                 "x", 15.0f,
	                                 "y", 15.0f,
	                                 NULL);
	priv->box_layout = g_object_new(CLUTTER_TYPE_BOX_LAYOUT,
	                                "pack-start", FALSE,
	                                "spacing", 1,
	                                "use-animations", FALSE,
	                                "vertical", TRUE,
	                                NULL);
	priv->rows_box = g_object_new(CLUTTER_TYPE_BOX,
	                              "layout-manager", priv->box_layout,
	                              "x", 200.0f,
	                              NULL);
	priv->data_bg = g_object_new(CLUTTER_TYPE_RECTANGLE,
	                             "color", &black,
	                             "height", DEFAULT_HEIGHT,
	                             "width", 200.0f,
	                             "x", 200.0f,
	                             NULL);
	clutter_box_pack(CLUTTER_BOX(priv->hbox), priv->rows_box,
	                 "x-align", CLUTTER_BOX_ALIGNMENT_START,
	                 "y-align", CLUTTER_BOX_ALIGNMENT_START,
	                 "x-fill", TRUE,
	                 "y-fill", TRUE,
	                 "expand", TRUE,
	                 NULL);
	clutter_container_add(CLUTTER_CONTAINER(row),
	                      priv->data_bg,
	                      priv->hbox,
	                      priv->header_text,
	                      NULL);

	g_signal_connect_after(priv->rows_box,
	                       "notify::allocation",
	                       G_CALLBACK(ppg_row_rows_notify_allocation),
	                       row);

	g_signal_connect_after(row, "notify::allocation",
	                       G_CALLBACK(ppg_row_notify_allocation), NULL);

	g_signal_connect(row,
	                 "parent-set",
	                 G_CALLBACK(ppg_row_parent_set),
	                 NULL);

	g_object_set(row,
	             "height", DEFAULT_HEIGHT,
	             "reactive", TRUE,
	             "width", 400.0f,
	             NULL);
}
