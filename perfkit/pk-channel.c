/* pk-channel.c
 *
 * Copyright (C) 2009 Christian Hergert <chris@dronelabs.com>
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

#include <dbus/dbus-glib.h>

#include "pk-channel.h"
#include "pk-connection.h"
#include "pk-connection-priv.h"

G_DEFINE_TYPE (PkChannel, pk_channel, G_TYPE_OBJECT)

struct _PkChannelPrivate
{
	PkConnection *connection;
	gint          channel_id;
};

static void
pk_channel_finalize (GObject *object)
{
	G_OBJECT_CLASS (pk_channel_parent_class)->finalize (object);
}

static void
pk_channel_class_init (PkChannelClass *klass)
{
	GObjectClass *object_class;

	object_class = G_OBJECT_CLASS (klass);
	object_class->finalize = pk_channel_finalize;
	g_type_class_add_private (object_class, sizeof (PkChannelPrivate));
}

static void
pk_channel_init (PkChannel *channel)
{
	channel->priv = G_TYPE_INSTANCE_GET_PRIVATE (channel,
	                                             PK_TYPE_CHANNEL,
	                                             PkChannelPrivate);
}

PkChannel*
pk_channel_new (PkConnection *connection,
                gint          channel_id)
{
	PkChannel *channel;

	channel = g_object_new (PK_TYPE_CHANNEL, NULL);
	channel->priv->channel_id = channel_id;
	g_object_add_weak_pointer (G_OBJECT (connection),
	                           (gpointer*)&channel->priv->connection);

	return channel;
}
