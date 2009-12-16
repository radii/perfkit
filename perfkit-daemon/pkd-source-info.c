/* pkd-source-info.c
 *
 * Copyright (C) 2009 Christian Hergert <chris@dronelabs.com>
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

#include <glib.h>
#include <glib-object.h>
#include <glib/gi18n.h>

#include "pkd-source-info.h"

G_DEFINE_TYPE (PkdSourceInfo, pkd_source_info, G_TYPE_OBJECT)

/**
 * SECTION:pkd-source_info
 * @title: PkdSourceInfo
 * @short_description: 
 *
 * 
 */

struct _PkdSourceInfoPrivate
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
	PROP_DESCRIPTION,
	PROP_VERSION,
};

/*
 *-----------------------------------------------------------------------------
 *
 * Public Methods
 *
 *-----------------------------------------------------------------------------
 */

/**
 * pkd_source_info_new:
 *
 * Creates a new instance of #PkdSourceInfo.
 *
 * Return value: the newly created #PkdSourceInfo instance.
 */
PkdSourceInfo*
pkd_source_info_new (void)
{
	return g_object_new (PKD_TYPE_SOURCE_INFO, NULL);
}

/**
 * pkd_source_info_get_uid:
 * @source_info: A #PkdSourceInfo
 *
 * Retrieves the "uid" property, a unique identifier for this data source
 * type.
 *
 * Return value: a string or %NULL.
 */
G_CONST_RETURN gchar*
pkd_source_info_get_uid (PkdSourceInfo *source_info)
{
	g_return_val_if_fail (PKD_IS_SOURCE_INFO (source_info), NULL);
	return source_info->priv->uid;
}

/**
 * pkd_source_info_get_name:
 * @source_info: A #PkdSourceInfo
 *
 * Retrieves the "name" property.
 *
 * Return value: a string or %NULL.
 */
G_CONST_RETURN gchar*
pkd_source_info_get_name (PkdSourceInfo *source_info)
{
	g_return_val_if_fail (PKD_IS_SOURCE_INFO (source_info), NULL);
	return source_info->priv->name;
}

/**
 * pkd_source_info_get_description:
 * @source_info: A #PkdSourceInfo
 *
 * Retrieves the "description" property.
 *
 * Return value: a string or %NULL.
 */
G_CONST_RETURN gchar*
pkd_source_info_get_description (PkdSourceInfo *source_info)
{
	g_return_val_if_fail (PKD_IS_SOURCE_INFO (source_info), NULL);
	return source_info->priv->description;
}

/**
 * pkd_source_info_get_version:
 * @source_info: #PkdSourceInfo
 *
 * Retrieves the "version" property.
 *
 * Return value: a string or %NULL.
 */
G_CONST_RETURN gchar*
pkd_source_info_get_version (PkdSourceInfo *source_info)
{
	g_return_val_if_fail (PKD_IS_SOURCE_INFO (source_info), NULL);
	return source_info->priv->version;
}

/*
 *-----------------------------------------------------------------------------
 *
 * Private Setters
 *
 *-----------------------------------------------------------------------------
 */

static void
pkd_source_info_set_uid (PkdSourceInfo *source_info,
                         const gchar   *uid)
{
	g_return_if_fail (PKD_IS_SOURCE_INFO (source_info));
	source_info->priv->uid = g_strdup (uid);
}

static void
pkd_source_info_set_name (PkdSourceInfo *source_info,
                          const gchar   *name)
{
	g_return_if_fail (PKD_IS_SOURCE_INFO (source_info));
	source_info->priv->name = g_strdup (name);
}

static void
pkd_source_info_set_description (PkdSourceInfo *source_info,
                                 const gchar   *description)
{
	g_return_if_fail (PKD_IS_SOURCE_INFO (source_info));
	source_info->priv->description = g_strdup (description);
}

static void
pkd_source_info_set_version (PkdSourceInfo *source_info,
                             const gchar   *version)
{
	g_return_if_fail (PKD_IS_SOURCE_INFO (source_info));
	source_info->priv->version = g_strdup (version);
}

/*
 *-----------------------------------------------------------------------------
 *
 * Class Methods
 *
 *-----------------------------------------------------------------------------
 */

static void
pkd_source_info_get_property (GObject    *object,
                              guint       property_id,
                              GValue     *value,
                              GParamSpec *pspec)
{
	const gchar *tmpstr;

	g_return_if_fail (PKD_IS_SOURCE_INFO (object));

	switch (property_id) {
	case PROP_UID:
		tmpstr = pkd_source_info_get_uid (PKD_SOURCE_INFO (object));
		g_value_set_string (value, tmpstr);
		break;
	case PROP_NAME:
		tmpstr = pkd_source_info_get_name (PKD_SOURCE_INFO (object));
		g_value_set_string (value, tmpstr);
		break;
	case PROP_DESCRIPTION:
		tmpstr = pkd_source_info_get_description (PKD_SOURCE_INFO (object));
		g_value_set_string (value, tmpstr);
		break;
	case PROP_VERSION:
		tmpstr = pkd_source_info_get_version (PKD_SOURCE_INFO (object));
		g_value_set_string (value, tmpstr);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
	}
}

static void
pkd_source_info_set_property (GObject      *object,
                              guint         property_id,
                              const GValue *value,
                              GParamSpec   *pspec)
{
	g_return_if_fail (PKD_IS_SOURCE_INFO (object));

	switch (property_id) {
	case PROP_UID:
		pkd_source_info_set_uid (PKD_SOURCE_INFO (object),
		                         g_value_get_string (value));
		break;
	case PROP_NAME:
		pkd_source_info_set_name (PKD_SOURCE_INFO (object),
		                          g_value_get_string (value));
		break;
	case PROP_DESCRIPTION:
		pkd_source_info_set_description (PKD_SOURCE_INFO (object),
		                                 g_value_get_string (value));
		break;
	case PROP_VERSION:
		pkd_source_info_set_version (PKD_SOURCE_INFO (object),
		                             g_value_get_string (value));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
	}
}

static void
pkd_source_info_finalize (GObject *object)
{
	PkdSourceInfoPrivate *priv;

	g_return_if_fail (PKD_IS_SOURCE_INFO (object));

	priv = PKD_SOURCE_INFO (object)->priv;

	G_OBJECT_CLASS (pkd_source_info_parent_class)->finalize (object);
}

static void
pkd_source_info_class_init (PkdSourceInfoClass *klass)
{
	GObjectClass *object_class;

	object_class = G_OBJECT_CLASS (klass);
	object_class->finalize = pkd_source_info_finalize;
	object_class->get_property = pkd_source_info_get_property;
	object_class->set_property = pkd_source_info_set_property;
	g_type_class_add_private (object_class, sizeof (PkdSourceInfoPrivate));

	/**
	 * PkdSourceInfo:uid:
	 *
	 * The "uid" property.
	 */
	g_object_class_install_property (object_class,
	                                 PROP_UID,
	                                 g_param_spec_string ("uid",
	                                                      "uid",
	                                                      "Unique Identifier",
	                                                      NULL,
	                                                      G_PARAM_READABLE | G_PARAM_CONSTRUCT_ONLY));

	/**
	 * PkdSourceInfo:name:
	 *
	 * The "name" property.
	 */
	g_object_class_install_property (object_class,
	                                 PROP_NAME,
	                                 g_param_spec_string ("name",
	                                                      "name",
	                                                      "Name",
	                                                      NULL,
	                                                      G_PARAM_READABLE | G_PARAM_CONSTRUCT_ONLY));

	/**
	 * PkdSourceInfo:description:
	 *
	 * The "description" property.
	 */
	g_object_class_install_property (object_class,
	                                 PROP_DESCRIPTION,
	                                 g_param_spec_string ("description",
	                                                      "description",
	                                                      "Description",
	                                                      NULL,
	                                                      G_PARAM_READABLE | G_PARAM_CONSTRUCT_ONLY));

	/**
	 * PkdSourceInfo:version:
	 *
	 * The "version" property.
	 */
	g_object_class_install_property (object_class,
	                                 PROP_VERSION,
	                                 g_param_spec_string ("version",
	                                                      "version",
	                                                      "Version",
	                                                      NULL,
	                                                      G_PARAM_READABLE | G_PARAM_CONSTRUCT_ONLY));
}

static void
pkd_source_info_init (PkdSourceInfo *source_info)
{
	source_info->priv = G_TYPE_INSTANCE_GET_PRIVATE (source_info,
	                                                 PKD_TYPE_SOURCE_INFO,
	                                                 PkdSourceInfoPrivate);
}
