/* pkg-session-view.c
 *
 * Copyright (C) 2010 Christian Hergert
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gdk/gdkkeysyms.h>
#include <glib/gi18n.h>
#include <clutter-gtk/clutter-gtk.h>
#include <perfkit/perfkit.h>

#include "pkg-dialog.h"
#include "pkg-panels.h"
#include "pkg-session-view.h"
#include "pkg-source-renderer.h"

#define DEFAULT_HEIGHT 40
#define ROW_IS_SELECTED(r) ((r) && ((r)->view->priv->selected == (r)))
#define ADD_COLOR_STOP(p,o,g) G_STMT_START {                      \
    cairo_pattern_add_color_stop_rgb((p), (o),                    \
                                     ((g)->red) / 65535.,         \
                                     ((g)->green) / 65535.,       \
                                     ((g)->blue) / 65535.);       \
} G_STMT_END
#define CLUTTER_COLOR_FROM_GDK(c,g) G_STMT_START {                \
    (c)->red = ((g)->red / 65535.) * 255.;                        \
    (c)->green = ((g)->green / 65535.) * 255.;                    \
    (c)->blue = ((g)->blue / 65535.) * 255.;                      \
    (c)->alpha = 0xFF;                                            \
} G_STMT_END

G_DEFINE_TYPE(PkgSessionView, pkg_session_view, GTK_TYPE_VBOX)

/**
 * SECTION:pkg-session_view
 * @title: PkgSessionView
 * @short_description: 
 *
 * 
 */

typedef struct _PkgSessionViewRow PkgSessionViewRow;

struct _PkgSessionViewPrivate
{
	PkgSession        *session;         /* Attached session */
	PkgSessionViewRow *selected;        /* Currently selected row */
	GList             *rows;            /* List of PkgSessionViewRow */
	GtkWidget         *label;           /* Label widget for the tab */
	GtkWidget         *vpaned;          /* Separator between src and detail */
	GtkWidget         *detail;          /* Detail container for source info */
	ClutterActor      *stage;           /* Toplevel clutter stage */
	ClutterActor      *row_group;       /* Container for rows */
	ClutterActor      *src_col_bg;      /* Background for sources column */
	ClutterActor      *src_col_sep;     /* Separator after sources column */
	ClutterActor      *header_bg;       /* Header background */
	ClutterActor      *header_text;     /* Header title */
	ClutterActor      *header_shadow;   /* Header shadow */
	GtkObject         *zadj;            /* Zoom adjustment */
	GtkObject         *vadj;            /* Vertical scroll adjustment */
	GtkObject         *hadj;            /* Horizontal scroll adjustment */
	GtkWidget         *ruler;
	gint               end_y;           /* Offset for end of last row */
};

struct _PkgSessionViewRow
{
	PkgSessionView *view;
	PkSource       *source;
	PkgSourceRenderer
	               *renderer;           /* Graph renderer */
	gfloat          row_ratio;          /* Size ratio compared to def. size */
	ClutterActor   *group;              /* Container for row actors */
	ClutterActor   *data_bg;            /* Background for data graph */
	ClutterActor   *data_fg;            /* Data graph content. */
	ClutterActor   *data_gloss;         /* Data graph glossy effect */
	ClutterActor   *header_bg;          /* Row header background */
	ClutterActor   *header_arrow;       /* Row header expanding arrow */
	ClutterActor   *header_text;        /* Row header title shadow */
	ClutterActor   *header_text2;       /* Row header title */
	ClutterActor   *header_info;        /* Row header info button */
	gboolean        header_info_pressed;/* Info button is pressed */
	gint            offset;
};

static void pkg_session_view_row_set_style (PkgSessionViewRow*, GtkStyle*);

/**
 * pkg_session_view_new:
 *
 * Creates a new instance of #PkgSessionView.
 *
 * Return value: the newly created #PkgSessionView instance.
 */
GtkWidget*
pkg_session_view_new (void)
{
	return g_object_new(PKG_TYPE_SESSION_VIEW, NULL);
}

/**
 * pkg_session_view_set_session:
 * @session_view: A #PkgSessionView.
 * @session: A #PkgSession.
 *
 * Sets the current session for the session view.
 *
 * Returns: None.
 * Side effects: None.
 */
void
pkg_session_view_set_session (PkgSessionView *session_view,
                              PkgSession     *session)
{
	g_return_if_fail(PKG_IS_SESSION_VIEW(session_view));
	g_return_if_fail(PKG_IS_SESSION(session));

	if (session_view->priv->session) {
		g_object_unref(session_view->priv->session);
	}
	session_view->priv->session = g_object_ref(session);
}

/**
 * pkg_session_view_get_session:
 * @session_view: A #PkgSessionView.
 *
 * Retrieves the #PkgSession attached to this view.
 *
 * Returns: A #PkgSessionView or %NULL.
 * Side effects: None.
 */
PkgSession*
pkg_session_view_get_session (PkgSessionView *session_view)
{
	g_return_val_if_fail(PKG_IS_SESSION_VIEW(session_view), NULL);
	return session_view->priv->session;
}

/**
 * pkg_session_view_get_label:
 * @session_view: A #PkgSessionView.
 *
 * Retrieves the label for the session view.
 *
 * Returns: A #GtkWidget.
 * Side effects: None.
 */
GtkWidget*
pkg_session_view_get_label (PkgSessionView *session_view)
{
	g_return_val_if_fail(PKG_IS_SESSION_VIEW(session_view), NULL);
	return session_view->priv->label;
}

static void
pkg_session_view_scroll_to_row (PkgSessionView    *session_view,
                                PkgSessionViewRow *row)
{
	PkgSessionViewPrivate *priv = session_view->priv;
	GtkAdjustment *adj = GTK_ADJUSTMENT(priv->vadj);
	gdouble value, page, height;
	gint offset;

	value = gtk_adjustment_get_value(adj);
	page = gtk_adjustment_get_page_size(adj);
	height = clutter_actor_get_height(row->group);

	if (row->offset < value){
		gtk_adjustment_set_value(adj, row->offset);
	}
	else if (row->offset > (value + page - height)){
		value = row->offset - page + height;
		gtk_adjustment_set_value(adj, value);
	}
}

static void
pkg_session_view_select_row (PkgSessionView    *session_view,
                             PkgSessionViewRow *row)
{
	PkgSessionViewRow *old_row;
	PkgSessionViewPrivate *priv;

	g_return_if_fail(PKG_IS_SESSION_VIEW(session_view));

	priv = session_view->priv;

	old_row = priv->selected;

	if (row == old_row) {
		return;
	}

	priv->selected = row;

	if (row) {
		pkg_session_view_row_set_style(row, GTK_WIDGET(session_view)->style);
		pkg_session_view_scroll_to_row(session_view, row);
	}
	if (old_row) {
		pkg_session_view_row_set_style(old_row, GTK_WIDGET(session_view)->style);
	}
}

static void
pkg_session_view_row_paint_arrow (PkgSessionViewRow *row,
                                  ClutterActor      *arrow)
{
	cairo_t *cr;

	g_return_if_fail(row != NULL);
	g_return_if_fail(row->view != NULL);

	cr = clutter_cairo_texture_create(CLUTTER_CAIRO_TEXTURE(arrow));
	gdk_cairo_set_source_color(cr, &GTK_WIDGET(row->view)->style->text[GTK_STATE_NORMAL]);
	cairo_move_to(cr, 0, 0);
	cairo_line_to(cr, 7, 5);
	cairo_line_to(cr, 0, 10);
	cairo_line_to(cr, 0, 0);
	cairo_fill(cr);
	cairo_destroy(cr);
}

static void
pkg_session_view_row_paint_header_bg (PkgSessionViewRow *row,
                                      ClutterActor      *header_bg)
{
	cairo_t *cr;
	cairo_pattern_t *p;
	gfloat height = 0, width = 0;
	gint state = GTK_STATE_NORMAL;
	GtkStyle *style;

	g_return_if_fail(row != NULL);
	g_return_if_fail(header_bg != NULL);

	if (ROW_IS_SELECTED(row)) {
		state = GTK_STATE_SELECTED;
	}

	style = GTK_WIDGET(row->view)->style;
	g_object_get(header_bg, "width", &width, "height", &height, NULL);
	cr = clutter_cairo_texture_create(CLUTTER_CAIRO_TEXTURE(header_bg));
	p = cairo_pattern_create_linear(0, 0, 0, height);
	ADD_COLOR_STOP(p, .0, &style->light[state]);
	ADD_COLOR_STOP(p, .5, &style->mid[state]);
	ADD_COLOR_STOP(p, .51, &style->dark[state]);
	ADD_COLOR_STOP(p, 1., &style->bg[state]);
	cairo_set_source(cr, p);
	cairo_rectangle(cr, 0, 0, width, height);
	cairo_fill(cr);
	cairo_pattern_destroy(p);
	cairo_destroy(cr);
}

static void
pkg_session_view_row_paint_data_fg (PkgSessionViewRow *row,
                                    ClutterActor      *data_fg)
{
	float w;

	g_return_if_fail(row != NULL);
	g_return_if_fail(data_fg != NULL);

	clutter_actor_get_size(data_fg, &w, NULL);
	pkg_source_renderer_render(row->renderer, data_fg, 0, w);
}

static void
pkg_session_view_row_paint_data_gloss (PkgSessionViewRow *row,
                                       ClutterActor      *data_gloss)
{
	cairo_t *cr;
	cairo_pattern_t *p;

	g_return_if_fail(row != NULL);
	g_return_if_fail(data_gloss != NULL);

	clutter_cairo_texture_clear(CLUTTER_CAIRO_TEXTURE(data_gloss));
	cr = clutter_cairo_texture_create(CLUTTER_CAIRO_TEXTURE(data_gloss));
	p = cairo_pattern_create_linear(0., 0., 0.,
	                                clutter_actor_get_height(data_gloss));
	cairo_pattern_add_color_stop_rgba(p, 0., 1., 1., 1., .3);
	cairo_pattern_add_color_stop_rgba(p, .618033, 1., 1., 1., .0);
	cairo_rectangle(cr, 0., 0.,
	                clutter_actor_get_width(data_gloss),
	                clutter_actor_get_height(data_gloss));
	cairo_set_source(cr, p);
	cairo_fill(cr);
	cairo_pattern_destroy(p);
	cairo_destroy(cr);
}

static void
pkg_session_view_row_paint_header_info (PkgSessionViewRow *row,
                                        ClutterActor      *info)
{
	cairo_t *cr;
	PangoLayout *pl;
	GtkWidget *widget;
	gint state = GTK_STATE_NORMAL;
	gint width, height;

	g_return_if_fail(row != NULL);
	g_return_if_fail(info != NULL);

	widget = GTK_WIDGET(row->view);
	clutter_cairo_texture_clear(CLUTTER_CAIRO_TEXTURE(info));

	if (ROW_IS_SELECTED(row)) {
		state = GTK_STATE_SELECTED;
	}

	cr = clutter_cairo_texture_create(CLUTTER_CAIRO_TEXTURE(info));
	cairo_arc(cr, 10., 10., 8., 0., 2 * G_PI);
	if (!row->header_info_pressed) {
		gdk_cairo_set_source_color(cr, &widget->style->bg[state]);
	} else {
		gdk_cairo_set_source_color(cr, &widget->style->dark[state]);
	}
	cairo_fill_preserve(cr);
	gdk_cairo_set_source_color(cr, &widget->style->dark[state]);
	cairo_stroke(cr);
	pl = pango_cairo_create_layout(cr);
	pango_layout_set_markup(pl, "<span size=\"smaller\" weight=\"bold\"><i>i</i></span>", -1);
	pango_layout_get_pixel_size(pl, &width, &height);
	cairo_move_to(cr, (20 - width) / 2, (20 - height) / 2);
	gdk_cairo_set_source_color(cr, &widget->style->text[state]);
	pango_cairo_show_layout(cr, pl);
	g_object_unref(pl);
	cairo_destroy(cr);
}

static gboolean
pkg_session_view_row_arrow_clicked (ClutterActor *arrow,
                                    ClutterEvent *event,
                                    gpointer      user_data)
{
	PkgSessionViewRow *row = user_data;
	gdouble z = 1.0;

	g_debug("Arrow clicked");

	if (!ROW_IS_SELECTED(row)) {
		pkg_session_view_select_row(row->view, row);
	}

	g_object_get(arrow, "rotation-angle-z", &z, NULL);
	z = (z != 0.) ? 0. : 90.;
	clutter_actor_animate(arrow, CLUTTER_LINEAR, 250,
	                      "rotation-angle-z", z,
	                      NULL);

	return TRUE;
}

static gboolean
pkg_session_view_row_info_released (ClutterActor *info,
                                    ClutterEvent *event,
                                    gpointer      user_data)
{
	PkgSessionViewRow *row = user_data;

	g_return_val_if_fail(row != NULL, FALSE);

	row->header_info_pressed = FALSE;
	pkg_session_view_row_paint_header_info(row, row->header_info);

	return TRUE;
}

static gboolean
pkg_session_view_row_info_clicked (ClutterActor *info,
                                   ClutterEvent *event,
                                   gpointer      user_data)
{
	PkgSessionViewRow *row = user_data;

	if (event->button.click_count == 1) {
		if (!ROW_IS_SELECTED(row)) {
			pkg_session_view_select_row(row->view, row);
		}

		g_debug("Info single clicked");

		row->header_info_pressed = TRUE;
		pkg_session_view_row_paint_header_info(row, row->header_info);
	}

	return TRUE;
}

static void
pkg_session_view_update_adjustments (PkgSessionView *session_view)
{
	PkgSessionViewPrivate *priv;
	gdouble h;

	priv = session_view->priv;

	h = clutter_actor_get_height(priv->stage);
	gtk_adjustment_set_upper(GTK_ADJUSTMENT(priv->vadj), priv->end_y);
	gtk_adjustment_set_page_size(GTK_ADJUSTMENT(priv->vadj), h);
}

static void
pkg_session_view_size_allocated (GtkWidget     *widget,
                                 GtkAllocation *alloc,
                                 gpointer       user_data)
{
	PkgSessionViewPrivate *priv;
	PkgSessionViewRow *row;
	gfloat width;
	GList *list;

	g_return_if_fail(PKG_IS_SESSION_VIEW(user_data));

	priv = PKG_SESSION_VIEW(user_data)->priv;

	for (list = priv->rows; list; list = list->next) {
		row = list->data;
		clutter_actor_set_width(row->group, alloc->width);
		clutter_actor_set_width(row->data_bg, alloc->width - 201);
		clutter_actor_set_width(row->data_fg, alloc->width - 201);
		clutter_actor_set_width(row->data_gloss, alloc->width - 201);
		clutter_cairo_texture_set_surface_size(
				CLUTTER_CAIRO_TEXTURE(row->data_gloss),
				alloc->width - 201,
				clutter_actor_get_height(row->group));
		clutter_cairo_texture_set_surface_size(
				CLUTTER_CAIRO_TEXTURE(row->data_fg),
				alloc->width - 201,
				clutter_actor_get_height(row->group));
		pkg_session_view_row_paint_data_fg(row, row->data_fg);
		pkg_session_view_row_paint_data_gloss(row, row->data_gloss);
	}

	pkg_session_view_update_adjustments(user_data);
}

static void
pkg_session_view_row_set_style (PkgSessionViewRow *row,
                                GtkStyle          *style)
{
	ClutterColor bg, text;
	gint state = GTK_STATE_NORMAL;

	g_return_if_fail(row != NULL);
	g_return_if_fail(style != NULL);

	if (ROW_IS_SELECTED(row)) {
		state = GTK_STATE_SELECTED;
	}

	CLUTTER_COLOR_FROM_GDK(&bg, &style->bg[state]);
	CLUTTER_COLOR_FROM_GDK(&text, &style->text[state]);

	pkg_session_view_row_paint_arrow(row, row->header_arrow);
	pkg_session_view_row_paint_header_bg(row, row->header_bg);
	pkg_session_view_row_paint_header_info(row, row->header_info);
	pkg_session_view_row_paint_data_gloss(row, row->data_gloss);
	g_object_set(row->data_bg, "color", &bg, NULL);
	g_object_set(row->header_text, "color", &bg, NULL);
	g_object_set(row->header_text2, "color", &text, NULL);
}

static gboolean
pkg_session_view_row_group_clicked (ClutterActor      *group,
                                    ClutterEvent      *event,
                                    PkgSessionViewRow *row)
{
	PkgSessionView *view;

	g_return_val_if_fail(group != NULL, FALSE);
	g_return_val_if_fail(row != NULL, FALSE);
	g_return_val_if_fail(row->group == group, FALSE);

	view = row->view;

	/*
	 * Unselect the selected row if it is clicked while holding
	 * Control.
	 */
	if ((event->button.modifier_state & CLUTTER_CONTROL_MASK) != 0) {
		if (ROW_IS_SELECTED(row)) {
			row = NULL;
		}
	}

	pkg_session_view_select_row(view, row);
}

static PkgSessionViewRow*
pkg_session_view_row_new (PkgSessionView *session_view,
                          PkSource       *source)
{
	PkgSessionViewPrivate *priv;
	PkgSessionViewRow *row;
	gdouble ratio;
	gdouble row_height;
	gchar *title;
	gint height;

	g_return_val_if_fail(PKG_IS_SESSION_VIEW(session_view), NULL);
	g_return_val_if_fail(PK_IS_SOURCE(source), NULL);

	priv = session_view->priv;
	ratio = gtk_adjustment_get_value(GTK_ADJUSTMENT(priv->zadj));
	row_height = DEFAULT_HEIGHT;
	title = g_strdup_printf("<span weight=\"bold\">%s</span>",
	                        g_type_name(G_TYPE_FROM_INSTANCE(source)));

	/*
	 * Create state and actors.
	 */
	row = g_slice_new0(PkgSessionViewRow);
	row->source = source ? g_object_ref(source) : NULL;
	row->row_ratio = 1.0;
	row->renderer = pkg_source_renderer_new();
	row->group = clutter_group_new();
	row->data_bg = clutter_rectangle_new();
	row->data_fg = clutter_cairo_texture_new(1, 1);
	row->data_gloss = clutter_cairo_texture_new(1, 1);
	row->header_bg = clutter_cairo_texture_new(200, row_height);
	row->header_arrow = clutter_cairo_texture_new(10, 10);
	row->header_text = clutter_text_new();
	row->header_text2 = clutter_text_new();
	row->header_info = clutter_cairo_texture_new(20, 20);
	row->view = session_view;
	g_object_add_weak_pointer(G_OBJECT(row->view), (gpointer *)&row->view);

	/*
	 * Setup row group.
	 */
	clutter_actor_set_size(row->group, 480, row_height);
	clutter_actor_set_position(row->group, 0, 0);
	clutter_actor_set_reactive(row->group, TRUE);
	g_signal_connect(row->group, "button-press-event",
	                 G_CALLBACK(pkg_session_view_row_group_clicked), row);

	/*
	 * Setup background for row data.
	 */
	clutter_container_add_actor(CLUTTER_CONTAINER(row->group), row->data_bg);
	clutter_actor_set_size(row->data_bg, 279, row_height);
	clutter_actor_set_position(row->data_bg, 201, 0);

	/*
	 * Setup data graph texture.
	 */
	row->data_fg = pkg_source_renderer_get_actor(row->renderer);
	clutter_container_add_actor(CLUTTER_CONTAINER(row->group), row->data_fg);
	clutter_actor_set_size(row->data_fg, 279, row_height);
	clutter_actor_set_position(row->data_fg, 201, 0);

	/*
	 * Setup data gloss texture.
	 */
	clutter_container_add_actor(CLUTTER_CONTAINER(row->group), row->data_gloss);
	clutter_actor_set_size(row->data_gloss, 279, row_height);
	clutter_actor_set_position(row->data_gloss, 201, 0);

	/*
	 * Setup header background.
	 */
	clutter_container_add_actor(CLUTTER_CONTAINER(row->group), row->header_bg);
	clutter_actor_set_size(row->header_bg, 200, row_height);
	clutter_actor_set_position(row->header_bg, 0, 0);

	/*
	 * Setup header text.
	 */
	g_object_set(row->header_text, "text", title, "use-markup", TRUE, NULL);
	g_object_set(row->header_text2, "text", title, "use-markup", TRUE, NULL);
	clutter_container_add_actor(CLUTTER_CONTAINER(row->group), row->header_text);
	clutter_container_add_actor(CLUTTER_CONTAINER(row->group), row->header_text2);
	height = clutter_actor_get_height(row->header_text);
	clutter_actor_set_position(row->header_text, 31, ((row_height - height) / 2) + 1);
	clutter_actor_set_position(row->header_text2, 30, ((row_height - height) / 2));

	/*
	 * Setup arrow.
	 */
	clutter_container_add_actor(CLUTTER_CONTAINER(row->group), row->header_arrow);
	clutter_actor_set_size(row->header_arrow, 10, 10);
	clutter_actor_set_reactive(row->header_arrow, TRUE);
	clutter_actor_set_position(row->header_arrow, 15, (row_height / 2));
	clutter_actor_set_anchor_point(row->header_arrow, 3.5, 5);
	g_signal_connect(row->header_arrow,
	                 "button-press-event",
	                 G_CALLBACK(pkg_session_view_row_arrow_clicked),
	                 row);

	/*
	 * Setup info button.
	 */
	clutter_container_add_actor(CLUTTER_CONTAINER(row->group), row->header_info);
	clutter_actor_set_size(row->header_info, 20, 20);
	clutter_actor_set_position(row->header_info, 175, (row_height - 20) / 2);
	clutter_actor_set_reactive(row->header_info, TRUE);
	g_signal_connect(row->header_info,
	                 "button-press-event",
	                 G_CALLBACK(pkg_session_view_row_info_clicked),
	                 row);
	g_signal_connect(row->header_info,
	                 "button-release-event",
	                 G_CALLBACK(pkg_session_view_row_info_released),
	                 row);
	

	/*
	 * Attach styling.
	 */
	if (GTK_WIDGET(session_view)->style) {
		pkg_session_view_row_set_style(row, GTK_WIDGET(session_view)->style);
	}

	clutter_actor_set_opacity(row->group, 0);
	clutter_actor_animate(row->group,
	                      CLUTTER_LINEAR,
	                      250,
	                      "opacity", 255,
	                      NULL);

	/*
	 * Show actors.
	 */
	clutter_actor_show(row->group);
	clutter_actor_show(row->data_bg);
	clutter_actor_show(row->data_fg);
	clutter_actor_show(row->data_gloss);
	clutter_actor_show(row->header_bg);
	clutter_actor_show(row->header_arrow);
	clutter_actor_show(row->header_text);
	clutter_actor_show(row->header_text2);
	clutter_actor_show(row->header_info);

	return row;
}

static void
pkg_session_view_row_free (PkgSessionViewRow *row)
{
	g_return_if_fail(row != NULL);

	g_object_unref(row->source);
	g_slice_free(PkgSessionViewRow, row);
}

static void
pkg_session_view_style_set (GtkWidget *widget,
                            GtkStyle  *old_style,
                            gpointer   user_data)
{
	PkgSessionViewPrivate *priv;
	PkgSessionView *view;
	ClutterColor dark, bg;
	GList *list;

	g_return_if_fail(PKG_IS_SESSION_VIEW(user_data));

	view = user_data;
	priv = view->priv;

	for (list = priv->rows; list; list = list->next) {
		pkg_session_view_row_set_style(list->data, GTK_WIDGET(view)->style);
	}

	gtk_clutter_get_dark_color(widget, GTK_STATE_NORMAL, &dark);
	g_object_set(priv->src_col_sep, "color", &dark, NULL);

	gtk_clutter_get_bg_color(widget, GTK_STATE_ACTIVE, &bg);
	g_object_set(priv->src_col_bg, "color", &bg, NULL);
}

static void
pkg_session_view_source_added (PkgSessionView *session_view,
                               PkSource       *source)
{
	PkgSessionViewRow *row;
	PkgSessionViewPrivate *priv;

	g_return_if_fail(PKG_IS_SESSION_VIEW(session_view));
	g_return_if_fail(PK_IS_SOURCE(source));

	priv = session_view->priv;

	row = pkg_session_view_row_new(session_view, source);
	priv->rows = g_list_append(priv->rows, row);
	clutter_container_add_actor(CLUTTER_CONTAINER(priv->row_group), row->group);
	clutter_actor_set_position(row->group, 0, priv->end_y);
	row->offset = priv->end_y;
	priv->end_y += clutter_actor_get_height(row->group);
	gtk_widget_queue_resize(GTK_WIDGET(session_view));
}

static void
pkg_session_view_stage_motion_event (ClutterActor   *stage,
                                     ClutterEvent   *event,
                                     PkgSessionView *session_view)
{
	PkgSessionViewPrivate *priv;
	gfloat w, x, ratio;
	gdouble lower, upper, pos;

	g_return_if_fail(PKG_IS_SESSION_VIEW(session_view));

	priv = session_view->priv;

	w = clutter_actor_get_width(stage);
	ratio = (event->motion.x - 200) / (w - 200);
	gtk_ruler_get_range(GTK_RULER(priv->ruler), &lower, &upper, NULL, NULL);
	pos = (upper - lower) * ratio;
	g_object_set(priv->ruler,
	             "position", CLAMP(pos, lower, upper),
	             NULL);
}

static gboolean
pkg_session_view_embed_key_press (GtkWidget      *embed,
                                  GdkEvent       *event,
                                  PkgSessionView *session_view)
{
	PkgSessionViewPrivate *priv;
	GList *iter;

	g_return_val_if_fail(PKG_IS_SESSION_VIEW(session_view), FALSE);

	priv = session_view->priv;
	if (!priv->selected) {
		return FALSE;
	}

	switch (event->key.keyval) {
	case GDK_Down:
		for (iter = priv->rows; iter; iter = iter->next) {
			if (iter->data == priv->selected && iter->next) {
				pkg_session_view_select_row(session_view, iter->next->data);
				break;
			}
		}
		break;
	case GDK_Up:
		for (iter = g_list_last(priv->rows); iter; iter = iter->prev) {
			if (iter->data == priv->selected && iter->prev) {
				pkg_session_view_select_row(session_view, iter->prev->data);
				break;
			}
		}
		break;
	}

	return TRUE;
}

static gboolean
pkg_session_view_embed_button_press (GtkWidget *widget,
                                     GdkEvent  *event,
                                     gpointer   user_data)
{
	gtk_widget_grab_focus(widget);
	return FALSE;
}

static void
pkg_session_view_drag_drop (GtkWidget      *embed,
                            GdkDragContext *context,
                            gint            x,
                            gint            y,
                            guint           time,
                            gpointer        user_data)
{
	PkSourceInfo *info;
	PkgSession *session;
	PkSource *source;
	PkChannel *channel;

	info = pkg_panels_get_selected_source_plugin();
	g_assert(info);

	session = pkg_session_view_get_session(user_data);
	g_assert(session);

	channel = pkg_session_get_channel(session);
	source = pk_channel_add_source(channel, info);

	if (!source) {
		pkg_dialog_warning(gtk_widget_get_toplevel(GTK_WIDGET(user_data)),
		                   "",
		                   "There was an error adding the data source.",
		                   NULL,
		                   FALSE);
		return;
	}

	pkg_session_view_source_added(user_data, source);

	g_debug("Added source plugin %s to channel. %dx%d",
	        pk_source_info_get_uid(info), x, y);
}

static void
pkg_session_view_vscrollbar_changed (GtkWidget      *vscroller,
                                     PkgSessionView *session_view)
{
	gdouble offset;

	g_return_if_fail(PKG_IS_SESSION_VIEW(session_view));

	offset = gtk_range_get_value(GTK_RANGE(vscroller));
	clutter_actor_set_y(session_view->priv->row_group, -offset);
}

static void
pkg_session_view_scroll_event (ClutterActor   *stage,
                               ClutterEvent   *event,
                               PkgSessionView *session_view)
{
	PkgSessionViewPrivate *priv;
	GtkAdjustment *adj;
	gboolean scroll_down;
	gdouble page, value, upper;
	gboolean down;

	down = (event->scroll.modifier_state == 1);
	if (!down) {
		if (event->scroll.modifier_state != 0) {
			return;
		}
	}

	priv = session_view->priv;
	adj = GTK_ADJUSTMENT(priv->vadj);
	upper = gtk_adjustment_get_upper(adj);

	if (priv->end_y < upper) {
		gtk_adjustment_set_value(adj, 0.);
		return;
	}

	page = gtk_adjustment_get_page_size(adj);
	value = gtk_adjustment_get_value(adj);
	value += down ? 40 : -40;

	if (value > (upper - page))
		value = upper - page;
	else if (value < 0)
		value = 0;

	gtk_adjustment_set_value(adj, value);
}

static void
pkg_session_view_finalize (GObject *object)
{
	PkgSessionViewPrivate *priv;

	g_return_if_fail (PKG_IS_SESSION_VIEW (object));

	priv = PKG_SESSION_VIEW (object)->priv;

	g_list_foreach(priv->rows, (GFunc)pkg_session_view_row_free, NULL);

	G_OBJECT_CLASS (pkg_session_view_parent_class)->finalize (object);
}

static void
pkg_session_view_class_init (PkgSessionViewClass *klass)
{
	GObjectClass *object_class;

	object_class = G_OBJECT_CLASS (klass);
	object_class->finalize = pkg_session_view_finalize;
	g_type_class_add_private(object_class, sizeof(PkgSessionViewPrivate));
}

static void
pkg_session_view_init (PkgSessionView *session_view)
{
	PkgSessionViewPrivate *priv;
	GtkWidget *icon,
	          *text,
	          *table,
	          *vscroller,
	          *hscroller,
	          *zhbox,
	          *scale,
	          *embed,
	          *img1,
	          *img2,
	          *hhbox,
	          *lbl;
	ClutterActor *stage;
	ClutterColor color;
	GtkTargetEntry target_entry[] = {
		{ "text/x-perfkit-source-info", GTK_TARGET_SAME_APP, 0 },
	};

	session_view->priv = G_TYPE_INSTANCE_GET_PRIVATE(session_view,
	                                                 PKG_TYPE_SESSION_VIEW,
	                                                 PkgSessionViewPrivate);
	priv = session_view->priv;

	/* setup label */
	priv->label = gtk_hbox_new(FALSE, 3);
	gtk_widget_show(priv->label);

	icon = gtk_image_new_from_icon_name("computer", GTK_ICON_SIZE_MENU);
	gtk_box_pack_start(GTK_BOX(priv->label), icon, FALSE, TRUE, 0);
	gtk_widget_show(icon);

	text = gtk_label_new("Session 0");
	gtk_box_pack_start(GTK_BOX(priv->label), text, TRUE, TRUE, 0);
	gtk_widget_show(text);

	priv->vpaned = gtk_vpaned_new();
	gtk_container_add(GTK_CONTAINER(session_view), priv->vpaned);
	gtk_widget_show(priv->vpaned);

	/* setup main layout */
	table = gtk_table_new(3, 3, FALSE);
	gtk_paned_add1(GTK_PANED(priv->vpaned), table);
	gtk_widget_show(table);

	priv->vadj = gtk_adjustment_new(0., 0., 0., 10., 40., 0.);
	vscroller = gtk_vscrollbar_new(GTK_ADJUSTMENT(priv->vadj));
	gtk_table_attach(GTK_TABLE(table),
	                 vscroller,
	                 2, 3, 0, 2,
	                 GTK_FILL,
	                 GTK_FILL | GTK_EXPAND,
	                 0, 0);
	g_signal_connect(vscroller,
	                 "value-changed",
	                 G_CALLBACK(pkg_session_view_vscrollbar_changed),
	                 session_view);
	gtk_widget_show(vscroller);

	hscroller = gtk_hscrollbar_new(NULL);
	gtk_table_attach(GTK_TABLE(table),
	                 hscroller,
	                 1, 2, 2, 3,
	                 GTK_FILL | GTK_EXPAND,
	                 GTK_FILL,
	                 0, 0);
	gtk_widget_show(hscroller);

	/* add sources stage */
	embed = gtk_clutter_embed_new();
	gtk_widget_add_events(embed, GDK_KEY_PRESS_MASK);
	gtk_table_attach(GTK_TABLE(table),
	                 embed,
	                 0, 2, 1, 2,
	                 GTK_FILL | GTK_EXPAND,
	                 GTK_FILL | GTK_EXPAND,
	                 0, 0);
	gtk_drag_dest_set(embed,
	                  GTK_DEST_DEFAULT_ALL,
	                  target_entry,
	                  G_N_ELEMENTS(target_entry),
	                  GDK_ACTION_COPY);
	g_signal_connect(embed,
	                 "style-set",
	                 G_CALLBACK(pkg_session_view_style_set),
	                 session_view);
	g_signal_connect(embed,
	                 "drag-drop",
	                 G_CALLBACK(pkg_session_view_drag_drop),
	                 session_view);
	g_signal_connect(embed,
	                 "scroll-event",
	                 G_CALLBACK(pkg_session_view_scroll_event),
	                 session_view);

	/*
	 * Setup stage signals.
	 */
	priv->stage = stage = gtk_clutter_embed_get_stage(GTK_CLUTTER_EMBED(embed));
	g_signal_connect(stage,
	                 "motion-event",
	                 G_CALLBACK(pkg_session_view_stage_motion_event),
	                 session_view);
	g_signal_connect(embed,
	                 "key-press-event",
	                 G_CALLBACK(pkg_session_view_embed_key_press),
	                 session_view);
	g_signal_connect(embed,
	                 "button-press-event",
	                 G_CALLBACK(pkg_session_view_embed_button_press),
	                 NULL);
	gtk_widget_show(embed);

	/* create zoom control */
	zhbox = gtk_hbox_new(FALSE, 3);
	gtk_table_attach(GTK_TABLE(table),
	                 zhbox,
	                 0, 1, 2, 3,
	                 GTK_FILL,
	                 GTK_FILL,
	                 6, 0);
	gtk_widget_set_size_request(zhbox, 188, -1);
	gtk_widget_show(zhbox);

	/* zoom out image */
	img1 = gtk_image_new_from_icon_name("zoom-out", GTK_ICON_SIZE_MENU);
	gtk_box_pack_start(GTK_BOX(zhbox), img1, FALSE, TRUE, 0);
	gtk_widget_show(img1);

	scale = gtk_hscale_new(NULL);
	gtk_scale_set_draw_value(GTK_SCALE(scale), FALSE);
	gtk_box_pack_start(GTK_BOX(zhbox), scale, TRUE, TRUE, 0);
	gtk_widget_show(scale);

	/* zoom in image */
	img2 = gtk_image_new_from_icon_name("zoom-in", GTK_ICON_SIZE_MENU);
	gtk_box_pack_start(GTK_BOX(zhbox), img2, FALSE, TRUE, 0);
	gtk_widget_show(img2);

	priv->src_col_bg = clutter_rectangle_new();
	clutter_container_add_actor(CLUTTER_CONTAINER(stage), priv->src_col_bg);
	clutter_actor_set_size(priv->src_col_bg, 200, 1000);
	clutter_actor_set_position(priv->src_col_bg, 0, 0);
	clutter_actor_show(priv->src_col_bg);

	priv->src_col_sep = clutter_rectangle_new();
	clutter_container_add_actor(CLUTTER_CONTAINER(stage), priv->src_col_sep);
	clutter_actor_set_size(priv->src_col_sep, 1, 1000);
	clutter_actor_set_position(priv->src_col_sep, 200, 0);
	clutter_actor_show(priv->src_col_sep);

	priv->row_group = clutter_group_new();
	clutter_container_add_actor(CLUTTER_CONTAINER(priv->stage),
	                            priv->row_group);
	clutter_actor_set_position(priv->row_group, 0, 0);
	clutter_actor_show(priv->row_group);

	priv->zadj = gtk_adjustment_new(1., .5, 5., .25, 1., 1.);
	g_object_set(scale, "adjustment", priv->zadj, NULL);

	/* position ruler */
	priv->ruler = gtk_hruler_new();
	gtk_table_attach(GTK_TABLE(table),
	                 priv->ruler,
	                 1, 2, 0, 1,
	                 GTK_FILL | GTK_EXPAND,
	                 GTK_FILL,
	                 0, 0);
	gtk_ruler_set_range(GTK_RULER(priv->ruler), 0., 10., 0., 0.);
	gtk_widget_show(priv->ruler);

	hhbox = gtk_hbox_new(FALSE, 6);
	gtk_table_attach(GTK_TABLE(table),
	                 hhbox,
	                 0, 1, 0, 1,
	                 GTK_FILL,
	                 GTK_FILL,
	                 0, 0);
	gtk_widget_show(hhbox);

	lbl = gtk_label_new(_("Sources"));
	gtk_box_pack_start(GTK_BOX(hhbox), lbl, TRUE, TRUE, 0);
	g_object_set(lbl, "ypad", 3, NULL);
	gtk_widget_show(lbl);

	g_signal_connect(session_view, "size-allocate",
	                 G_CALLBACK(pkg_session_view_size_allocated),
	                 session_view);



	// tmp


	/*
	{
		PkgSessionViewRow *row;

		row = pkg_session_view_row_new(session_view, NULL);
		priv->rows = g_list_append(priv->rows, row);
		clutter_container_add_actor(CLUTTER_CONTAINER(stage), row->group);
		clutter_actor_set_position(row->group, 0, 0);

		row = pkg_session_view_row_new(session_view, NULL);
		priv->rows = g_list_append(priv->rows, row);
		clutter_container_add_actor(CLUTTER_CONTAINER(stage), row->group);
		clutter_actor_set_position(row->group, 0, 40);

		row = pkg_session_view_row_new(session_view, NULL);
		priv->rows = g_list_append(priv->rows, row);
		clutter_container_add_actor(CLUTTER_CONTAINER(stage), row->group);
		clutter_actor_set_position(row->group, 0, 80);
	}
	*/
}
