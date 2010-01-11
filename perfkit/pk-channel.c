/* pk-channel.c
 *
 * Copyright (C) 2010 Christian Hergert
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "pk-channel.h"
#include "pk-connection.h"

G_DEFINE_TYPE(PkChannel, pk_channel, G_TYPE_OBJECT)

/**
 * SECTION:pk-channel
 * @title: PkChannel
 * @short_description: 
 *
 * 
 */

struct _PkChannelPrivate
{
	PkConnection *conn;
};

enum
{
	PROP_0,
	PROP_CONN,
};

static void
pk_channel_finalize (GObject *object)
{
	G_OBJECT_CLASS(pk_channel_parent_class)->finalize(object);
}

static void
pk_channel_dispose (GObject *object)
{
	G_OBJECT_CLASS(pk_channel_parent_class)->dispose(object);
}

static void
pk_channel_class_init (PkChannelClass *klass)
{
	GObjectClass *object_class;

	object_class = G_OBJECT_CLASS (klass);
	object_class->finalize = pk_channel_finalize;
	object_class->dispose = pk_channel_dispose;
	g_type_class_add_private (object_class, sizeof (PkChannelPrivate));
}

static void
pk_channel_init (PkChannel *channel)
{
	channel->priv = G_TYPE_INSTANCE_GET_PRIVATE(channel,
	                                            PK_TYPE_CHANNEL,
	                                            PkChannelPrivate);
}
