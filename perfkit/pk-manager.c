/* pk-manager.c
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

#include "perfkit-lowlevel.h"
#include "pk-connection.h"
#include "pk-manager.h"

G_DEFINE_TYPE(PkManager, pk_manager, G_TYPE_OBJECT)

/**
 * SECTION:pk-manager
 * @title: PkManager
 * @short_description: 
 *
 * 
 */

#define RPC(m,n,...)                               \
    PK_CONNECTION_GET_CLASS((m)->priv->conn)->     \
        manager_##n((m)->priv->conn, __VA_ARGS__)

struct _PkManagerPrivate
{
	PkConnection *conn;
};

enum
{
	PROP_0,
	PROP_CONN,
};


/**
 * pk_manager_ping:
 * @manager: A #PkManager
 * @tv: The time at which the agent responded.
 *
 * Pings the remote agent via the configured connection.  The time at
 * which the agent responded is stored in @tv.
 *
 * Return value: %TRUE if successful; otherwise %FALSE.
 *
 * Side effects: None.
 */
gboolean
pk_manager_ping (PkManager  *manager,
                 GTimeVal   *tv,
                 GError    **error)
{
	g_return_val_if_fail(PK_IS_MANAGER(manager), FALSE);
	return RPC(manager, ping, tv, error);
}

/**
 * pk_manager_get_channels:
 * @manager: A #PkManager
 * @channels: A location for a #GList.
 * @error: A location for a #GError or %NULL.
 *
 * Retrieves a list of channels from the remote agent.  The list returned is
 * a copy and the #PkChannel instances are reference counted.
 *
 * When done with the channels, they should be unref'd and the list freed.
 *
 * [[
 * g_list_foreach(list, (GFunc)g_object_unref, NULL);
 * g_list_free(list);
 * ]]
 *
 * Returns: A newly created list of #PkChannel.
 *
 * Side effects: None.
 */
gboolean
pk_manager_get_channels (PkManager  *manager,
                         GList     **channels,
                         GError    **error)
{
	g_return_val_if_fail(PK_IS_MANAGER(manager), FALSE);
	return RPC(manager, get_channels, channels, error);
}

PkChannel*
pk_manager_get_channel (PkManager *manager,
                        gint       channel_id)
{
	PkManagerPrivate *priv;

	g_return_val_if_fail(PK_IS_MANAGER(manager), NULL);

	priv = manager->priv;

	return g_object_new(PK_TYPE_CHANNEL,
	                    "connection", priv->conn,
	                    "id", channel_id,
	                    NULL);
}

PkChannel*
pk_manager_create_channel (PkManager    *manager,
                           PkSpawnInfo  *spawn_info,
                           GError      **error)
{
	PkManagerPrivate *priv;
	gint channel_id = 0;

	g_return_val_if_fail(PK_IS_MANAGER(manager), NULL);

	priv = manager->priv;

	if (!pk_connection_manager_create_channel(priv->conn,
	                                          spawn_info,
	                                          &channel_id,
	                                          NULL))
	    return NULL;

	return g_object_new(PK_TYPE_CHANNEL,
	                    "connection", priv->conn,
	                    "id", channel_id,
	                    NULL);
}

static void
pk_manager_finalize (GObject *object)
{
	G_OBJECT_CLASS(pk_manager_parent_class)->finalize(object);
}

static void
pk_manager_dispose (GObject *object)
{
	G_OBJECT_CLASS(pk_manager_parent_class)->dispose(object);
}

static void
pk_manager_set_property (GObject        *object,
                         guint           property_id,
                         const GValue   *value,
                         GParamSpec     *pspec)
{
	PkManagerPrivate *priv = PK_MANAGER(object)->priv;

	switch (property_id) {
	case PROP_CONN:
		priv->conn = g_object_ref(g_value_get_object(value));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
	}
}

static void
pk_manager_get_property (GObject    *object,
                         guint       property_id,
                         GValue     *value,
                         GParamSpec *pspec)
{
	switch (property_id) {
	case PROP_CONN:
		g_value_set_object(value, PK_MANAGER(object)->priv->conn);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
	}
}

static void
pk_manager_class_init (PkManagerClass *klass)
{
	GObjectClass *object_class;

	object_class = G_OBJECT_CLASS (klass);
	object_class->finalize = pk_manager_finalize;
	object_class->dispose = pk_manager_dispose;
	object_class->get_property = pk_manager_get_property;
	object_class->set_property = pk_manager_set_property;
	g_type_class_add_private(object_class, sizeof(PkManagerPrivate));

	/**
	 * PkManager:connection:
	 * @connection: A #PkConnection
	 *
	 * The "connection" property.
	 */
	g_object_class_install_property(object_class,
	                                PROP_CONN,
	                                g_param_spec_object("connection",
	                                                    "connection",
	                                                    "Perfkit connection",
	                                                    PK_TYPE_CONNECTION,
	                                                    G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE));
}

static void
pk_manager_init (PkManager *manager)
{
	manager->priv = G_TYPE_INSTANCE_GET_PRIVATE(manager,
	                                            PK_TYPE_MANAGER,
	                                            PkManagerPrivate);
}
