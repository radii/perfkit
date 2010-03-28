/* pk-source.c
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

#include "pk-connection.h"
#include "pk-source.h"

G_DEFINE_TYPE(PkSource, pk_source, G_TYPE_OBJECT)

/**
 * SECTION:pk-source
 * @title: PkSource
 * @short_description: 
 *
 * 
 */

struct _PkSourcePrivate
{
	PkConnection *conn;
	gint id;
};

enum
{
	PROP_0,
	PROP_CONN,
	PROP_ID,
};

gint
pk_source_get_id (PkSource *source)
{
	g_return_val_if_fail(PK_IS_SOURCE(source), -1);
	return source->priv->id;
}

static void
pk_source_finalize (GObject *object)
{
	G_OBJECT_CLASS(pk_source_parent_class)->finalize(object);
}

static void
pk_source_dispose (GObject *object)
{
	G_OBJECT_CLASS(pk_source_parent_class)->dispose(object);
}

static void
pk_source_get_property (GObject    *object,
                        guint       prop_id,
                        GValue     *value,
                        GParamSpec *pspec)
{
	switch (prop_id) {
	case PROP_ID:
		g_value_set_int(value, pk_source_get_id(PK_SOURCE(object)));
		break;
	case PROP_CONN:
		g_value_set_object(value, PK_SOURCE(object)->priv->conn);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
	}
}

static void
pk_source_set_property (GObject      *object,
                        guint         prop_id,
                        const GValue *value,
                        GParamSpec   *pspec)
{
	switch (prop_id) {
	case PROP_ID:
		PK_SOURCE(object)->priv->id = g_value_get_int(value);
		break;
	case PROP_CONN:
		PK_SOURCE(object)->priv->conn = g_value_dup_object(value);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
	}
}

static void
pk_source_class_init (PkSourceClass *klass)
{
	GObjectClass *object_class;

	object_class = G_OBJECT_CLASS (klass);
	object_class->finalize = pk_source_finalize;
	object_class->dispose = pk_source_dispose;
	object_class->set_property = pk_source_set_property;
	object_class->get_property = pk_source_get_property;
	g_type_class_add_private(object_class, sizeof(PkSourcePrivate));

	g_object_class_install_property(object_class,
	                                PROP_ID,
	                                g_param_spec_int("id",
	                                                 "Id",
	                                                 "The source id",
	                                                 0,
	                                                 G_MAXINT,
	                                                 0,
	                                                 G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));

	g_object_class_install_property(object_class,
	                                PROP_CONN,
	                                g_param_spec_object("connection",
	                                                    "Connection",
	                                                    "The source connection",
	                                                    PK_TYPE_CONNECTION,
	                                                    G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
}

static void
pk_source_init (PkSource *source)
{
	source->priv = G_TYPE_INSTANCE_GET_PRIVATE(source,
	                                           PK_TYPE_SOURCE,
	                                           PkSourcePrivate);
}
