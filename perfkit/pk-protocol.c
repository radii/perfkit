/* pk-protocol.c
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

#include "pk-protocol.h"

G_DEFINE_TYPE(PkProtocol, pk_protocol, G_TYPE_OBJECT)

/**
 * SECTION:pk-protocol
 * @title: PkProtocol
 * @short_description: 
 *
 * 
 */

struct _PkProtocolPrivate
{
	gchar *uri;
};

enum
{
	PROP_0,
	PROP_URI,
};

/**
 * pk_protocol_get_uri:
 * @protocol: A #PkProtocol
 *
 * Retrieves the uri of the protocol.
 *
 * Returns: the uri string.
 */
const gchar*
pk_protocol_get_uri (PkProtocol *protocol)
{
	g_return_val_if_fail(PK_IS_PROTOCOL(protocol), NULL);
	return protocol->priv->uri;
}

static void
pk_protocol_set_property (GObject      *object,
                          guint         prop_id,
                          const GValue *value,
                          GParamSpec   *pspec)
{
	switch (prop_id) {
	case PROP_URI:
		PK_PROTOCOL(object)->priv->uri = g_value_dup_string(value);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
	}
}

static void
pk_protocol_get_property (GObject    *object,
                          guint       prop_id,
                          GValue     *value,
                          GParamSpec *pspec)
{
	switch (prop_id) {
	case PROP_URI:
		g_value_set_string(value, PK_PROTOCOL(object)->priv->uri);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
	}
}

static void
pk_protocol_finalize (GObject *object)
{
	PkProtocolPrivate *priv = PK_PROTOCOL(object)->priv;

	g_free(priv->uri);

	G_OBJECT_CLASS (pk_protocol_parent_class)->finalize (object);
}

static void
pk_protocol_class_init (PkProtocolClass *klass)
{
	GObjectClass *object_class;

	object_class = G_OBJECT_CLASS (klass);
	object_class->finalize = pk_protocol_finalize;
	object_class->set_property = pk_protocol_set_property;
	object_class->get_property = pk_protocol_get_property;
	g_type_class_add_private (object_class, sizeof (PkProtocolPrivate));

	/**
	 * PkProtocol:uri:
	 *
	 * The "uri" property.
	 */
	g_object_class_install_property(object_class,
	                                PROP_URI,
	                                g_param_spec_string("uri",
	                                                    "uri",
	                                                    "Protocol Uri",
	                                                    NULL,
	                                                    G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
}

static void
pk_protocol_init (PkProtocol *protocol)
{
	protocol->priv = G_TYPE_INSTANCE_GET_PRIVATE(protocol,
	                                             PK_TYPE_PROTOCOL,
	                                             PkProtocolPrivate);
}
