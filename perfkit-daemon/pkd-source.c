/* pkd-source.c
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
#include "config.h"
#endif

#include <dbus/dbus-glib.h>

#include "pkd-channel-priv.h"
#include "pkd-runtime.h"
#include "pkd-source.h"
#include "pkd-source-glue.h"
#include "pkd-source-dbus.h"

/**
 * SECTION:pkd-source
 * @title: PkdSource
 * @short_description: Data collection sources
 *
 * #PkdSource represents a data collection source.  Sources are responsible
 * for creating samples which contain their collected data.
 */

G_DEFINE_ABSTRACT_TYPE (PkdSource, pkd_source, G_TYPE_OBJECT)

struct _PkdSourcePrivate
{
	gint         id;
	PkdChannel *channel;
};

static gint source_seq = 0;

static gboolean
noop_needs_spawn (PkdSource *source)
{
	return FALSE;
}

static gboolean
noop_start (PkdSource  *source,
            GError    **error)
{
	return TRUE;
}

static void
noop_stop (PkdSource *source)
{
}

static void
noop_pause (PkdSource *source)
{
}

static void
noop_unpause (PkdSource *source)
{
}

static void
pkd_source_finalize (GObject *object)
{
	G_OBJECT_CLASS (pkd_source_parent_class)->finalize (object);
}

static void
pkd_source_class_init (PkdSourceClass *klass)
{
	GObjectClass *object_class;

	object_class = G_OBJECT_CLASS (klass);
	object_class->finalize = pkd_source_finalize;
	g_type_class_add_private (object_class, sizeof (PkdSourcePrivate));

	klass->needs_spawn = noop_needs_spawn;
	klass->start = noop_start;
	klass->stop = noop_stop;
	klass->pause = noop_pause;
	klass->unpause = noop_unpause;

	dbus_g_object_type_install_info (PKD_TYPE_SOURCE, &dbus_glib_pkd_source_object_info);
}

static void
pkd_source_init (PkdSource *source)
{
	gchar *path;

	source->priv = G_TYPE_INSTANCE_GET_PRIVATE (source,
	                                            PKD_TYPE_SOURCE,
	                                            PkdSourcePrivate);

	source->priv->id = g_atomic_int_exchange_and_add (&source_seq, 1);

	path = g_strdup_printf ("/com/dronelabs/Perfkit/Sources/%d",
	                        source->priv->id);
	dbus_g_connection_register_g_object (pkd_runtime_get_connection (),
	                                     path,
	                                     G_OBJECT (source));
	g_free (path);
}

/**
 * pkd_source_new:
 *
 * Creates a new instance of #PkdSource.
 *
 * Return value: the newly created #PkdSource instance.
 */
PkdSource*
pkd_source_new (void)
{
	return g_object_new (PKD_TYPE_SOURCE, NULL);
}

/**
 * pkd_source_get_id:
 * @source: A #PkdSource
 *
 * Retrieves the identifier of a data source.
 *
 * Return value: the source identifier.
 */
gint
pkd_source_get_id (PkdSource *source)
{
	g_return_val_if_fail (PKD_IS_SOURCE (source), -1);
	return source->priv->id;
}

/**
 * pkd_source_needs_spawn:
 * @source: A #PkdSource
 *
 * Checks to see if the source needs to spawn the child process to function
 * correctly.
 *
 * Return value: TRUE if the source is required to spawn the child process
 */
gboolean
pkd_source_needs_spawn (PkdSource *source)
{
	g_return_val_if_fail (PKD_IS_SOURCE (source), FALSE);
	return PKD_SOURCE_GET_CLASS (source)->needs_spawn (source);
}

/**
 * pkd_source_spawn:
 * @source: A #PkdSource
 *
 * Spawns the target executable if needed.
 *
 * Return value: %TRUE if the child is spawned.
 */
gboolean
pkd_source_spawn (PkdSource  *source,
                  GError    **error)
{
	g_return_val_if_fail (PKD_IS_SOURCE (source), FALSE);
	return PKD_SOURCE_GET_CLASS (source)->spawn (source, error);
}

/**
 * pkd_source_start:
 * @source: A #PkdSource
 * @error: a location for a #GError
 *
 * Starts the data source recording samples.
 *
 * Return value: %TRUE on success
 */
gboolean
pkd_source_start (PkdSource  *source,
                  GError    **error)
{
	g_return_val_if_fail (PKD_IS_SOURCE (source), FALSE);
	return PKD_SOURCE_GET_CLASS (source)->start (source, error);
}

/**
 * pkd_source_stop:
 * @source: A #PkdSource
 *
 * Stops the data source from recording samples.
 */
void
pkd_source_stop (PkdSource *source)
{
	g_return_if_fail (PKD_IS_SOURCE (source));
	PKD_SOURCE_GET_CLASS (source)->stop (source);
}

/**
 * pkd_source_pause:
 * @source: A #PkdSource
 *
 * Pauses a #PkdSource, preventing new data samples from being created.
 */
void
pkd_source_pause (PkdSource *source)
{
	g_return_if_fail (PKD_IS_SOURCE (source));
	if (PKD_SOURCE_GET_CLASS (source)->pause)
		PKD_SOURCE_GET_CLASS (source)->pause (source);
}

/**
 * pkd_source_unpause:
 * @source: A #PkdSource
 *
 * Unpauses a #PkdSource, allowing new data samples to be created.
 */
void
pkd_source_unpause (PkdSource *source)
{
	g_return_if_fail (PKD_IS_SOURCE (source));
	if (PKD_SOURCE_GET_CLASS (source)->unpause)
		PKD_SOURCE_GET_CLASS (source)->unpause (source);
}

/**
 * pkd_source_set_channel:
 * @source: A #PkdSource
 * @channel: A #PkdChannel
 *
 * Sets the channel for the source to deliver samples to.  This may only
 * be called once and subsequent calls will result in an error being
 * printed to the error console.
 */
void
pkd_source_set_channel (PkdSource  *source,
                        PkdChannel *channel)
{
	PkdSourcePrivate *priv;

	g_return_if_fail (PKD_IS_SOURCE (source));
	g_return_if_fail (PKD_IS_CHANNEL (channel));

	priv = source->priv;

	if (priv->channel) {
		g_warning ("Cannot set channel multiple times");
		return;
	}

	priv->channel = g_object_ref (channel);
	pkd_channel_add_source (channel, source);
}

/**
 * pkd_channel_get_channel:
 * @source: A #PkdSource
 *
 * Retrieves the channel in which the source delivers samples.
 *
 * Return value: A #PkdChannel or %NULL.
 */
PkdChannel*
pkd_source_get_channel (PkdSource *source)
{
	g_return_val_if_fail (PKD_IS_SOURCE (source), NULL);
	return source->priv->channel;
}

static gboolean
pkd_source_get_channel_dbus (PkdSource  *source,
                             gchar     **path,
                             GError    **error)
{
	PkdSourcePrivate *priv;

	g_return_val_if_fail (path != NULL, FALSE);

	priv = source->priv;

	if (!priv->channel)
		*path = g_strdup ("");
	else
		*path = g_strdup_printf ("/com/dronelabs/Perfkit/Channels/%d",
		                         pkd_channel_get_id (priv->channel));

	return TRUE;
}

static gboolean
pkd_source_set_channel_dbus (PkdSource    *source,
                             const gchar  *path,
                             GError      **error)
{
	PkdChannel *channel;

	g_return_val_if_fail (path != NULL, FALSE);

	channel = PKD_CHANNEL (dbus_g_connection_lookup_g_object (pkd_runtime_get_connection (), path));

	if (!channel)
		return FALSE;

	pkd_source_set_channel (source, channel);

	return TRUE;
}
