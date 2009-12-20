/* pk-source-info.c
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <glib.h>
#include <glib-object.h>
#include <glib/gi18n.h>

#include "pk-connection.h"
#include "pk-connection-priv.h"
#include "pk-source-info.h"
#include "pk-source-info-priv.h"
#include "pk-source-priv.h"

G_DEFINE_TYPE (PkSourceInfo, pk_source_info, G_TYPE_OBJECT)

/**
 * SECTION:pk-source_info
 * @title: PkSourceInfo
 * @short_description: 
 *
 * 
 */

struct _PkSourceInfoPrivate
{
	PkConnection *conn;
	gchar        *uid;
};

/*
 *-----------------------------------------------------------------------------
 *
 * Public Methods
 *
 *-----------------------------------------------------------------------------
 */

PkSourceInfo*
pk_source_info_new (PkConnection *conn,
                    const gchar  *uid)
{
	PkSourceInfo *source_info;

	g_return_val_if_fail (PK_IS_CONNECTION (conn), NULL);
	g_return_val_if_fail (uid != NULL, NULL);

	source_info = g_object_new (PK_TYPE_SOURCE_INFO, NULL);
	source_info->priv->conn = g_object_ref (conn);
	source_info->priv->uid = g_strdup (uid);

	return source_info;
}

/**
 * pk_source_info_get_uid:
 * @source_info: A #PkSourceInfo
 *
 * Retrieves the "uid" property.
 *
 * Return value:
 *       A string containing the uid.  The string should be freed with
 *       g_free().
 *
 * Side effects:
 *       None.
 */
gchar*
pk_source_info_get_uid (PkSourceInfo *source_info)
{
	g_return_val_if_fail (PK_IS_SOURCE_INFO (source_info), NULL);
	return g_strdup (source_info->priv->uid);
}

/**
 * pk_source_info_get_name:
 * @source_info: A #PkSourceInfo
 *
 * Retrieves the "name" property.
 *
 * Return value:
 *       A string containing the name.  The string should be freed with
 *       g_free().
 *
 * Side effects:
 *       None.
 */
gchar*
pk_source_info_get_name (PkSourceInfo *source_info)
{
	g_return_val_if_fail (PK_IS_SOURCE_INFO (source_info), NULL);
	return pk_connection_source_info_get_name (source_info->priv->conn,
	                                           source_info->priv->uid);
}

/**
 * pk_source_info_create:
 * @source_info: A #PkSourceInfo
 *
 * Creates a new instance of a #PkSource for the given #PkSourceInfo.
 *
 * Return value:
 *       A proxy object for the created #PkSource.  The source should be
 *       freed with g_object_unref().
 *
 * Side effects:
 *       None.
 */
PkSource*
pk_source_info_create (PkSourceInfo *source_info)
{
	gint source_id;

	g_return_val_if_fail (PK_IS_SOURCE_INFO (source_info), NULL);

	source_id = pk_connection_source_info_create (source_info->priv->conn,
	                                              source_info->priv->uid);
	if (source_id >= 0) {
		return pk_source_new (source_info->priv->conn, source_id);
	}

	return NULL;
}

/*
 *-----------------------------------------------------------------------------
 *
 * Class Methods
 *
 *-----------------------------------------------------------------------------
 */

static void
pk_source_info_finalize (GObject *object)
{
	PkSourceInfoPrivate *priv;

	g_return_if_fail (PK_IS_SOURCE_INFO (object));

	priv = PK_SOURCE_INFO (object)->priv;

	G_OBJECT_CLASS (pk_source_info_parent_class)->finalize (object);
}

static void
pk_source_info_class_init (PkSourceInfoClass *klass)
{
	GObjectClass *object_class;

	object_class = G_OBJECT_CLASS (klass);
	object_class->finalize = pk_source_info_finalize;
	g_type_class_add_private (object_class, sizeof (PkSourceInfoPrivate));
}

static void
pk_source_info_init (PkSourceInfo *source_info)
{
	source_info->priv = G_TYPE_INSTANCE_GET_PRIVATE (source_info,
	                                                 PK_TYPE_SOURCE_INFO,
	                                                 PkSourceInfoPrivate);
}
