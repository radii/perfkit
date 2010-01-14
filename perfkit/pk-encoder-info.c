/* pk-encoder-info.c
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
#include "pk-encoder-info.h"

G_DEFINE_TYPE(PkEncoderInfo, pk_encoder_info, G_TYPE_OBJECT)

/**
 * SECTION:pk-encoder_info
 * @title: PkEncoderInfo
 * @short_description: 
 *
 * 
 */

struct _PkEncoderInfoPrivate
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
pk_encoder_info_get_property (GObject    *object,
                             guint       prop_id,
                             GValue     *value,
                             GParamSpec *pspec)
{
	switch (prop_id) {
	case PROP_UID:
		g_value_set_string (value, pk_encoder_info_get_uid (PK_ENCODER_INFO (object)));
		break;
	case PROP_NAME:
		g_value_set_string (value, pk_encoder_info_get_name (PK_ENCODER_INFO (object)));
		break;
	case PROP_VERSION:
		g_value_set_string (value, pk_encoder_info_get_version (PK_ENCODER_INFO (object)));
		break;
	case PROP_DESCRIPTION:
		g_value_set_string (value, pk_encoder_info_get_description (PK_ENCODER_INFO (object)));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
	}
}

static void
pk_encoder_info_set_property (GObject      *object,
                             guint         prop_id,
                             const GValue *value,
                             GParamSpec   *pspec)
{
	switch (prop_id) {
	case PROP_UID:
		PK_ENCODER_INFO(object)->priv->uid = g_value_dup_string(value);
		break;
	case PROP_NAME:
		PK_ENCODER_INFO(object)->priv->name = g_value_dup_string(value);
		break;
	case PROP_VERSION:
		PK_ENCODER_INFO(object)->priv->version = g_value_dup_string(value);
		break;
	case PROP_DESCRIPTION:
		PK_ENCODER_INFO(object)->priv->description = g_value_dup_string(value);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
	}
}

static void
pk_encoder_info_finalize (GObject *object)
{
	PkEncoderInfoPrivate *priv = PK_ENCODER_INFO(object)->priv;

	g_free(priv->uid);
	g_free(priv->description);
	g_free(priv->name);
	g_free(priv->version);

	G_OBJECT_CLASS(pk_encoder_info_parent_class)->finalize(object);
}

static void
pk_encoder_info_dispose (GObject *object)
{
	G_OBJECT_CLASS(pk_encoder_info_parent_class)->dispose(object);
}

static void
pk_encoder_info_class_init (PkEncoderInfoClass *klass)
{
	GObjectClass *object_class;

	object_class = G_OBJECT_CLASS (klass);
	object_class->finalize = pk_encoder_info_finalize;
	object_class->dispose = pk_encoder_info_dispose;
	object_class->get_property = pk_encoder_info_get_property;
	object_class->set_property = pk_encoder_info_set_property;
	g_type_class_add_private(object_class, sizeof(PkEncoderInfoPrivate));

	/**
	 * PkEncoderInfo:uid:
	 *
	 * The "uid" property.
	 */
	g_object_class_install_property(object_class,
	                                PROP_UID,
	                                g_param_spec_string("uid",
	                                                    "uid",
	                                                    "The uid property",
	                                                    NULL,
	                                                    G_PARAM_READABLE | G_PARAM_CONSTRUCT_ONLY));

	/**
	 * PkEncoderInfo:name:
	 *
	 * The "name" property.
	 */
	g_object_class_install_property(object_class,
	                                PROP_NAME,
	                                g_param_spec_string("name",
	                                                    "name",
	                                                    "The name property",
	                                                    NULL,
	                                                    G_PARAM_READABLE | G_PARAM_CONSTRUCT_ONLY));

	/**
	 * PkEncoderInfo:version:
	 *
	 * The "version" property.
	 */
	g_object_class_install_property(object_class,
	                                PROP_VERSION,
	                                g_param_spec_string("version",
	                                                    "version",
	                                                    "The version property",
	                                                    NULL,
	                                                    G_PARAM_READABLE | G_PARAM_CONSTRUCT_ONLY));

	/**
	 * PkEncoderInfo:description:
	 *
	 * The "description" property.
	 */
	g_object_class_install_property(object_class,
	                                PROP_DESCRIPTION,
	                                g_param_spec_string("description",
	                                                    "description",
	                                                    "The description property",
	                                                    NULL,
	                                                    G_PARAM_READABLE | G_PARAM_CONSTRUCT_ONLY));
}

static void
pk_encoder_info_init (PkEncoderInfo *encoder_info)
{
	encoder_info->priv = G_TYPE_INSTANCE_GET_PRIVATE(encoder_info,
	                                                PK_TYPE_ENCODER_INFO,
	                                                PkEncoderInfoPrivate);
}

/**
 * pk_encoder_info_get_uid:
 * @encoder_info: A #PkEncoderInfo
 *
 * Retrieves the "uid" property.
 *
 * Return value: a string containing the "uid" property or %NULL.
 *   The value should not be modified or freed.
 *
 * Side effects: None.
 */
 G_CONST_RETURN gchar*
pk_encoder_info_get_uid (PkEncoderInfo *encoder_info)
{
	g_return_val_if_fail (PK_IS_ENCODER_INFO (encoder_info), NULL);
	return encoder_info->priv->uid;
}

/**
 * pk_encoder_info_get_name:
 * @encoder_info: A #PkEncoderInfo
 *
 * Retrieves the "name" property.
 *
 * Return value: a string containing the "name" property or %NULL.
 *   The value should not be modified or freed.
 *
 * Side effects: None.
 */
 G_CONST_RETURN gchar*
pk_encoder_info_get_name (PkEncoderInfo *encoder_info)
{
	g_return_val_if_fail (PK_IS_ENCODER_INFO (encoder_info), NULL);
	return encoder_info->priv->name;
}

/**
 * pk_encoder_info_get_version:
 * @encoder_info: A #PkEncoderInfo
 *
 * Retrieves the "version" property.
 *
 * Return value: a string containing the "version" property or %NULL.
 *   The value should not be modified or freed.
 *
 * Side effects: None.
 */
 G_CONST_RETURN gchar*
pk_encoder_info_get_version (PkEncoderInfo *encoder_info)
{
	g_return_val_if_fail (PK_IS_ENCODER_INFO (encoder_info), NULL);
	return encoder_info->priv->version;
}

/**
 * pk_encoder_info_get_description:
 * @encoder_info: A #PkEncoderInfo
 *
 * Retrieves the "description" property.
 *
 * Return value: a string containing the "description" property or %NULL.
 *   The value should not be modified or freed.
 *
 * Side effects: None.
 */
G_CONST_RETURN gchar*
pk_encoder_info_get_description (PkEncoderInfo *encoder_info)
{
	g_return_val_if_fail (PK_IS_ENCODER_INFO (encoder_info), NULL);
	return encoder_info->priv->description;
}
