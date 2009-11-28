/* pkd-source-simple.c
 * 
 * Copyright (C) 2009 Christian Hergert
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

#include "pkd-source-simple.h"

struct _PkdSourceSimplePrivate
{
	PkdSourceSimpleSampleFunc  sample_func;
	GDestroyNotify             destroy;
	gpointer                   user_data;
};

G_DEFINE_TYPE (PkdSourceSimple, pkd_source_simple, PKD_TYPE_SOURCE)

static void
pkd_source_simple_finalize (GObject *object)
{
	G_OBJECT_CLASS (pkd_source_simple_parent_class)->finalize (object);
}

static void
pkd_source_simple_class_init (PkdSourceSimpleClass *klass)
{
	GObjectClass *object_class;

	object_class = G_OBJECT_CLASS (klass);
	object_class->finalize = pkd_source_simple_finalize;
	g_type_class_add_private (object_class, sizeof (PkdSourceSimplePrivate));
}

static void
pkd_source_simple_init (PkdSourceSimple *source)
{
	source->priv = G_TYPE_INSTANCE_GET_PRIVATE (source,
	                                            PKD_TYPE_SOURCE_SIMPLE,
	                                            PkdSourceSimplePrivate);
}

/**
 * pkd_source_simple_new:
 *
 * Creates a new simple source.  Use pkd_source_simple_set_sample_func() to
 * set the callback to be called to generate a sample.
 *
 * Return value: the newly created #PkdSourceSimple.
 */
PkdSource*
pkd_source_simple_new (void)
{
	return g_object_new (PKD_TYPE_SOURCE_SIMPLE, NULL);
}

/**
 * pkd_source_simple_set_sample_func:
 * @source: A #PkdSourceSimple
 * @sample_func: A #PkdSourceSimpleSampleFunc
 * @user_data: user data for @sample_func or %NULL
 * @destroy: A #GDestroyNotify or %NULL
 *
 * Sets the sample func to generate samples when the source needs them.
 */
void
pkd_source_simple_set_sample_func (PkdSourceSimple           *source,
                                   PkdSourceSimpleSampleFunc  sample_func,
                                   gpointer                   user_data,
                                   GDestroyNotify             destroy)
{
	PkdSourceSimplePrivate *priv;

	g_return_if_fail (PKD_IS_SOURCE_SIMPLE (source));

	priv = source->priv;

	if (priv->user_data && priv->destroy)
		priv->destroy (priv->user_data);

	priv->sample_func = sample_func;
	priv->user_data = user_data;
	priv->destroy = destroy;
}
