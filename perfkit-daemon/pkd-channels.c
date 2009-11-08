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

#include "pkd-channels.h"
#include "pkd-channels-dbus.h"

/**
 * SECTION:pkd-channels
 * @title: PkdChannels
 * @short_description: Runtime management of #PkdChannel<!-- -->s
 *
 * #PkdChannels provides runtime management of #PkdChannel instances.  Channels
 * are exposed over the DBUS.
 */

G_DEFINE_TYPE (PkdChannels, pkd_channels, G_TYPE_OBJECT)

struct _PkdChannelsPrivate
{
	GHashTable *channels;
};

static void
pkd_channels_finalize (GObject *object)
{
	G_OBJECT_CLASS (pkd_channels_parent_class)->finalize (object);
}

static void
pkd_channels_class_init (PkdChannelsClass *klass)
{
	GObjectClass *object_class;

	object_class = G_OBJECT_CLASS (klass);
	object_class->finalize = pkd_channels_finalize;
	g_type_class_add_private (object_class, sizeof (PkdChannelsPrivate));

	dbus_g_object_type_install_info (PKD_TYPE_CHANNELS, &dbus_glib_pkd_channels_object_info);
}

static void
pkd_channels_init (PkdChannels *channels)
{
	channels->priv = G_TYPE_INSTANCE_GET_PRIVATE ((channels),
	                                              PKD_TYPE_CHANNELS,
	                                              PkdChannelsPrivate);
	channels->priv->channels = g_hash_table_new_full (g_int_hash, g_int_equal,
	                                                  g_free, g_object_unref);
}

/**
 * pkd_channels_new:
 *
 * Creates a new instance of #PkdChannels.
 *
 * Return value: the newly created #PkdChannels instance.
 */
PkdChannels*
pkd_channels_new (void)
{
	return g_object_new (PKD_TYPE_CHANNELS, NULL);
}

