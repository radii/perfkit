/* pkc-scatter.c
 *
 * Copyright (C) 2010 Christian Hergert <chris@dronelabs.com>
 *
 * This file is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 * 
 * This file is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#if HAVE_CONFIG_H
#include "config.h"
#endif

#include <clutter/clutter.h>
#include <clutter-gtk/clutter-gtk.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>

#include "pkc-log.h"
#include "pkc-scatter.h"

/**
 * SECTION:pkc-scatter
 * @title: PkcScatter
 * @short_description: 
 *
 * Section overview.
 */

G_DEFINE_TYPE(PkcScatter, pkc_scatter, GTK_TYPE_ALIGNMENT)

struct _PkcScatterPrivate
{
	GtkWidget     *embed;
	ClutterActor  *stage;
	ClutterActor  *bg;

	ClutterActor  *legend;
	ClutterActor  *legend_label;
	ClutterActor  *legend_label2;

	GtkAdjustment *x_adj;
	GtkAdjustment *y_adj;
};

/**
 * pkc_scatter_new:
 *
 * Creates a new instance of #PkcScatter.
 *
 * Returns: the newly created instance of #PkcScatter.
 * Side effects: None.
 */
GtkWidget*
pkc_scatter_new (void)
{
	GtkWidget *scatter;

	ENTRY;
	scatter = g_object_new(PKC_TYPE_SCATTER, NULL);
	RETURN(scatter);
}

static void
pkc_scatter_update_background (PkcScatter *scatter) /* IN */
{
	PkcScatterPrivate *priv;
	ClutterCairoTexture *texture;
	gdouble dashes[] = { 1., 2. };
	GdkColor bg_color;
	gfloat x0, x1, x2, x3;
	gfloat y0, y1, y2, y3;
	GtkAllocation alloc;
	gfloat w, h;
	cairo_t *cr;

	#define LABEL_HEIGHT 24
	#define TICK_HEIGHT  15

	g_return_if_fail(PKC_IS_SCATTER(scatter));

	ENTRY;
	priv = scatter->priv;
	if (!priv->bg) {
		priv->bg = clutter_cairo_texture_new(1, 1);
		clutter_container_add(CLUTTER_CONTAINER(priv->stage), priv->bg, NULL);
		clutter_actor_show(priv->bg);
	}

	texture = CLUTTER_CAIRO_TEXTURE(priv->bg);
	cr = clutter_cairo_texture_create(texture);

	/*
	 * Calculate regions.
	 */
	gtk_widget_get_allocation(priv->embed, &alloc);
	w = alloc.width;
	h = alloc.height;
	x0 = .5;
	x1 = x0 + LABEL_HEIGHT;
	x2 = x1 + TICK_HEIGHT;
	x3 = ((gint)w) - .5;
	y0 = ((gint)h) - .5;
	y1 = y0 - LABEL_HEIGHT;
	y2 = y1 - TICK_HEIGHT;
	y3 = .5;

	DEBUG(Scatter, "x0=%f x1=%f x2=%f x3=%f", x0, x1, x2, x3);
	DEBUG(Scatter, "y0=%f y1=%f y2=%f y3=%f", y0, y1, y2, y3);

	/*
	 * Set background to widget default bg color.
	 */
	bg_color = gtk_widget_get_style(GTK_WIDGET(scatter))->bg[GTK_STATE_NORMAL];
	cairo_rectangle(cr, 0, 0, w, h);
	gdk_cairo_set_source_color(cr, &bg_color);
	cairo_fill(cr);

	/*
	 * Set the scatter background to white.
	 */
	cairo_set_line_width(cr, 1.0);
	cairo_rectangle(cr, x2, y3, x3 - x2, y2 - y3);
	cairo_set_source_rgb(cr, 1, 1, 1);
	cairo_fill_preserve(cr);

	/*
	 * Draw border basic border.
	 */
	cairo_set_source_rgb(cr, 0, 0, 0);
	cairo_set_dash(cr, dashes, 2, 0);
	cairo_stroke(cr);

	/*
	 * Draw Y-axis ticks.
	 */
	cairo_move_to(cr, x1, y3);
	cairo_line_to(cr, x2, y3);
	cairo_move_to(cr, x1, y2);
	cairo_line_to(cr, x2, y2);
	cairo_stroke(cr);

	/*
	 * Draw X-axis ticks.
	 */
	cairo_move_to(cr, x2, y2);
	cairo_line_to(cr, x2, y1);
	cairo_move_to(cr, x3, y2);
	cairo_line_to(cr, x3, y1);
	cairo_stroke(cr);

	cairo_destroy(cr);
	EXIT;
}

static void
pkc_scatter_update_legend (PkcScatter *scatter)
{
	PkcScatterPrivate *priv;
	ClutterColor black = { 0, 0, 0, 0xFF };
	ClutterColor white = { 0xFF, 0xFF, 0xFF, 0xFF };

	#define TAB_WIDTH 120
	#define TAB_HEIGHT 24

	ENTRY;
	priv = scatter->priv;
	if (!priv->legend) {
		priv->legend = clutter_rectangle_new_with_color(&black);
		clutter_actor_set_size(priv->legend, TAB_WIDTH, TAB_HEIGHT);
		clutter_container_add_actor(CLUTTER_CONTAINER(priv->stage), priv->legend);
		clutter_actor_set_opacity(priv->legend, 0x99);
		clutter_actor_show(priv->legend);
		priv->legend_label = clutter_text_new_full("Monospace", _("Legend"), &white);
		priv->legend_label2 = clutter_text_new_full("Monospace", _("Legend"), &black);
		clutter_container_add(CLUTTER_CONTAINER(priv->stage), priv->legend_label2, priv->legend_label, NULL);
		clutter_actor_show(priv->legend_label);
		clutter_actor_show(priv->legend_label2);
	}
	EXIT;
}

static void
pkc_scatter_size_allocate (GtkWidget     *scatter,
                           GtkAllocation *allocation)
{
	PkcScatterPrivate *priv;
	gfloat w, h;

	ENTRY;
	priv = PKC_SCATTER(scatter)->priv;
	GTK_WIDGET_CLASS(pkc_scatter_parent_class)->size_allocate(scatter, allocation);
	clutter_cairo_texture_set_surface_size(CLUTTER_CAIRO_TEXTURE(priv->bg),
	                                       allocation->width,
	                                       allocation->height);
	clutter_actor_set_size(priv->bg, allocation->width, allocation->height);
	clutter_actor_set_position(priv->legend, allocation->width - TAB_WIDTH, 0);
	clutter_actor_get_size(priv->legend_label, &w, &h);
	clutter_actor_set_position(priv->legend_label,
	                           allocation->width - TAB_WIDTH + (w / 2), 0);
	clutter_actor_set_position(priv->legend_label2,
	                           allocation->width - TAB_WIDTH + (w / 2) + 1, 1);
	pkc_scatter_update_background(PKC_SCATTER(scatter));
	EXIT;
}

static void
pkc_scatter_style_set (GtkWidget *scatter,
                       GtkStyle  *style)
{
	ENTRY;
	GTK_WIDGET_CLASS(pkc_scatter_parent_class)->style_set(scatter, style);
	pkc_scatter_update_background(PKC_SCATTER(scatter));
	EXIT;
}
                           

/**
 * pkc_scatter_finalize:
 * @object: A #PkcScatter.
 *
 * Finalizer for a #PkcScatter instance.  Frees any resources held by
 * the instance.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
pkc_scatter_finalize (GObject *object) /* IN */
{
	ENTRY;
	G_OBJECT_CLASS(pkc_scatter_parent_class)->finalize(object);
	EXIT;
}

/**
 * pkc_scatter_class_init:
 * @klass: A #PkcScatterClass.
 *
 * Initializes the #PkcScatterClass and prepares the vtable.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
pkc_scatter_class_init (PkcScatterClass *klass) /* IN */
{
	GObjectClass *object_class;
	GtkWidgetClass *widget_class;

	ENTRY;
	object_class = G_OBJECT_CLASS(klass);
	object_class->finalize = pkc_scatter_finalize;
	g_type_class_add_private(object_class, sizeof(PkcScatterPrivate));
	widget_class = GTK_WIDGET_CLASS(klass);
	widget_class->size_allocate = pkc_scatter_size_allocate;
	widget_class->style_set = pkc_scatter_style_set;
	EXIT;
}

/**
 * pkc_scatter_init:
 * @scatter: A #PkcScatter.
 *
 * Initializes the newly created #PkcScatter instance.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
pkc_scatter_init (PkcScatter *scatter) /* IN */
{
	PkcScatterPrivate *priv;

	ENTRY;
	scatter->priv = G_TYPE_INSTANCE_GET_PRIVATE(scatter,
	                                            PKC_TYPE_SCATTER,
	                                            PkcScatterPrivate);
	priv = scatter->priv;

	/*
	 * Create embedded clutter stage.
	 */
	priv->embed = gtk_clutter_embed_new();
	priv->stage = gtk_clutter_embed_get_stage(GTK_CLUTTER_EMBED(priv->embed));
	gtk_container_add(GTK_CONTAINER(scatter), priv->embed);
	gtk_widget_show(priv->embed);

	/*
	 * Create scale ranges.
	 */
	priv->x_adj = GTK_ADJUSTMENT(gtk_adjustment_new(0, 0, 0, 0, 0, 0));
	priv->y_adj = GTK_ADJUSTMENT(gtk_adjustment_new(0, 0, 0, 0, 0, 0));

	/*
	 * Create core layers.
	 */
	pkc_scatter_update_background(scatter);
	pkc_scatter_update_legend(scatter);

	EXIT;
}
