/* pka-encoder-info.c
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

#include "pka-encoder-info.h"

/**
 * SECTION:pka-encoder-info
 * @title: PkaEncoderInfo
 * @short_description: Encoder plugins
 *
 * #PkaEncoderInfo encapsulates information about a #PkaEncoder plugin.
 * It also provides a simple interface to the plugins factory for creating
 * new instances of #PkaEncoder.
 */

G_DEFINE_TYPE(PkaEncoderInfo, pka_encoder_info, G_TYPE_OBJECT)

struct _PkaEncoderInfoPrivate
{
	/* The module containing the plugin. */
	GModule *module;

	/* The factory function to create a PkaEncoder. */
	PkaEncoderFactory factory;

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
 * pka_encoder_info_new:
 *
 * Creates a new instance of #PkaEncoderInfo.
 *
 * Return value: the newly created #PkaEncoderInfo instance.
 */
PkaEncoderInfo*
pka_encoder_info_new (void)
{
	return g_object_new (PKA_TYPE_ENCODER_INFO, NULL);
}

/**
 * pka_encoder_info_load_from_file:
 * @encoder_info: A #PkaEncoderInfo
 * @filename: A module filename to load
 * @error: A location for a #GError or %NULL
 *
 * Opens the module filename provided by @filename.  The symbol
 * "pka_encoder_register" is retrieved from the module and executed to
 * register the encoder factory.
 *
 * Returns: %TRUE if successful; otherwise %FALSE and @error is set.
 *
 * Side effects: The module is opened and executed.
 */
gboolean
pka_encoder_info_load_from_file (PkaEncoderInfo  *encoder_info,
                                 const gchar     *filename,
                                 GError         **error)
{
	PkaStaticEncoderInfo *static_info = NULL;
	PkaEncoderInfoPrivate *priv;
	GModule *module;

	g_return_val_if_fail(PKA_IS_ENCODER_INFO(encoder_info), FALSE);
	g_return_val_if_fail(filename != NULL, FALSE);

	priv = encoder_info->priv;

	/*
	 * Ensure we have access to the filename.
	 */
	if (!g_file_test(filename, G_FILE_TEST_IS_REGULAR)) {
		g_set_error(error, PKA_ENCODER_INFO_ERROR, PKA_ENCODER_INFO_ERROR_FILENAME,
		            _("%s: not a Perfkit Agent plugin."), filename);
		return FALSE;
	}

	/*
	 * Crack open the shared library.
	 */
	module = g_module_open(filename, G_MODULE_BIND_MASK);
	if (!module) {
		g_set_error(error, PKA_ENCODER_INFO_ERROR, PKA_ENCODER_INFO_ERROR_MODULE,
		            _("%s: not a Module."), filename);
		return FALSE;
	}

	/*
	 * Retrieve the "pka_encoder_plugin" symbol which contains the information
	 * vtable.
	 */
	g_module_symbol(module, "pka_encoder_plugin", (gpointer *)&static_info);
	if (!static_info) {
		g_set_error(error, PKA_ENCODER_INFO_ERROR, PKA_ENCODER_INFO_ERROR_SYMBOL,
					_("%s: symbol pka_encoder_plugin not found."), filename);
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
 * pka_encoder_info_get_name:
 * @encoder_info: A #PkaEncoderInfo
 *
 * Retrieves the "name" property.
 *
 * Returns: a string or %NULL.
 *
 * Side effects: None.
 */
const gchar*
pka_encoder_info_get_name (PkaEncoderInfo *encoder_info)
{
	g_return_val_if_fail(PKA_IS_ENCODER_INFO(encoder_info), NULL);
	return encoder_info->priv->name;
}

/**
 * pka_encoder_info_get_uid:
 * @encoder_info: A #PkaEncoderInfo
 *
 * Retrieves the "uid" property.
 *
 * Returns: a string or %NULL.
 *
 * Side effects: None.
 */
const gchar*
pka_encoder_info_get_uid (PkaEncoderInfo *encoder_info)
{
	g_return_val_if_fail(PKA_IS_ENCODER_INFO(encoder_info), NULL);
	return encoder_info->priv->uid;
}

/**
 * pka_encoder_info_get_description:
 * @encoder_info: A #PkaEncoderInfo
 *
 * Retrieves the "description" property.
 *
 * Returns: A string which should not be modified or freed.
 *
 * Side effects: None.
 */
const gchar*
pka_encoder_info_get_description (PkaEncoderInfo *encoder_info)
{
	g_return_val_if_fail(PKA_IS_ENCODER_INFO(encoder_info), NULL);
	return encoder_info->priv->description;
}

/**
 * pka_encoder_info_get_version:
 * @encoder_info: A #PkaEncoderInfo
 *
 * Retrieves the "version" property.
 *
 * Returns: A string which should not be modified or freed.
 *
 * Side effects: None.
 */
const gchar*
pka_encoder_info_get_version (PkaEncoderInfo *encoder_info)
{
	g_return_val_if_fail(PKA_IS_ENCODER_INFO(encoder_info), NULL);
	return encoder_info->priv->version;
}

/**
 * pka_encoder_info_create:
 * @encoder_info: A #PkaEncoderInfo
 *
 * Executes the factory function to create a new instance of the #PkaEncoder
 * described by the #PkaEncoderInfo.
 *
 * Returns: A newly created #PkaEncoder or %NULL.
 *
 * Side effects: None.
 */
PkaEncoder*
pka_encoder_info_create (PkaEncoderInfo *encoder_info)
{
	g_return_val_if_fail(PKA_IS_ENCODER_INFO(encoder_info), NULL);
	g_return_val_if_fail(encoder_info->priv->factory, NULL);

	g_debug("Creating %s encoder.", encoder_info->priv->name);
	return encoder_info->priv->factory();
}

/**
 * pka_encoder_info_error_quark:
 *
 * Returns: the error domain quark for use with #GError.
 */
GQuark
pka_encoder_info_error_quark(void)
{
	return g_quark_from_static_string("pka-encoder-info-error-quark");
}

static void
pka_encoder_info_get_property (PkaEncoderInfo *encoder_info,
                               guint           prop_id,
                               GValue         *value,
                               GParamSpec     *pspec)
{
	switch (prop_id) {
	case PROP_UID:
		g_value_set_string(value, pka_encoder_info_get_uid(encoder_info));
		break;
	case PROP_NAME:
		g_value_set_string(value, pka_encoder_info_get_name(encoder_info));
		break;
	case PROP_DESC:
		g_value_set_string(value, pka_encoder_info_get_description(encoder_info));
		break;
	case PROP_VERSION:
		g_value_set_string(value, pka_encoder_info_get_version(encoder_info));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(encoder_info, prop_id, pspec);
	}
}

static void
pka_encoder_info_finalize (GObject *object)
{
	PkaEncoderInfoPrivate *priv;

	g_return_if_fail(PKA_IS_ENCODER_INFO(object));

	priv = PKA_ENCODER_INFO(object)->priv;

	g_free(priv->uid);
	g_free(priv->name);
	g_free(priv->description);
	g_free(priv->version);
	g_module_close(priv->module);

	G_OBJECT_CLASS(pka_encoder_info_parent_class)->finalize(object);
}

static void
pka_encoder_info_dispose (GObject *object)
{
	G_OBJECT_CLASS(pka_encoder_info_parent_class)->dispose(object);
}

static void
pka_encoder_info_class_init (PkaEncoderInfoClass *klass)
{
	GObjectClass *object_class;

	object_class = G_OBJECT_CLASS (klass);
	object_class->finalize = pka_encoder_info_finalize;
	object_class->dispose = pka_encoder_info_dispose;
	object_class->get_property = (gpointer)pka_encoder_info_get_property;
	g_type_class_add_private(object_class, sizeof(PkaEncoderInfoPrivate));

	/**
	 * PkaEncoderInfo:uid:
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
	 * PkaEncoderInfo:name:
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
	 * PkaEncoderInfo:description:
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
	 * PkaEncoderInfo:version:
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
pka_encoder_info_init (PkaEncoderInfo *encoder_info)
{
	encoder_info->priv = G_TYPE_INSTANCE_GET_PRIVATE(encoder_info,
	                                                 PKA_TYPE_ENCODER_INFO,
	                                                 PkaEncoderInfoPrivate);
}
