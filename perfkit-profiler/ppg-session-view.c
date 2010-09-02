/* ppg-session-view.c
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <clutter-gtk/clutter-gtk.h>
#include <gdk/gdkkeysyms.h>
#include <glib/gi18n.h>

#include "ppg-session-view.h"


#if 0
#define ENTRY g_printerr("ENTRY: %s(): %d\n", G_STRFUNC, __LINE__)
#define EXIT                                                  \
	G_STMT_START {                                            \
		g_printerr(" EXIT: %s(): %d\n", G_STRFUNC, __LINE__); \
		return;                                               \
	} G_STMT_END
#define RETURN(r)                                             \
	G_STMT_START {                                            \
		g_printerr(" EXIT: %s(): %d\n", G_STRFUNC, __LINE__); \
		return (r);                                           \
	} G_STMT_END
#define CASE(_l)                                              \
    case _l:                                                  \
        G_STMT_START {                                        \
            g_printerr(" CASE: %s(): %d %s\n",                \
                       G_STRFUNC, __LINE__, #_l);             \
        } G_STMT_END

#else
#define ENTRY
#define EXIT      return
#define RETURN(r) return (r)
#define CASE(_l)  case _l:
#endif

#define LEFT_SIZE          200
#define PIXELS_PER_SECOND   20
#define ROW_HEIGHT          45

#define LOWER     (0.0)
#define UPPER     (2.0)
#define SCALE_MIN (0.001)
#define SCALE_MAX (100.0)


#define gdk_cairo_add_color_stop(pattern, offset, color) \
    G_STMT_START {                                       \
        cairo_pattern_add_color_stop_rgb(                \
            (pattern), (offset),                         \
            (color)->red / 65535.0,                      \
            (color)->green / 65535.0,                    \
            (color)->blue / 65535.0);                    \
    } G_STMT_END


G_DEFINE_TYPE(PpgSessionView, ppg_session_view, GTK_TYPE_ALIGNMENT)


typedef struct
{
	ClutterActor *group;
	ClutterActor *hdr_bg;
	ClutterActor *hdr_text;
	ClutterActor *data_bg;
	ClutterActor *data_bg2;
} Row;


struct _PpgSessionViewPrivate
{
	GPtrArray *rows;

	GtkAdjustment *vadj;
	GtkAdjustment *hadj;
	GtkAdjustment *zadj;

	GtkWidget *paned;
	GtkWidget *details;
	GtkWidget *ruler;
	GtkWidget *embed;

	ClutterActor *bg;
	ClutterActor *separator;
	ClutterActor *rows_box;

	Row *selected;

	gboolean in_move;
};


/**
 * row_new:
 *
 * Create a new row within the session view.
 *
 * Returns: The newly allocated Row.
 * Side effects: None.
 */
static Row*
row_new (void)
{
	ClutterColor black;
	ClutterColor white;
	Row *row;

	clutter_color_from_string(&black, "#000");
	clutter_color_from_string(&white, "#fff");

	row = g_slice_new0(Row);
	row->group = g_object_new(CLUTTER_TYPE_GROUP,
	                          "reactive", TRUE,
	                          NULL);
	row->hdr_bg = clutter_cairo_texture_new(200, ROW_HEIGHT);
	row->hdr_text = g_object_new(CLUTTER_TYPE_TEXT,
	                             "font-name", "Sans",
	                             "color", &black,
	                             "x", 15.0,
	                             NULL);
	row->data_bg = g_object_new(CLUTTER_TYPE_RECTANGLE,
	                            "width", 10.0,
	                            "height", (gfloat)ROW_HEIGHT,
	                            "color", &black,
	                            "x", 200.0,
	                            NULL);
	row->data_bg2 = g_object_new(CLUTTER_TYPE_RECTANGLE,
	                             "width", 10.0,
	                             "height", (gfloat)ROW_HEIGHT - 1.0,
	                             "color", &white,
	                             "x", 200.0,
	                             "y", 0.0,
	                             NULL);

	clutter_container_add(CLUTTER_CONTAINER(row->group),
	                      row->hdr_bg,
	                      row->hdr_text,
	                      row->data_bg,
	                      row->data_bg2,
	                      NULL);

	return row;
}


/**
 * row_paint:
 * @row: A #Row.
 * @view: A #PpgSessionView.
 *
 * Updates the styling and content of a row.  If you wish to only update the
 * data lines of a row, use row_paint_data().
 *
 * Returns: None.
 * Side effects: None.
 */
static void
row_paint (Row            *row,
           PpgSessionView *view)
{
	PpgSessionViewPrivate *priv;
	GtkWidget *widget = GTK_WIDGET(view);
	cairo_pattern_t *linear;
	GtkStateType state;
	ClutterColor color;
	GtkStyle *style;
	cairo_t *cr;
	gfloat width;
	gfloat height;

	g_return_if_fail(PPG_SESSION_VIEW(view));
	g_return_if_fail(row != NULL);

	priv = view->priv;
	style = gtk_widget_get_style(widget);
	state = GTK_STATE_NORMAL;
	if (row == priv->selected) {
		state = GTK_STATE_SELECTED;
	}
	/*
	 * Update font color.
	 */
	gtk_clutter_get_text_color(widget, state, &color);
	g_object_set(row->hdr_text, "color", &color, NULL);
	/*
	 * Paint header background.
	 */
	cr = clutter_cairo_texture_create(CLUTTER_CAIRO_TEXTURE(row->hdr_bg));
	g_object_get(row->group, "width", &width, "height", &height, NULL);
	linear = cairo_pattern_create_linear(0, 0, 0, height);
	gdk_cairo_add_color_stop(linear, 0.0, &style->mid[state]);
	gdk_cairo_add_color_stop(linear, 1.0, &style->dark[state]);
	cairo_rectangle(cr, 0, 0, width, height);
	cairo_set_source(cr, linear);
	cairo_fill(cr);
	/*
	 * Cleanup resources.
	 */
	cairo_pattern_destroy(linear);
	cairo_destroy(cr);
	/*
	 * Update row background.
	 */
	gtk_clutter_get_dark_color(widget, state, &color);
	g_object_set(row->data_bg, "color", &color, NULL);
}


/**
 * row_set_title:
 * @row: A Row.
 * @title: The new title for the row.
 *
 * Updates the title for a row and sets the text position.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
row_set_title (Row         *row,
               const gchar *title)
{
	gfloat height;

	g_return_if_fail(row != NULL);
	g_return_if_fail(title != NULL);

	g_object_set(row->hdr_text, "text", title, NULL);
	height = clutter_actor_get_height(row->hdr_text);
	clutter_actor_set_y(row->hdr_text, (gint)((ROW_HEIGHT - height) / 2.));
}


/**
 * row_free:
 * @row: A #Row.
 *
 * Cleans up after a row and frees the memory allocated by it.
 *
 * Returns: None.
 * Side effects: Structure is deallocated.
 */
static void
row_free (Row      *row,
          gboolean  in_finalize)
{
	g_return_if_fail(row != NULL);

	if (!in_finalize) {
		clutter_actor_destroy(row->group);
	}
	g_slice_free(Row, row);
}


/**
 * row_attach:
 * @row: (in): A #Row.
 * @view: (in): A #PpgSessionView.
 *
 * Attaches a row by adding it to the PpgSessionView stage.
 *
 * Returns: None.
 * Side effects: Actor is added to stage which may make it visible and
 *   adjust the visible rows on screen.
 */
static void
row_attach (Row            *row,
            PpgSessionView *view)
{
	PpgSessionViewPrivate *priv;
	ClutterBoxLayout *layout;

	g_return_if_fail(row != NULL);
	g_return_if_fail(PPG_SESSION_VIEW(view));

	priv = view->priv;
	g_object_get(priv->rows_box, "layout-manager", &layout, NULL);
	clutter_box_layout_pack(layout, row->group,
	                        TRUE, TRUE, FALSE,
	                        CLUTTER_BOX_ALIGNMENT_START,
	                        CLUTTER_BOX_ALIGNMENT_START);
	g_object_unref(layout);
	row_paint(row, view);
}


static void
row_make_visible (Row            *row,
                  PpgSessionView *view)
{
	PpgSessionViewPrivate *priv;
	GtkAllocation alloc;
	gfloat row_y;
	gfloat row_h;
	gfloat box_y;

	g_return_if_fail(row != NULL);
	g_return_if_fail(PPG_IS_SESSION_VIEW(view));

	priv = view->priv;
	gtk_widget_get_allocation(priv->embed, &alloc);
	g_object_get(row->group, "y", &row_y, "height", &row_h, NULL);
	g_object_get(priv->rows_box, "y", &box_y, NULL);
	if (box_y < -row_y) {
		clutter_actor_set_y(priv->rows_box, -row_y);
	} else if ((row_y + row_h + box_y) > alloc.height) {
		clutter_actor_set_y(priv->rows_box, -(row_y + row_h - alloc.height));
	}
}


/**
 * row_detach:
 * @row: A #Row.
 * @view: A #PpgSessionView.
 *
 * Removes a row from the stage.
 *
 * Returns: None.
 * Side effects: Row will no longer be visible.
 */
#if 0
static void
row_detach (Row          *row,
            ClutterActor *box)
{
	g_return_if_fail(row != NULL);
	g_return_if_fail(CLUTTER_IS_ACTOR(box));

	clutter_container_remove_actor(CLUTTER_CONTAINER(box), row->group);
}
#endif


/**
 * ppg_session_view_new:
 *
 * Creates a new instance of #PpgSessionView.
 *
 * Returns: the newly created #PpgSessionView instance.
 * Side effects: None.
 */
GtkWidget *
ppg_session_view_new (void)
{
	return g_object_new(PPG_TYPE_SESSION_VIEW, NULL);
}


static void
ppg_session_view_select_row (PpgSessionView *view,
                             Row            *row)
{
	PpgSessionViewPrivate *priv;
	Row *tmprow;

	g_return_if_fail(PPG_IS_SESSION_VIEW(view));

	ENTRY;

	priv = view->priv;
	if (priv->selected == row) {
		EXIT;
	}
	if ((tmprow = priv->selected)) {
		priv->selected = NULL;
		row_paint(tmprow, view);
	}
	priv->selected = row;
	if (priv->selected) {
		row_paint(priv->selected, view);
		row_make_visible(priv->selected, view);
	}

	EXIT;
}


static gboolean
ppg_session_view_scroll_event (ClutterActor       *stage,
                               ClutterScrollEvent *scroll,
                               PpgSessionView     *view)
{
	PpgSessionViewPrivate *priv;
	gboolean ret = TRUE;
	gdouble upper;
	gdouble lower;
	gdouble value;
	gdouble page;

	g_return_val_if_fail(PPG_IS_SESSION_VIEW(view), FALSE);
	g_return_val_if_fail(scroll->type == CLUTTER_SCROLL, FALSE);

	ENTRY;

	priv = view->priv;
	g_object_get(priv->vadj,
	             "upper", &upper,
	             "lower", &lower,
	             "value", &value,
	             "page-size", &page,
	             NULL);
	upper = upper - page;
	page = page / 5.0;

	switch (scroll->direction) {
	CASE(CLUTTER_SCROLL_DOWN);
		value += page;
		gtk_adjustment_set_value(priv->vadj, CLAMP(value, lower, upper));
		break;
	CASE(CLUTTER_SCROLL_UP);
		value -= page;
		gtk_adjustment_set_value(priv->vadj, CLAMP(value, lower, upper));
		break;
	CASE(CLUTTER_SCROLL_RIGHT);
	CASE(CLUTTER_SCROLL_LEFT);
	default:
		ret = FALSE;
		break;
	}

	RETURN(ret);
}


static gboolean
ppg_session_view_key_press_event (GtkWidget      *widget,
                                  GdkEventKey    *key,
                                  PpgSessionView *view)
{
	PpgSessionViewPrivate *priv;
	GtkAllocation alloc;
	gboolean ret = TRUE;
	gboolean found = FALSE;
	Row *last = NULL;
	Row *row;
	gdouble upper;
	gdouble value;
	gdouble lower;
	gdouble page;
	gint i;

	g_return_val_if_fail(PPG_IS_SESSION_VIEW(view), FALSE);
	g_return_val_if_fail(key->type == GDK_KEY_PRESS, FALSE);

	ENTRY;

	priv = view->priv;
	gtk_widget_get_allocation(priv->embed, &alloc);
	g_object_get(priv->vadj,
	             "value", &value,
	             "upper", &upper,
	             "lower", &lower,
	             "page-increment", &page,
	             NULL);
	upper -= alloc.height;

	switch (key->keyval) {
	CASE(GDK_Page_Up);
		value -= page;
		gtk_adjustment_set_value(priv->vadj, CLAMP(value, lower, upper));
		break;
	CASE(GDK_Page_Down);
		value += page;
		gtk_adjustment_set_value(priv->vadj, CLAMP(value, lower, upper));
		break;
	CASE(GDK_Home);
		if (key->state & GDK_CONTROL_MASK) {
			if (priv->rows->len) {
				row = g_ptr_array_index(priv->rows, 0);
				ppg_session_view_select_row(view, row);
			}
		}
		break;
	CASE(GDK_End);
		if (key->state & GDK_CONTROL_MASK) {
			if (priv->rows->len) {
				row = g_ptr_array_index(priv->rows, priv->rows->len - 1);
				ppg_session_view_select_row(view, row);
			}
		}
		break;
	CASE(GDK_Down);
		if (!priv->selected) {
			if (priv->rows->len) {
				row = g_ptr_array_index(priv->rows, 0);
				ppg_session_view_select_row(view, row);
			}
		} else {
			for (i = 0; i < priv->rows->len; i++) {
				row = g_ptr_array_index(priv->rows, i);
				if (found) {
					ppg_session_view_select_row(view, row);
					break;
				}
				if (priv->selected == row) {
					found = TRUE;
				}
			}
		}
		break;
	CASE(GDK_Up);
		if (!priv->selected) {
			if (priv->rows->len) {
				row = g_ptr_array_index(priv->rows, priv->rows->len - 1);
				ppg_session_view_select_row(view, row);
			}
		} else {
			for (i = 0; i < priv->rows->len; i++) {
				row = g_ptr_array_index(priv->rows, i);
				if (row == priv->selected) {
					if (last) {
						ppg_session_view_select_row(view, last);
					}
				}
				last = row;
			}
		}
		break;
	CASE(GDK_Return);
		if (priv->selected) {
			if (clutter_actor_get_height(priv->selected->hdr_bg) >= alloc.height) {
				clutter_actor_animate(priv->selected->hdr_bg, CLUTTER_LINEAR, 500,
				                      "height", (gfloat)ROW_HEIGHT,
				                      NULL);
			} else {
				clutter_actor_animate(priv->selected->hdr_bg, CLUTTER_LINEAR, 500,
				                      "height", (gfloat)alloc.height,
				                      NULL);
			}
			clutter_actor_set_y(priv->rows_box,
			                    -(gint)clutter_actor_get_y(priv->selected->group));
		}
		break;
	default:
		ret = FALSE;
		break;
	}

	/*
	 * XXX: Hack to work around our focus leaving the widget.
	 */
	if (ret) {
		gtk_widget_grab_focus(priv->embed);
	}

	RETURN(ret);
}


static gboolean
ppg_session_view_button_press_event (ClutterActor       *stage,
                                     ClutterButtonEvent *button,
                                     PpgSessionView     *view)
{
	PpgSessionViewPrivate *priv;
	ClutterActorBox box;
	gboolean ret = TRUE;
	gfloat y;
	Row *row;
	gint i;

	g_return_val_if_fail(CLUTTER_IS_ACTOR(stage), FALSE);
	g_return_val_if_fail(PPG_IS_SESSION_VIEW(view), FALSE);

	ENTRY;

	priv = view->priv;
	switch (button->button) {
	CASE(1);
		switch (button->click_count) {
		CASE(1);
			if (button->modifier_state & CLUTTER_CONTROL_MASK) {
				ppg_session_view_select_row(view, NULL);
				break;
			}
			/*
			 * XXX: We could keep the index of the top-most row here to reduce
			 *      the size of the linear scan.
			 */
			y = button->y - clutter_actor_get_y(priv->rows_box);
			for (i = 0; i < priv->rows->len; i++) {
				row = g_ptr_array_index(priv->rows, i);
				clutter_actor_get_allocation_box(row->group, &box);
				if (clutter_actor_box_contains(&box, button->x, y)) {
					ppg_session_view_select_row(view, row);
					break;
				}
			}
			break;
		default:
			break;
		}
		break;
	default:
		ret = FALSE;
	}

	if (ret) {
		gtk_widget_grab_focus(priv->embed);
	}

	RETURN(ret);
}


/**
 * ppg_session_view_update_adjustments:
 * @view: (in): A #PpgSessionView.
 *
 * Update the GtkAdjustment's used by scrollbars and scales.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
ppg_session_view_update_adjustments (PpgSessionView *view)
{
	PpgSessionViewPrivate *priv;
	GtkAllocation alloc;
	gdouble upper;
	gdouble page;

	g_return_if_fail(PPG_IS_SESSION_VIEW(view));

	priv = view->priv;
	gtk_widget_get_allocation(priv->embed, &alloc);
	/*
	 * Update the vertical adjustment.
	 */
	upper = MAX(1.0, clutter_actor_get_height(priv->rows_box));
	page = MAX(1.0, alloc.height);
	g_object_set(priv->vadj,
	             "page-increment", (page / 2.0),
	             "page-size", page,
	             "upper", upper,
	             NULL);
	/*
	 * XXX: Update the horizontal adjustment.
	 */
	upper = 1;
	page = 1;
	g_object_set(priv->hadj, "page-size", page, "upper", upper, NULL);
}


/**
 * ppg_session_view_add_row:
 * @view: A #PpgSessionView.
 *
 * Adds a new row to the view.
 *
 * Returns: A #Row.
 * Side effects: None.
 */
static Row *
ppg_session_view_add_row (PpgSessionView *view,
                          const gchar    *title)
{
	PpgSessionViewPrivate *priv;
	Row *row;

	g_return_val_if_fail(PPG_IS_SESSION_VIEW(view), NULL);

	ENTRY;

	priv = view->priv;
	row = row_new();
	row_set_title(row, title);
	row_attach(row, view);
	g_ptr_array_add(priv->rows, row);

	RETURN(row);
}


/**
 * ppg_session_view_rows_allocation_notify:
 * @rows_box: A #ClutterBox.
 * @pspec: A #GParamSpec.
 * @view: A #PpgSessionView.
 *
 * Handles the allocation changing of the rows container.  The vertical
 * adjustment is updated if needed.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
ppg_session_view_rows_allocation_notify (ClutterActor    *rows_box,
                                         GParamSpec      *pspec,
                                         PpgSessionView  *view)
{
	PpgSessionViewPrivate *priv;
	ClutterActorBox box;
	gdouble upper;
	gdouble value;

	g_return_if_fail(CLUTTER_IS_ACTOR(rows_box));
	g_return_if_fail(PPG_IS_SESSION_VIEW(view));

	ENTRY;

	priv = view->priv;
	if (!priv->in_move) {
		clutter_actor_get_allocation_box(rows_box, &box);
		upper = clutter_actor_box_get_height(&box);
		value = -(gint)box.y1;
		g_object_set(priv->vadj,
		             "upper", upper,
		             "value", value,
		             NULL);
	}

	EXIT;
}


/**
 * ppg_session_view_zoom_changed:
 * @adj: A #GtkAdjustment.
 * @view: A #PpgSessionView.
 *
 * Handle the zoom adjustment changing position.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
ppg_session_view_zoom_changed (GtkAdjustment  *adj,
                               PpgSessionView *view)
{
	PpgSessionViewPrivate *priv;
	GtkAllocation alloc;
	gdouble scale;
	gdouble upper;
	gdouble lower;

	g_return_if_fail(GTK_IS_ADJUSTMENT(adj));
	g_return_if_fail(PPG_IS_SESSION_VIEW(view));

	ENTRY;

	priv = view->priv;
	gtk_widget_get_allocation(priv->embed, &alloc);
	g_object_get(adj, "value", &scale, NULL);
	/*
	 * Scale the value to ratio if needed.
	 */
	scale = CLAMP(scale, LOWER, UPPER);
	if (scale > 1.0) {
		scale = (scale - 1.0) * SCALE_MAX;
	}
	scale = CLAMP(scale, SCALE_MIN, SCALE_MAX);
	/*
	 * Determine the range for the ruler.
	 */
	upper = (alloc.width - LEFT_SIZE) / (scale * PIXELS_PER_SECOND);
	lower = 0.0; /* FIXME */
	gtk_ruler_set_range(GTK_RULER(priv->ruler),
	                    lower, upper, lower, 0);

	EXIT;
}


/**
 * ppg_session_view_vadj_changed:
 * @adj: A #GtkAdjustment.
 * @view: A #PpgSessionView.
 *
 * Handle the vertical adjustment changing position.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
ppg_session_view_vadj_changed (GtkAdjustment  *adj,
                               PpgSessionView *view)
{
	PpgSessionViewPrivate *priv;
	gint value;

	g_return_if_fail(GTK_IS_ADJUSTMENT(adj));
	g_return_if_fail(PPG_IS_SESSION_VIEW(view));

	ENTRY;

	priv = view->priv;
	priv->in_move = TRUE;
	value = gtk_adjustment_get_value(adj);
	clutter_actor_set_y(priv->rows_box, -value);
	priv->in_move = FALSE;

	EXIT;
}


/**
 * ppg_session_view_size_allocate:
 * @widget: (in): A #PpgSessionView.
 * @alloc: (in): A #GtkAllocation.
 *
 * Handle the size-allocation of the #GtkWidget.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
ppg_session_view_size_allocate (GtkWidget     *widget,
                                GtkAllocation *alloc)
{
	PpgSessionViewPrivate *priv;
	PpgSessionView *view;
	GtkAllocation embed_alloc;
	ClutterActor *stage;
	gfloat height;
	gfloat width;
	Row *row;
	gint i;

	g_return_if_fail(PPG_IS_SESSION_VIEW(widget));

	ENTRY;

	view = PPG_SESSION_VIEW(widget);
	priv = view->priv;
	/*
	 * Call parent class.
	 */
	GTK_WIDGET_CLASS(ppg_session_view_parent_class)->size_allocate(widget, alloc);
	/*
	 * Update various adjustments.
	 */
	ppg_session_view_update_adjustments(PPG_SESSION_VIEW(widget));
	/*
	 * Update actors.
	 */
	g_object_set(priv->bg, "height", (gfloat)alloc->height, NULL);
	g_object_set(priv->separator, "height", (gfloat)alloc->height, NULL);
	height = clutter_actor_get_height(priv->rows_box);
	stage = gtk_clutter_embed_get_stage(GTK_CLUTTER_EMBED(priv->embed));
	if (height <= clutter_actor_get_height(stage)) {
		clutter_actor_set_y(priv->rows_box, 0.0);
	}
	/*
	 * Repaint row backgrounds.
	 */
	gtk_widget_get_allocation(priv->embed, &embed_alloc);
	width = embed_alloc.width - LEFT_SIZE;
	for (i = 0; i < priv->rows->len; i++) {
		row = g_ptr_array_index(priv->rows, i);
		clutter_actor_set_width(row->data_bg, width);
		clutter_actor_set_width(row->data_bg2, width);
		/*
		 * XXX: Move to zoom_changed handler?
		 */
		row_paint(row, view);
	}
	/*
	 * Adjust visible zrange.
	 */
	ppg_session_view_zoom_changed(priv->zadj, view);

	EXIT;
}


/**
 * ppg_session_view_style_set:
 * @widget: A #PpgSessionView.
 * @old_style: A #GtkStyle.
 *
 * Updates the styling of the widget based on the new #GtkStyle.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
ppg_session_view_style_set (GtkWidget *widget,
                            GtkStyle  *old_style)
{
	PpgSessionView *view = PPG_SESSION_VIEW(widget);
	PpgSessionViewPrivate *priv;
	ClutterColor color;
	GtkStyle *style;
	Row *row;
	gint i;

	g_return_if_fail(PPG_IS_SESSION_VIEW(widget));

	ENTRY;

	priv = PPG_SESSION_VIEW(widget)->priv;
	GTK_WIDGET_CLASS(ppg_session_view_parent_class)->style_set(widget, old_style);
	style = gtk_widget_get_style(widget);
	/*
	 * Update clutter actor styles.
	 */
	gtk_clutter_get_dark_color(widget, GTK_STATE_NORMAL, &color);
	g_object_set(priv->separator, "color", &color, NULL);
	gtk_clutter_get_mid_color(widget, GTK_STATE_NORMAL, &color);
	g_object_set(priv->bg, "color", &color, NULL);
	/*
	 * Repaint row backgrounds.
	 */
	for (i = 0; i < priv->rows->len; i++) {
		row = g_ptr_array_index(priv->rows, i);
		row_paint(row, view);
	}

	EXIT;
}


/**
 * ppg_session_view_grab_focus:
 * @widget: (in): A #PpgSessionView.
 *
 * Handle the "grab-focus" signal for the widget.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
ppg_session_view_grab_focus (GtkWidget *widget)
{
	PpgSessionViewPrivate *priv;

	g_return_if_fail(PPG_IS_SESSION_VIEW(widget));

	priv = PPG_SESSION_VIEW(widget)->priv;
	gtk_widget_grab_focus(priv->embed);
}


/**
 * ppg_session_view_finalize:
 * @object: (in): A #PpgSessionView.
 *
 * Finalizer for a #PpgSessionView instance.  Frees any resources held by
 * the instance.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
ppg_session_view_finalize (GObject *object)
{
	PpgSessionViewPrivate *priv = PPG_SESSION_VIEW(object)->priv;
	Row *row;
	gint i;

	if (priv->rows) {
		for (i = 0; i < priv->rows->len; i++) {
			row = g_ptr_array_index(priv->rows, i);
			row_free(row, TRUE);
		}
		g_ptr_array_free(priv->rows, TRUE);
	}

	G_OBJECT_CLASS(ppg_session_view_parent_class)->finalize(object);
}


/**
 * ppg_session_view_class_init:
 * @klass: (in): A #PpgSessionViewClass.
 *
 * Initializes the #PpgSessionViewClass and prepares the vtable.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
ppg_session_view_class_init (PpgSessionViewClass *klass)
{
	GObjectClass *object_class;
	GtkWidgetClass *widget_class;

	object_class = G_OBJECT_CLASS(klass);
	object_class->finalize = ppg_session_view_finalize;
	g_type_class_add_private(object_class, sizeof(PpgSessionViewPrivate));

	widget_class = GTK_WIDGET_CLASS(klass);
	widget_class->size_allocate = ppg_session_view_size_allocate;
	widget_class->style_set = ppg_session_view_style_set;
	widget_class->grab_focus = ppg_session_view_grab_focus;
}


/**
 * ppg_session_view_init:
 * @view: (in): A #PpgSessionView.
 *
 * Initializes the newly created #PpgSessionView instance.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
ppg_session_view_init (PpgSessionView *view)
{
	PpgSessionViewPrivate *priv;
	ClutterLayoutManager *manager;
	ClutterActor *stage;
	ClutterColor color;
	GtkWidget *table;
	GtkWidget *scroll;
	GtkWidget *hbox;
	GtkWidget *image;
	GtkWidget *scale;

	priv = G_TYPE_INSTANCE_GET_PRIVATE(view, PPG_TYPE_SESSION_VIEW,
	                                   PpgSessionViewPrivate);
	view->priv = priv;

	priv->rows = g_ptr_array_new();
	priv->hadj = g_object_new(GTK_TYPE_ADJUSTMENT,
	                          NULL);
	priv->vadj = g_object_new(GTK_TYPE_ADJUSTMENT,
	                          "step-increment", 1.0,
	                          NULL);
	priv->zadj = g_object_new(GTK_TYPE_ADJUSTMENT,
	                          "lower", 0.001,
	                          "upper", 2.0,
	                          "step-increment", 0.01,
	                          "page-increment", 0.5,
	                          "value", 1.0,
	                          NULL);

	priv->paned = g_object_new(GTK_TYPE_VPANED,
	                           "visible", TRUE,
	                           NULL);
	gtk_container_add(GTK_CONTAINER(view), priv->paned);

	table = g_object_new(GTK_TYPE_TABLE,
	                     "n-columns", 3,
	                     "n-rows", 4,
	                     "visible", TRUE,
	                     NULL);
	gtk_container_add_with_properties(GTK_CONTAINER(priv->paned),
	                                  table,
	                                  "shrink", FALSE,
	                                  "resize", FALSE,
	                                  NULL);

	priv->details = g_object_new(GTK_TYPE_ALIGNMENT,
	                             "visible", TRUE,
	                             NULL);
	gtk_container_add_with_properties(GTK_CONTAINER(priv->paned),
	                                  priv->details,
	                                  "shrink", FALSE,
	                                  "resize", FALSE,
	                                  NULL);

	scroll = g_object_new(GTK_TYPE_VSCROLLBAR,
	                      "adjustment", priv->vadj,
	                      "visible", TRUE,
	                      NULL);
	gtk_container_add_with_properties(GTK_CONTAINER(table), scroll,
	                                  "bottom-attach", 2,
	                                  "left-attach", 2,
	                                  "right-attach", 3,
	                                  "top-attach", 0,
	                                  "x-options", GTK_FILL,
	                                  "y-options", GTK_FILL | GTK_EXPAND,
	                                  NULL);

	scroll = g_object_new(GTK_TYPE_HSCROLLBAR,
	                      "adjustment", priv->hadj,
	                      "visible", TRUE,
	                      NULL);
	gtk_container_add_with_properties(GTK_CONTAINER(table), scroll,
	                                  "bottom-attach", 3,
	                                  "left-attach", 1,
	                                  "right-attach", 2,
	                                  "top-attach", 2,
	                                  "x-options", GTK_EXPAND | GTK_FILL,
	                                  "y-options", GTK_FILL,
	                                  NULL);

	hbox = g_object_new(GTK_TYPE_HBOX,
	                    "spacing", 3,
	                    "width-request", LEFT_SIZE,
	                    "visible", TRUE,
	                    NULL);
	gtk_container_add_with_properties(GTK_CONTAINER(table), hbox,
	                                  "bottom-attach", 4,
	                                  "left-attach", 0,
	                                  "right-attach", 1,
	                                  "top-attach", 2,
	                                  "x-options", GTK_FILL,
	                                  "y-options", GTK_FILL,
	                                  NULL);

	image = g_object_new(GTK_TYPE_IMAGE,
	                     "icon-name", GTK_STOCK_ZOOM_OUT,
	                     "icon-size", GTK_ICON_SIZE_MENU,
	                     "visible", TRUE,
	                     "xpad", 3,
	                     NULL);
	gtk_container_add_with_properties(GTK_CONTAINER(hbox), image,
	                                  "expand", FALSE,
	                                  "fill", TRUE,
	                                  "pack-type", GTK_PACK_START,
	                                  NULL);

	image = g_object_new(GTK_TYPE_IMAGE,
	                     "icon-name", GTK_STOCK_ZOOM_IN,
	                     "icon-size", GTK_ICON_SIZE_MENU,
	                     "visible", TRUE,
	                     "xpad", 3,
	                     NULL);
	gtk_container_add_with_properties(GTK_CONTAINER(hbox), image,
	                                  "expand", FALSE,
	                                  "fill", TRUE,
	                                  "pack-type", GTK_PACK_END,
	                                  NULL);

	scale = g_object_new(GTK_TYPE_HSCALE,
	                     "adjustment", priv->zadj,
	                     "draw-value", FALSE,
	                     "digits", 2,
	                     "visible", TRUE,
	                     "width-request", 75,
	                      NULL);
	gtk_container_add_with_properties(GTK_CONTAINER(hbox), scale,
	                                  "expand", TRUE,
	                                  "fill", TRUE,
	                                  "pack-type", GTK_PACK_START,
	                                  NULL);
	gtk_scale_add_mark(GTK_SCALE(scale), 1.0, GTK_POS_BOTTOM, NULL);

	priv->ruler = g_object_new(GTK_TYPE_HRULER,
	                           "visible", TRUE,
	                           NULL);
	gtk_container_add_with_properties(GTK_CONTAINER(table), priv->ruler,
	                                  "bottom-attach", 1,
	                                  "left-attach", 1,
	                                  "right-attach", 2,
	                                  "top-attach", 0,
	                                  "x-options", GTK_EXPAND | GTK_FILL,
	                                  "y-options", 0,
	                                  NULL);

	priv->embed = g_object_new(GTK_CLUTTER_TYPE_EMBED,
	                           "can-focus", TRUE,
	                           "has-focus", TRUE,
	                           "visible", TRUE,
	                           NULL);
	gtk_container_add_with_properties(GTK_CONTAINER(table), priv->embed,
	                                  "bottom-attach", 2,
	                                  "left-attach", 0,
	                                  "right-attach", 2,
	                                  "top-attach", 1,
	                                  "x-options", GTK_EXPAND | GTK_FILL,
	                                  "y-options", GTK_EXPAND | GTK_FILL,
	                                  NULL);

	stage = gtk_clutter_embed_get_stage(GTK_CLUTTER_EMBED(priv->embed));
	clutter_color_from_string(&color, "#FFF");
	g_object_set(stage, "color", &color, "reactive", TRUE, NULL);

	priv->bg = g_object_new(CLUTTER_TYPE_RECTANGLE,
	                        "width", 200.0,
	                        NULL);
	clutter_container_add_actor(CLUTTER_CONTAINER(stage), priv->bg);

	manager = g_object_new(CLUTTER_TYPE_BOX_LAYOUT,
	                       "easing-duration", 250,
	                       "easing-mode", CLUTTER_EASE_IN_QUAD,
	                       "pack-start", FALSE,
	                       "use-animations", TRUE,
	                       "vertical", TRUE,
	                       NULL);
	priv->rows_box = g_object_new(CLUTTER_TYPE_BOX,
	                              "layout-manager", manager,
	                              NULL);
	clutter_container_add_actor(CLUTTER_CONTAINER(stage), priv->rows_box);

	priv->separator = g_object_new(CLUTTER_TYPE_RECTANGLE,
	                               "x", 200.0,
	                               "width", 1.0,
	                               "height", 10.0,
	                               NULL);
	clutter_container_add_actor(CLUTTER_CONTAINER(stage), priv->separator);

	g_signal_connect(stage,
	                 "scroll-event",
	                 G_CALLBACK(ppg_session_view_scroll_event),
	                 view);

	g_signal_connect(priv->embed,
	                 "key-press-event",
	                 G_CALLBACK(ppg_session_view_key_press_event),
	                 view);

	g_signal_connect(stage,
	                 "button-press-event",
	                 G_CALLBACK(ppg_session_view_button_press_event),
	                 view);

	g_signal_connect(priv->rows_box,
	                 "notify::allocation",
	                 G_CALLBACK(ppg_session_view_rows_allocation_notify),
	                 view);

	g_signal_connect(priv->vadj,
	                 "value-changed",
	                 G_CALLBACK(ppg_session_view_vadj_changed),
	                 view);

	g_signal_connect(priv->zadj,
	                 "value-changed",
	                 G_CALLBACK(ppg_session_view_zoom_changed),
	                 view);

	ppg_session_view_zoom_changed(priv->zadj, view);

#if 1
	ppg_session_view_add_row(view, "Memory Sampler");
	ppg_session_view_add_row(view, "Row 1");
	ppg_session_view_add_row(view, "Row 2");
	ppg_session_view_add_row(view, "Row 3");
	ppg_session_view_add_row(view, "Row 4");
	ppg_session_view_add_row(view, "Row 5");
	ppg_session_view_add_row(view, "Row 6");
	ppg_session_view_add_row(view, "Row 7");
	ppg_session_view_add_row(view, "Row 8");
#endif
}
