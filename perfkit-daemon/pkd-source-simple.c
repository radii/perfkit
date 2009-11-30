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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "pkd-source-simple.h"
#include "pkd-channel-priv.h"

G_DEFINE_TYPE (PkdSourceSimple, pkd_source_simple, PKD_TYPE_SOURCE)

struct _PkdSourceSimplePrivate
{
	PkdSourceSimpleSampleFunc  sample_func;
	GDestroyNotify             destroy;
	gpointer                   user_data;

	GMutex                    *mutex;
	GCond                     *cond;
	gboolean                   stopped;
	gboolean                   paused;
	gulong                     freq;
};

static void
pkd_source_simple_finalize (GObject *object)
{
	G_OBJECT_CLASS (pkd_source_simple_parent_class)->finalize (object);
}

static gpointer
thread_func (gpointer user_data)
{
	PkdSourceSimple        *simple;
	PkdSourceSimplePrivate *priv;
	PkdChannel             *channel;
	PkdSample              *sample;
	GError                 *error = NULL;
	GTimeVal                timeout;

	simple = user_data;
	priv = simple->priv;
	channel = pkd_source_get_channel (PKD_SOURCE (simple));

next_sample:
	if (g_atomic_int_get (&priv->stopped))
		return NULL;

	/* calculate the time of our subsequent sample so we redruce drift */
	g_get_current_time (&timeout);
	g_time_val_add (&timeout, priv->freq);

	/* execute our sampling */
	if (NULL != (sample = priv->sample_func (user_data, priv->user_data, &error))) {
		pkd_channel_deliver (channel, user_data, sample);
		pkd_sample_unref (sample);
	}

	if (error) {
		/* TODO: What should we do upon error */
		g_error_free (error);
		error = NULL;
	}

	if (g_atomic_int_get (&priv->stopped))
		return NULL;

	/* We need to sleep until our next sampling.  We will sleep indefinitely
	 * if we are paused.  Upon unpausing, we will get a signal on our condition
	 * to continue.  If we are not paused, we will sleep until our next
	 * sample timeout.
	 */

	g_mutex_lock (priv->mutex);
	if (priv->paused)
		g_cond_wait (priv->cond, priv->mutex);
	else
		g_cond_timed_wait (priv->cond, priv->mutex, &timeout);
	g_mutex_unlock (priv->mutex);

	goto next_sample;
}

static gboolean
pkd_source_simple_real_start (PkdSource  *source,
                              GError    **error)
{
	PkdSourceSimplePrivate *priv;

	g_return_val_if_fail (PKD_IS_SOURCE_SIMPLE (source), FALSE);

	priv = PKD_SOURCE_SIMPLE (source)->priv;

	if (!priv->sample_func) {
		g_warning ("No sample method installed to start on source %d",
		           pkd_source_get_id (source));
		return FALSE;
	}

	/* TODO: Make sure not started/paused */

	if (!g_thread_create (thread_func, source, FALSE, error))
		return FALSE;

	return TRUE;
}

static void
pkd_source_simple_real_stop (PkdSource *source)
{
	PkdSourceSimplePrivate *priv;

	g_return_if_fail (PKD_IS_SOURCE_SIMPLE (source));

	priv = PKD_SOURCE_SIMPLE (source)->priv;

	g_mutex_lock (priv->mutex);
	priv->stopped = TRUE;
	g_cond_signal (priv->cond);
	g_mutex_unlock (priv->mutex);
}

static void
pkd_source_simple_class_init (PkdSourceSimpleClass *klass)
{
	GObjectClass   *object_class;
	PkdSourceClass *source_class;

	object_class = G_OBJECT_CLASS (klass);
	object_class->finalize = pkd_source_simple_finalize;
	g_type_class_add_private (object_class, sizeof (PkdSourceSimplePrivate));

	source_class = PKD_SOURCE_CLASS (klass);
	source_class->start = pkd_source_simple_real_start;
	source_class->stop = pkd_source_simple_real_stop;
}

static void
pkd_source_simple_init (PkdSourceSimple *source)
{
	source->priv = G_TYPE_INSTANCE_GET_PRIVATE (source,
	                                            PKD_TYPE_SOURCE_SIMPLE,
	                                            PkdSourceSimplePrivate);

	source->priv->mutex = g_mutex_new ();
	source->priv->cond = g_cond_new ();

	/* FIXME: */
	source->priv->freq = G_USEC_PER_SEC * 2;
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
