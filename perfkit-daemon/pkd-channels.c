/* pkd-channels.c
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

#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include "pkd-channel.h"
#include "pkd-channels.h"
#include "pkd-service.h"

/**
 * SECTION:pkd-channels
 * @title: PkdChannels
 * @short_description: Runtime management of #PkdChannel<!-- -->s
 *
 * #PkdChannels provides runtime management of #PkdChannel instances.  Channels
 * are exposed over the DBUS.
 */

static void pkd_channels_init_service (PkdServiceIface *iface);

G_DEFINE_TYPE_EXTENDED (PkdChannels, pkd_channels, G_TYPE_OBJECT, 0,
                        G_IMPLEMENT_INTERFACE (PKD_TYPE_SERVICE,
                                               pkd_channels_init_service));

struct _PkdChannelsPrivate
{
	GStaticRWLock  rw_lock;
	GHashTable    *channels;
};

enum
{
	CHANNEL_ADDED,
	LAST_SIGNAL
};

static guint signals [LAST_SIGNAL];

/**
 * pkd_channels_add:
 * @channels: A #PkdChannels
 *
 * Adds a new #PkdChannel.
 *
 * Return value: the newly created #PkdChannel instance.
 *
 * Side effects: The created channel is cached in the #PkdChannels instance
 *   for future access.
 */
PkdChannel*
pkd_channels_add (PkdChannels *channels)
{
	PkdChannelsPrivate *priv;
	PkdChannel         *channel;
	gint               *id;

	g_return_val_if_fail (PKD_IS_CHANNELS (channels), NULL);

	priv = channels->priv;

	channel = g_object_new (PKD_TYPE_CHANNEL, NULL);
	id = g_malloc (sizeof (gint));
	*id = pkd_channel_get_id (channel);

	g_static_rw_lock_writer_lock (&priv->rw_lock);
	g_hash_table_insert (priv->channels, id, channel);
	g_static_rw_lock_writer_unlock (&priv->rw_lock);

	g_signal_emit (channels, signals [CHANNEL_ADDED], 0, channel);

	return channel;
}

/**
 * pkd_channels_remove:
 * @channels: A #PkdChannels
 *
 * Removes an existing #PkdChannel.  When the channel is freed, it will
 * automatically be removed from the DBUS.
 *
 * Side effects: Removes the #PkdChannel from the #PkdChannels channel
 *   cache.
 */
void
pkd_channels_remove (PkdChannels *channels,
                     PkdChannel  *channel)
{
	PkdChannelsPrivate *priv;
	gint                id;

	g_return_if_fail (PKD_IS_CHANNELS (channels));
	g_return_if_fail (PKD_IS_CHANNEL (channel));

	priv = channels->priv;
	id = pkd_channel_get_id (channel);

	g_static_rw_lock_writer_lock (&priv->rw_lock);
	g_hash_table_remove (priv->channels, &id);
	g_static_rw_lock_writer_unlock (&priv->rw_lock);
}

/**
 * pkd_channels_find_all:
 * @channels: A #PkdChannels
 *
 * Retrieves all of the 
 *
 * Return value: a newly created #GList containing #PkdChannel<!-- -->'s.  The
 *   list should be freed by decrementing each item in the list with
 *   g_object_unref() and then g_list_free().
 *
 * Side effects: None
 */
GList*
pkd_channels_find_all (PkdChannels *channels)
{
	PkdChannelsPrivate *priv;
	GHashTableIter      iter;
	GList              *list = NULL;
	gpointer            key, value;

	g_return_val_if_fail (PKD_IS_CHANNELS (channels), NULL);

	priv = channels->priv;

	g_static_rw_lock_reader_lock (&priv->rw_lock);

	g_hash_table_iter_init (&iter, priv->channels);
	while (g_hash_table_iter_next (&iter, &key, &value))
		list = g_list_prepend (list, g_object_ref (value));

	g_static_rw_lock_reader_unlock (&priv->rw_lock);

	return list;
}

/**
 * pkd_channels_get:
 * @channels: A #PkdChannels
 * @channel_id: the channel identifier
 *
 * Retrieves a #PkdChannel for the given channel id.
 *
 * Return value:
 *       An instance of #PkdChannel if successful; otherwise %NULL.  The caller
 *       owns a reference to the resulting #PkdChannel and should unref it with
 *       g_object_unref().
 *
 * Side effects:
 *       None.
 */
PkdChannel*
pkd_channels_get (PkdChannels *channels,
                  gint         channel_id)
{
	PkdChannelsPrivate *priv;
	PkdChannel *channel;

	g_return_val_if_fail (PKD_IS_CHANNELS (channels), NULL);

	priv = channels->priv;

	g_static_rw_lock_reader_lock (&priv->rw_lock);
	channel = g_hash_table_lookup (priv->channels, &channel_id);
	if (channel)
		g_object_ref (channel);
	g_static_rw_lock_reader_unlock (&priv->rw_lock);

	return channel;
}

GQuark
pkd_channels_error_quark (void)
{
	return g_quark_from_static_string ("pkd-channels-error");
}

/**************************************************************************
 *                         GObject Class Methods                          *
 **************************************************************************/

static void
pkd_channels_finalize (GObject *object)
{
	PkdChannelsPrivate *priv;

	priv = PKD_CHANNELS (object)->priv;

	g_hash_table_destroy (priv->channels);
	priv->channels = NULL;

	G_OBJECT_CLASS (pkd_channels_parent_class)->finalize (object);
}

static void
pkd_channels_class_init (PkdChannelsClass *klass)
{
	GObjectClass *object_class;

	object_class = G_OBJECT_CLASS (klass);
	object_class->finalize = pkd_channels_finalize;
	g_type_class_add_private (object_class, sizeof (PkdChannelsPrivate));

	/**
	 * PkdChannels::channel-added:
	 * @channel: A #PkdChannel
	 *
	 * The "channel-added" signal.  This signal is emitted when a new
	 * channel is created.
	 */
	signals [CHANNEL_ADDED] = g_signal_new ("channel-added",
	                                        PKD_TYPE_CHANNELS,
	                                        G_SIGNAL_RUN_FIRST,
	                                        0,
	                                        NULL,
	                                        NULL,
	                                        g_cclosure_marshal_VOID__OBJECT,
	                                        G_TYPE_NONE,
	                                        1,
	                                        PKD_TYPE_CHANNEL);
}

static void
pkd_channels_init (PkdChannels *channels)
{
	channels->priv = G_TYPE_INSTANCE_GET_PRIVATE ((channels),
	                                              PKD_TYPE_CHANNELS,
	                                              PkdChannelsPrivate);

	g_static_rw_lock_init (&channels->priv->rw_lock);
	channels->priv->channels = g_hash_table_new_full (g_int_hash, g_int_equal,
	                                                  g_free, g_object_unref);
}

static void
pkd_channels_init_service (PkdServiceIface *iface)
{
}
