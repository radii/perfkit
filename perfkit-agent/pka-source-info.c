/* pka-source-info.c
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

#include "pka-source-info.h"

/**
 * SECTION:pka-source-info
 * @title: PkaSourceInfo
 * @short_description: #PkaSource plugins
 *
 * #PkaSourceInfo represents a plugin that provides #PkaSource<!-- -->'s for
 * instrumenting a process.  When the plugin's module is opened, various data
 * is extracted and stored in a #PkaSourceInfo structure.
 *
 * pka_source_info_create() will call the factory method for the plugin to
 * create a new instance of a #PkaSource.
 */

G_DEFINE_TYPE (PkaSourceInfo, pka_source_info, G_TYPE_OBJECT)

struct _PkaSourceInfoPrivate
{
	GModule          *module;
	PkaSourceFactory   factory;
	gchar            *uid;
	gchar            *name;
	gchar            *description;
	gchar            *version;
	gchar           **conflicts;
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
 * pka_source_info_new:
 *
 * Creates a new instance of #PkaSourceInfo.
 *
 * Return value: the newly created #PkaSourceInfo instance.
 */
PkaSourceInfo*
pka_source_info_new (void)
{
	return g_object_new (PKA_TYPE_SOURCE_INFO, NULL);
}

/**
 * pka_source_info_load_from_file:
 * @source_info: A #PkaSourceInfo
 * @filename: A module filename to load
 * @error: A location for a #GError or %NULL
 *
 * Opens the module filename provided by @filename.  The symbol
 * "pka_source_register" is retrieved from the module and executed to
 * register the source factory.
 *
 * Returns: %TRUE if successful; otherwise %FALSE and @error is set.
 *
 * Side effects:
 *   The module is opened and information extracted from introspected symbols.
 */
gboolean
pka_source_info_load_from_file (PkaSourceInfo  *source_info,
                                const gchar   *filename,
                                GError       **error)
{
	PkaStaticSourceInfo *static_info = NULL;
	PkaSourceInfoPrivate *priv;
	GModule *module;
	gint i;

	g_return_val_if_fail(PKA_IS_SOURCE_INFO(source_info), FALSE);
	g_return_val_if_fail(filename != NULL, FALSE);

	priv = source_info->priv;

	/*
	 * Ensure we have access to the filename.
	 */
	if (!g_file_test(filename, G_FILE_TEST_IS_REGULAR)) {
		g_set_error(error, PKA_SOURCE_INFO_ERROR, PKA_SOURCE_INFO_ERROR_FILENAME,
		            _("%s: not a Perfkit Agent plugin."), filename);
		return FALSE;
	}

	/*
	 * Crack open the shared library.
	 */
	module = g_module_open(filename, G_MODULE_BIND_MASK);
	if (!module) {
		g_set_error(error, PKA_SOURCE_INFO_ERROR, PKA_SOURCE_INFO_ERROR_MODULE,
		            _("%s: not a Module."), filename);
		return FALSE;
	}

	/*
	 * Retrieve the "pka_source_plugin" symbol which contains the information
	 * vtable.
	 */
	g_module_symbol(module, "pka_source_plugin", (gpointer *)&static_info);
	if (!static_info) {
		g_set_error(error, PKA_SOURCE_INFO_ERROR, PKA_SOURCE_INFO_ERROR_SYMBOL,
					_("%s: symbol pka_source_plugin not found."), filename);
		g_module_close(module);
		return FALSE;
	}

	/*
	 * Make sure there is a factory method defined.
	 */
	if (!static_info->factory) {
		g_warning("%s: Missing source factory function.", filename);
		g_module_close(module);
		return FALSE;
	}

	/*
	 * Retrieve the source descriptive text.
	 */
	priv->factory = static_info->factory;
	priv->uid = g_strdup(static_info->uid);
	priv->name = g_strdup(static_info->name);
	priv->version = g_strdup(static_info->version);
	priv->description = g_strdup(static_info->description);
	if (static_info->conflicts) {
		priv->conflicts = g_strsplit(static_info->conflicts, ",", 0);
		for (i = 0; priv->conflicts[i]; i++) {
			g_strstrip(priv->conflicts[i]);
		}
	}

	/*
	 * Save the module reference.
	 */
	priv->module = module;

	return TRUE;
}

/**
 * pka_source_info_get_name:
 * @source_info: A #PkaSourceInfo
 *
 * Retrieves the "name" property.
 *
 * Returns: a string or %NULL.
 *
 * Side effects: None.
 */
const gchar*
pka_source_info_get_name (PkaSourceInfo *source_info)
{
	g_return_val_if_fail(PKA_IS_SOURCE_INFO(source_info), NULL);
	return source_info->priv->name;
}

/**
 * pka_source_info_get_uid:
 * @source_info: A #PkaSourceInfo
 *
 * Retrieves the "uid" property.
 *
 * Returns: a string or %NULL.
 *
 * Side effects: None.
 */
const gchar*
pka_source_info_get_uid (PkaSourceInfo *source_info)
{
	g_return_val_if_fail(PKA_IS_SOURCE_INFO(source_info), NULL);
	return source_info->priv->uid;
}

/**
 * pka_source_info_get_description:
 * @source_info: A #PkaSourceInfo
 *
 * Retrieves the "description" property.
 *
 * Returns: A string which should not be modified or freed.
 *
 * Side effects: None.
 */
const gchar*
pka_source_info_get_description (PkaSourceInfo *source_info)
{
	g_return_val_if_fail(PKA_IS_SOURCE_INFO(source_info), NULL);
	return source_info->priv->description;
}

/**
 * pka_source_info_get_version:
 * @source_info: A #PkaSourceInfo
 *
 * Retrieves the "version" property.
 *
 * Returns: A string which should not be modified or freed.
 *
 * Side effects: None.
 */
const gchar*
pka_source_info_get_version (PkaSourceInfo *source_info)
{
	g_return_val_if_fail(PKA_IS_SOURCE_INFO(source_info), NULL);
	return source_info->priv->version;
}

/**
 * pka_source_info_conflicts:
 * @source_info: A #PkaSourceInfo
 * @other: A #PkaSourceInfo
 *
 * Determines if @other conflicts with @source_info.  Conflicting #PkaSourceInfo
 * cannot be added to the same channel.  This allows UI implementations to fail
 * fast or grey out options when not available.
 *
 * Returns: %TRUE if @other conflicts with @source_info.
 */
gboolean
pka_source_info_conflicts (PkaSourceInfo *source_info,
                           PkaSourceInfo *other)
{
	PkaSourceInfoPrivate *priv;
	const gchar *other_uid;
	gint i;

	g_return_val_if_fail(PKA_IS_SOURCE_INFO(source_info), FALSE);
	g_return_val_if_fail(PKA_IS_SOURCE_INFO(other), FALSE);

	priv = source_info->priv;

	if (!priv->conflicts) {
		return FALSE;
	}

	other_uid = pka_source_info_get_uid(other);
	if (!other_uid) {
		g_warning("%s: Invalid source info uid (NULL).", G_STRLOC);
		return FALSE;
	}

	for (i = 0; i < g_strv_length(priv->conflicts); i++) {
		if (g_str_equal(priv->conflicts[i], other_uid))
			return TRUE;
	}

	return FALSE;
}

/**
 * pka_source_info_create:
 * @source_info: A #PkaSourceInfo
 *
 * Executes the factory function to create a new instance of the #PkaSource
 * described by the #PkaSourceInfo.
 *
 * Returns: A newly created #PkaSource or %NULL.
 *
 * Side effects: None.
 */
PkaSource*
pka_source_info_create (PkaSourceInfo *source_info)
{
	PkaSource *source;

	g_return_val_if_fail(PKA_IS_SOURCE_INFO(source_info), NULL);
	g_return_val_if_fail(source_info->priv->factory, NULL);

	g_debug("Creating %s source.", source_info->priv->name);
	source = source_info->priv->factory();
	g_object_set_qdata(G_OBJECT(source),
	                   g_quark_from_static_string("pka-source-info"),
	                   g_object_ref(source_info));
	return source;
}

GQuark
pka_source_info_error_quark (void)
{
	return g_quark_from_static_string("pka-source-info-error-quark");
}

static void
pka_source_info_get_property (PkaSourceInfo *source_info,
                              guint          prop_id,
                              GValue        *value,
                              GParamSpec    *pspec)
{
	switch (prop_id) {
	case PROP_UID:
		g_value_set_string(value, pka_source_info_get_uid(source_info));
		break;
	case PROP_NAME:
		g_value_set_string(value, pka_source_info_get_name(source_info));
		break;
	case PROP_DESC:
		g_value_set_string(value, pka_source_info_get_description(source_info));
		break;
	case PROP_VERSION:
		g_value_set_string(value, pka_source_info_get_version(source_info));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(source_info, prop_id, pspec);
	}
}

static void
pka_source_info_finalize (GObject *object)
{
	PkaSourceInfoPrivate *priv;

	g_return_if_fail(PKA_IS_SOURCE_INFO(object));

	priv = PKA_SOURCE_INFO(object)->priv;

	g_free(priv->uid);
	g_free(priv->name);
	g_free(priv->description);
	g_free(priv->version);
	g_module_close(priv->module);

	G_OBJECT_CLASS (pka_source_info_parent_class)->finalize(object);
}

static void
pka_source_info_dispose(GObject *object)
{
	G_OBJECT_CLASS(pka_source_info_parent_class)->dispose(object);
}

static void
pka_source_info_class_init(PkaSourceInfoClass *klass)
{
	GObjectClass *object_class;

	object_class = G_OBJECT_CLASS (klass);
	object_class->finalize = pka_source_info_finalize;
	object_class->dispose = pka_source_info_dispose;
	object_class->get_property = (gpointer)pka_source_info_get_property;
	g_type_class_add_private (object_class, sizeof(PkaSourceInfoPrivate));

	/**
	 * PkaSourceInfo:uid:
	 *
	 * The "uid" property.
	 */
	g_object_class_install_property (object_class,
	                                 PROP_UID,
	                                 g_param_spec_string("uid",
	                                                     "uid",
	                                                     "Unique id",
	                                                     NULL,
	                                                     G_PARAM_READABLE));

	/**
	 * PkaSourceInfo:name:
	 *
	 * The "name" property.
	 */
	g_object_class_install_property (object_class,
	                                 PROP_NAME,
	                                 g_param_spec_string("name",
	                                                     "name",
	                                                     "Name",
	                                                     NULL,
	                                                     G_PARAM_READABLE));

	/**
	 * PkaSourceInfo:description:
	 *
	 * The "description" property.
	 */
	g_object_class_install_property (object_class,
	                                 PROP_DESC,
	                                 g_param_spec_string("description",
	                                                     "description",
	                                                     "Description",
	                                                     NULL,
	                                                     G_PARAM_READABLE));

	/**
	 * PkaSourceInfo:version:
	 *
	 * The "version" property.
	 */
	g_object_class_install_property (object_class,
	                                 PROP_VERSION,
	                                 g_param_spec_string("version",
	                                                     "version",
	                                                     "Version",
	                                                     NULL,
	                                                     G_PARAM_READABLE));
}

static void
pka_source_info_init (PkaSourceInfo *source_info)
{
	source_info->priv = G_TYPE_INSTANCE_GET_PRIVATE (source_info,
	                                                 PKA_TYPE_SOURCE_INFO,
	                                                 PkaSourceInfoPrivate);
}
