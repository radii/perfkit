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
	GtkWidget    *embed;
	ClutterActor *stage;
	ClutterActor *bg;
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
	ClutterCairoTexture *ct;
	cairo_t *cr;
	gfloat width;
	gfloat height;

	ENTRY;
	priv = scatter->priv;
	if (!priv->bg) {
		priv->bg = clutter_cairo_texture_new(1, 1);
		clutter_container_add_actor(CLUTTER_CONTAINER(priv->stage), priv->bg);
		clutter_actor_show(priv->bg);
	}
	ct = CLUTTER_CAIRO_TEXTURE(priv->bg);
	clutter_cairo_texture_clear(ct);
	clutter_actor_get_size(priv->bg, &width, &height);
	cr = clutter_cairo_texture_create(ct);
	cairo_rectangle(cr, 0, 0, width, height);
	cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
	cairo_fill(cr);
	cairo_move_to(cr, 0, 0);
	cairo_line_to(cr, width, height);
	cairo_set_source_rgb(cr, 0, 0, 0);
	cairo_stroke(cr);
	cairo_destroy(cr);
	EXIT;
}

static void
pkc_scatter_size_allocate (GtkWidget     *scatter,
                           GtkAllocation *allocation)
{
	PkcScatterPrivate *priv;

	ENTRY;
	priv = PKC_SCATTER(scatter)->priv;
	GTK_WIDGET_CLASS(pkc_scatter_parent_class)->size_allocate(scatter, allocation);
	clutter_cairo_texture_set_surface_size(CLUTTER_CAIRO_TEXTURE(priv->bg),
	                                       allocation->width,
	                                       allocation->height);
	clutter_actor_set_size(priv->bg, allocation->width, allocation->height);
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
	 * Create core layers.
	 */
	pkc_scatter_update_background(scatter);

	EXIT;
}
