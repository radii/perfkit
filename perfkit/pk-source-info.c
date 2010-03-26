/* pk-source-info.c
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
#include "pk-source-info.h"

G_DEFINE_TYPE(PkSourceInfo, pk_source_info, G_TYPE_OBJECT)

/**
 * SECTION:pk-source_info
 * @title: PkSourceInfo
 * @short_description: 
 *
 * 
 */

struct _PkSourceInfoPrivate
{
	gchar *uid;
	gchar *name;
	gchar *description;
	gchar *version;
};

enum
{
	PROP_0,
	PROP_UID,
	PROP_NAME,
	PROP_VERSION,
	PROP_DESCRIPTION,
};

static void
pk_source_info_get_property (GObject    *object,
                             guint       prop_id,
                             GValue     *value,
                             GParamSpec *pspec)
{
	switch (prop_id) {
	case PROP_UID:
		g_value_set_string (value, pk_source_info_get_uid (PK_SOURCE_INFO (object)));
		break;
	case PROP_NAME:
		g_value_set_string (value, pk_source_info_get_name (PK_SOURCE_INFO (object)));
		break;
	case PROP_VERSION:
		g_value_set_string (value, pk_source_info_get_version (PK_SOURCE_INFO (object)));
		break;
	case PROP_DESCRIPTION:
		g_value_set_string (value, pk_source_info_get_description (PK_SOURCE_INFO (object)));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
	}
}

static void
pk_source_info_set_property (GObject      *object,
                             guint         prop_id,
                             const GValue *value,
                             GParamSpec   *pspec)
{
	switch (prop_id) {
	case PROP_UID:
		if (g_value_get_string(value)) {
			g_free(PK_SOURCE_INFO(object)->priv->uid);
			PK_SOURCE_INFO(object)->priv->uid = g_value_dup_string(value);
		}
		break;
	case PROP_NAME:
		if (g_value_get_string(value)) {
			g_free(PK_SOURCE_INFO(object)->priv->name);
			PK_SOURCE_INFO(object)->priv->name = g_value_dup_string(value);
		}
		break;
	case PROP_VERSION:
		if (g_value_get_string(value)) {
			g_free(PK_SOURCE_INFO(object)->priv->version);
			PK_SOURCE_INFO(object)->priv->version = g_value_dup_string(value);
		}
		break;
	case PROP_DESCRIPTION:
		if (g_value_get_string(value)) {
			g_free(PK_SOURCE_INFO(object)->priv->description);
			PK_SOURCE_INFO(object)->priv->description = g_value_dup_string(value);
		}
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
	}
}

static void
pk_source_info_finalize (GObject *object)
{
	PkSourceInfoPrivate *priv = PK_SOURCE_INFO(object)->priv;

	g_free(priv->uid);
	g_free(priv->description);
	g_free(priv->name);
	g_free(priv->version);

	G_OBJECT_CLASS(pk_source_info_parent_class)->finalize(object);
}

static void
pk_source_info_dispose (GObject *object)
{
	G_OBJECT_CLASS(pk_source_info_parent_class)->dispose(object);
}

static void
pk_source_info_class_init (PkSourceInfoClass *klass)
{
	GObjectClass *object_class;

	object_class = G_OBJECT_CLASS (klass);
	object_class->finalize = pk_source_info_finalize;
	object_class->dispose = pk_source_info_dispose;
	object_class->get_property = pk_source_info_get_property;
	object_class->set_property = pk_source_info_set_property;
	g_type_class_add_private(object_class, sizeof(PkSourceInfoPrivate));

	/**
	 * PkSourceInfo:uid:
	 *
	 * The "uid" property.
	 */
	g_object_class_install_property(object_class,
	                                PROP_UID,
	                                g_param_spec_string("uid",
	                                                    "uid",
	                                                    "The uid property",
	                                                    NULL,
	                                                    G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));

	/**
	 * PkSourceInfo:name:
	 *
	 * The "name" property.
	 */
	g_object_class_install_property(object_class,
	                                PROP_NAME,
	                                g_param_spec_string("name",
	                                                    "name",
	                                                    "The name property",
	                                                    NULL,
	                                                    G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));

	/**
	 * PkSourceInfo:version:
	 *
	 * The "version" property.
	 */
	g_object_class_install_property(object_class,
	                                PROP_VERSION,
	                                g_param_spec_string("version",
	                                                    "version",
	                                                    "The version property",
	                                                    NULL,
	                                                    G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));

	/**
	 * PkSourceInfo:description:
	 *
	 * The "description" property.
	 */
	g_object_class_install_property(object_class,
	                                PROP_DESCRIPTION,
	                                g_param_spec_string("description",
	                                                    "description",
	                                                    "The description property",
	                                                    NULL,
	                                                    G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
}

static void
pk_source_info_init (PkSourceInfo *source_info)
{
	source_info->priv = G_TYPE_INSTANCE_GET_PRIVATE(source_info,
	                                                PK_TYPE_SOURCE_INFO,
	                                                PkSourceInfoPrivate);
	source_info->priv->description = g_strdup("");
	source_info->priv->version = g_strdup("");
	source_info->priv->uid = g_strdup("");
	source_info->priv->name = g_strdup("");
}

/**
 * pk_source_info_get_uid:
 * @source_info: A #PkSourceInfo
 *
 * Retrieves the "uid" property.
 *
 * Return value: a string containing the "uid" property or %NULL.
 *   The value should not be modified or freed.
 *
 * Side effects: None.
 */
 G_CONST_RETURN gchar*
pk_source_info_get_uid (PkSourceInfo *source_info)
{
	g_return_val_if_fail (PK_IS_SOURCE_INFO (source_info), NULL);
	return source_info->priv->uid;
}

/**
 * pk_source_info_get_name:
 * @source_info: A #PkSourceInfo
 *
 * Retrieves the "name" property.
 *
 * Return value: a string containing the "name" property or %NULL.
 *   The value should not be modified or freed.
 *
 * Side effects: None.
 */
 G_CONST_RETURN gchar*
pk_source_info_get_name (PkSourceInfo *source_info)
{
	g_return_val_if_fail (PK_IS_SOURCE_INFO (source_info), NULL);
	return source_info->priv->name;
}

/**
 * pk_source_info_get_version:
 * @source_info: A #PkSourceInfo
 *
 * Retrieves the "version" property.
 *
 * Return value: a string containing the "version" property or %NULL.
 *   The value should not be modified or freed.
 *
 * Side effects: None.
 */
 G_CONST_RETURN gchar*
pk_source_info_get_version (PkSourceInfo *source_info)
{
	g_return_val_if_fail (PK_IS_SOURCE_INFO (source_info), NULL);
	return source_info->priv->version;
}

/**
 * pk_source_info_get_description:
 * @source_info: A #PkSourceInfo
 *
 * Retrieves the "description" property.
 *
 * Return value: a string containing the "description" property or %NULL.
 *   The value should not be modified or freed.
 *
 * Side effects: None.
 */
G_CONST_RETURN gchar*
pk_source_info_get_description (PkSourceInfo *source_info)
{
	g_return_val_if_fail (PK_IS_SOURCE_INFO (source_info), NULL);
	return source_info->priv->description;
}
