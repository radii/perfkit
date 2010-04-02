/* pkd-encoder-info.c
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

#include <glib.h>
#include <glib-object.h>
#include <glib/gi18n.h>
#include <gmodule.h>

#include "pkd-encoder-info.h"

/**
 * SECTION:pkd-encoder-info
 * @title: PkdEncoderInfo
 * @short_description: Encoder plugins
 *
 * #PkdEncoderInfo encapsulates information about a #PkdEncoder plugin.
 * It also provides a simple interface to the plugins factory for creating
 * new instances of #PkdEncoder.
 */

G_DEFINE_TYPE(PkdEncoderInfo, pkd_encoder_info, G_TYPE_OBJECT)

struct _PkdEncoderInfoPrivate
{
	/* The module containing the plugin. */
	GModule *module;

	/* The factory function to create a PkdEncoder. */
	PkdEncoderFactory factory;

	/* The uid of the encoder type. */
	gchar *uid;

	/* The name of the endcoder type. */
	gchar *name;

	/* The description of the encoder type. */
	gchar *description;

	/* The version of the encoder type. */
	gchar *version;
};

enum
{
	PROP_0,
	PROP_UID,
	PROP_NAME,
	PROP_DESC,
	PROP_VERSION,
};

/**
 * pkd_encoder_info_new:
 *
 * Creates a new instance of #PkdEncoderInfo.
 *
 * Return value: the newly created #PkdEncoderInfo instance.
 */
PkdEncoderInfo*
pkd_encoder_info_new (void)
{
	return g_object_new (PKD_TYPE_ENCODER_INFO, NULL);
}

/**
 * pkd_encoder_info_load_from_file:
 * @encoder_info: A #PkdEncoderInfo
 * @filename: A module filename to load
 * @error: A location for a #GError or %NULL
 *
 * Opens the module filename provided by @filename.  The symbol
 * "pkd_encoder_register" is retrieved from the module and executed to
 * register the encoder factory.
 *
 * Returns: %TRUE if successful; otherwise %FALSE and @error is set.
 *
 * Side effects: The module is opened and executed.
 */
gboolean
pkd_encoder_info_load_from_file (PkdEncoderInfo  *encoder_info,
                                 const gchar     *filename,
                                 GError         **error)
{
	PkdStaticEncoderInfo *static_info = NULL;
	PkdEncoderInfoPrivate *priv;
	GModule *module;

	g_return_val_if_fail(PKD_IS_ENCODER_INFO(encoder_info), FALSE);
	g_return_val_if_fail(filename != NULL, FALSE);

	priv = encoder_info->priv;

	/*
	 * Ensure we have access to the filename.
	 */
	if (!g_file_test(filename, G_FILE_TEST_IS_REGULAR)) {
		g_set_error(error, PKD_ENCODER_INFO_ERROR, PKD_ENCODER_INFO_ERROR_FILENAME,
		            _("%s: not a Perfkit Daemon plugin."), filename);
		return FALSE;
	}

	/*
	 * Crack open the shared library.
	 */
	module = g_module_open(filename, G_MODULE_BIND_MASK);
	if (!module) {
		g_set_error(error, PKD_ENCODER_INFO_ERROR, PKD_ENCODER_INFO_ERROR_MODULE,
		            _("%s: not a Module."), filename);
		return FALSE;
	}

	/*
	 * Retrieve the "pkd_encoder_plugin" symbol which contains the information
	 * vtable.
	 */
	g_module_symbol(module, "pkd_encoder_plugin", (gpointer *)&static_info);
	if (!static_info) {
		g_set_error(error, PKD_ENCODER_INFO_ERROR, PKD_ENCODER_INFO_ERROR_SYMBOL,
					_("%s: symbol pkd_encoder_plugin not found."), filename);
		g_module_close(module);
		return FALSE;
	}

	/*
	 * Make sure there is a factory method defined.
	 */
	if (!static_info->factory) {
		g_warning("%s: Missing encoder factory function.", filename);
		g_module_close(module);
		return FALSE;
	}

	/*
	 * Retrieve the encoder descriptive text.
	 */
	priv->factory = static_info->factory;
	priv->uid = g_strdup(static_info->uid);
	priv->name = g_strdup(static_info->name);
	priv->version = g_strdup(static_info->version);
	priv->description = g_strdup(static_info->description);

	/*
	 * Save the module reference.
	 */
	priv->module = module;

	return TRUE;
}

/**
 * pkd_encoder_info_get_name:
 * @encoder_info: A #PkdEncoderInfo
 *
 * Retrieves the "name" property.
 *
 * Returns: a string or %NULL.
 *
 * Side effects: None.
 */
const gchar*
pkd_encoder_info_get_name (PkdEncoderInfo *encoder_info)
{
	g_return_val_if_fail(PKD_IS_ENCODER_INFO(encoder_info), NULL);
	return encoder_info->priv->name;
}

/**
 * pkd_encoder_info_get_uid:
 * @encoder_info: A #PkdEncoderInfo
 *
 * Retrieves the "uid" property.
 *
 * Returns: a string or %NULL.
 *
 * Side effects: None.
 */
const gchar*
pkd_encoder_info_get_uid (PkdEncoderInfo *encoder_info)
{
	g_return_val_if_fail(PKD_IS_ENCODER_INFO(encoder_info), NULL);
	return encoder_info->priv->uid;
}

/**
 * pkd_encoder_info_get_description:
 * @encoder_info: A #PkdEncoderInfo
 *
 * Retrieves the "description" property.
 *
 * Returns: A string which should not be modified or freed.
 *
 * Side effects: None.
 */
const gchar*
pkd_encoder_info_get_description (PkdEncoderInfo *encoder_info)
{
	g_return_val_if_fail(PKD_IS_ENCODER_INFO(encoder_info), NULL);
	return encoder_info->priv->description;
}

/**
 * pkd_encoder_info_get_version:
 * @encoder_info: A #PkdEncoderInfo
 *
 * Retrieves the "version" property.
 *
 * Returns: A string which should not be modified or freed.
 *
 * Side effects: None.
 */
const gchar*
pkd_encoder_info_get_version (PkdEncoderInfo *encoder_info)
{
	g_return_val_if_fail(PKD_IS_ENCODER_INFO(encoder_info), NULL);
	return encoder_info->priv->version;
}

/**
 * pkd_encoder_info_create:
 * @encoder_info: A #PkdEncoderInfo
 *
 * Executes the factory function to create a new instance of the #PkdEncoder
 * described by the #PkdEncoderInfo.
 *
 * Returns: A newly created #PkdEncoder or %NULL.
 *
 * Side effects: None.
 */
PkdEncoder*
pkd_encoder_info_create (PkdEncoderInfo *encoder_info)
{
	g_return_val_if_fail(PKD_IS_ENCODER_INFO(encoder_info), NULL);
	g_return_val_if_fail(encoder_info->priv->factory, NULL);

	g_debug("Creating %s encoder.", encoder_info->priv->name);
	return encoder_info->priv->factory();
}

/**
 * pkd_encoder_info_error_quark:
 *
 * Returns: the error domain quark for use with #GError.
 */
GQuark
pkd_encoder_info_error_quark(void)
{
	return g_quark_from_static_string("pkd-encoder-info-error-quark");
}

static void
pkd_encoder_info_get_property (PkdEncoderInfo *encoder_info,
                               guint           prop_id,
                               GValue         *value,
                               GParamSpec     *pspec)
{
	switch (prop_id) {
	case PROP_UID:
		g_value_set_string(value, pkd_encoder_info_get_uid(encoder_info));
		break;
	case PROP_NAME:
		g_value_set_string(value, pkd_encoder_info_get_name(encoder_info));
		break;
	case PROP_DESC:
		g_value_set_string(value, pkd_encoder_info_get_description(encoder_info));
		break;
	case PROP_VERSION:
		g_value_set_string(value, pkd_encoder_info_get_version(encoder_info));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(encoder_info, prop_id, pspec);
	}
}

static void
pkd_encoder_info_finalize (GObject *object)
{
	PkdEncoderInfoPrivate *priv;

	g_return_if_fail(PKD_IS_ENCODER_INFO(object));

	priv = PKD_ENCODER_INFO(object)->priv;

	g_free(priv->uid);
	g_free(priv->name);
	g_free(priv->description);
	g_free(priv->version);
	g_module_close(priv->module);

	G_OBJECT_CLASS(pkd_encoder_info_parent_class)->finalize(object);
}

static void
pkd_encoder_info_dispose (GObject *object)
{
	G_OBJECT_CLASS(pkd_encoder_info_parent_class)->dispose(object);
}

static void
pkd_encoder_info_class_init (PkdEncoderInfoClass *klass)
{
	GObjectClass *object_class;

	object_class = G_OBJECT_CLASS (klass);
	object_class->finalize = pkd_encoder_info_finalize;
	object_class->dispose = pkd_encoder_info_dispose;
	object_class->get_property = (gpointer)pkd_encoder_info_get_property;
	g_type_class_add_private(object_class, sizeof(PkdEncoderInfoPrivate));

	/**
	 * PkdEncoderInfo:uid:
	 *
	 * The "uid" property.
	 */
	g_object_class_install_property(object_class,
	                                PROP_UID,
	                                g_param_spec_string("uid",
	                                                    "uid",
	                                                    "Unique id",
	                                                    NULL,
	                                                    G_PARAM_READABLE));

	/**
	 * PkdEncoderInfo:name:
	 *
	 * The "name" property.
	 */
	g_object_class_install_property(object_class,
	                                PROP_NAME,
	                                g_param_spec_string("name",
	                                                    "name",
	                                                    "Name",
	                                                    NULL,
	                                                    G_PARAM_READABLE));

	/**
	 * PkdEncoderInfo:description:
	 *
	 * The "description" property.
	 */
	g_object_class_install_property(object_class,
	                                PROP_DESC,
	                                g_param_spec_string("description",
	                                                    "description",
	                                                    "Description",
	                                                    NULL,
	                                                    G_PARAM_READABLE));

	/**
	 * PkdEncoderInfo:version:
	 *
	 * The "version" property.
	 */
	g_object_class_install_property(object_class,
	                                PROP_VERSION,
	                                g_param_spec_string("version",
	                                                    "version",
	                                                    "Version",
	                                                    NULL,
	                                                    G_PARAM_READABLE));
}

static void
pkd_encoder_info_init (PkdEncoderInfo *encoder_info)
{
	encoder_info->priv = G_TYPE_INSTANCE_GET_PRIVATE(encoder_info,
	                                                 PKD_TYPE_ENCODER_INFO,
	                                                 PkdEncoderInfoPrivate);
}
