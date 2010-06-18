/* pka-plugin.c
 *
 * Copyright (C) 2010 Christian Hergert <chris@dronelabs.com>
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

#ifdef G_LOG_DOMAIN
#undef G_LOG_DOMAIN
#endif
#define G_LOG_DOMAIN "Plugin"

#include <gmodule.h>
#include <glib/gprintf.h>

#include "pka-config.h"
#include "pka-log.h"
#include "pka-plugin.h"
#include "pka-source.h"

extern void pka_source_set_plugin (PkaSource *source, PkaPlugin *plugin);

/**
 * SECTION:pka-plugin
 * @title: PkaPlugin
 * @short_description: Support for pluggable features in Perfkit.
 *
 * The #PkaPlugin class represents a module that can be loaded from disk
 * to provide additional features to Perfkit.  Most of Perfkit is
 * implemented this way such as DBus support and various data sources.
 *
 * A plugin can be one of three types (and a module can only contain
 * a single plugin type).
 *
 *  1) A source plugin @PKA_PLUGIN_SOURCE.
 *  2) An encoder plugin @PKA_PLUGIN_ENCODER.
 *  3) A listener plugin @PKA_PLUGIN_LISTENER.
 *
 * A source plugin provides a way to extract data from the host system.
 *
 * An encoder plugin provides a way to encode data before it goes on
 * the wire in a format suitable to the transport.  A ZLib encoder would
 * be a prime example.
 *
 * A listener plugin provides a communication layer for local or remote
 * systems to communicate with the agent.
 */

G_DEFINE_TYPE(PkaPlugin, pka_plugin, G_TYPE_OBJECT)

struct _PkaPluginPrivate
{
	GModule *module;
	PkaPluginInfo *info;
};

/**
 * pka_plugin_new:
 *
 * Creates a new instance of #PkaPlugin.
 *
 * Returns: the newly created instance of #PkaPlugin.
 * Side effects: None.
 */
PkaPlugin*
pka_plugin_new (void)
{
	ENTRY;
	RETURN(g_object_new(PKA_TYPE_PLUGIN, NULL));
}

/**
 * pka_plugin_get_description:
 * @plugin: A #PkaPlugin.
 *
 * Retrieves the description of the plugin.  The value should not be modified
 * or freed.
 *
 * Returns: The description string.
 * Side effects: None.
 */
const gchar*
pka_plugin_get_description (PkaPlugin *plugin) /* IN */
{
	g_return_val_if_fail(PKA_IS_PLUGIN(plugin), NULL);
	if (plugin->priv->info) {
		return plugin->priv->info->description;
	}
	return "";
}

/**
 * pka_plugin_get_copyright:
 * @plugin: A #PkaPlugin.
 *
 * Retrieves the copyright for @plugin.
 *
 * Returns: A string which should not be modified or freed.
 * Side effects: None.
 */
const gchar*
pka_plugin_get_copyright (PkaPlugin *plugin) /* IN */
{
	g_return_val_if_fail(PKA_IS_PLUGIN(plugin), NULL);
	if (plugin->priv->info) {
		return plugin->priv->info->copyright;
	}
	return "";
}

/**
 * pka_plugin_get_name:
 * @plugin: A #PkaPlugin.
 *
 * Retrieves the name of the plugin.  The value should not be modified
 * or freed.
 *
 * Returns: The name string.
 * Side effects: None.
 */
const gchar*
pka_plugin_get_name (PkaPlugin *plugin) /* IN */
{
	g_return_val_if_fail(PKA_IS_PLUGIN(plugin), NULL);
	if (plugin->priv->info) {
		return plugin->priv->info->name;
	}
	return "";
}

/**
 * pka_plugin_get_version:
 * @plugin: A #PkaPlugin.
 *
 * Retrieves the version of the plugin.  The value should not be modified
 * or freed.
 *
 * Returns: The version string.
 * Side effects: None.
 */
const gchar*
pka_plugin_get_version (PkaPlugin *plugin) /* IN */
{
	g_return_val_if_fail(PKA_IS_PLUGIN(plugin), NULL);
	if (plugin->priv->info) {
		return plugin->priv->info->version;
	}
	return "";
}

/**
 * pka_plugin_get_plugin_type:
 * @plugin: A #PkaPlugin.
 *
 * Retrieves the plugin type of the plugin.
 *
 * Returns: The plugin type.
 * Side effects: None.
 */
PkaPluginType
pka_plugin_get_plugin_type (PkaPlugin *plugin) /* IN */
{
	g_return_val_if_fail(PKA_IS_PLUGIN(plugin), PKA_PLUGIN_INVALID);
	if (plugin->priv->info) {
		return plugin->priv->info->plugin_type;
	}
	return PKA_PLUGIN_INVALID;
}

/**
 * pka_plugin_get_id:
 * @plugin: A #PkaPlugin.
 *
 * Retrieves the id of the plugin.
 *
 * Returns: A string which should not be modified or freed.
 * Side effects: None.
 */
const gchar*
pka_plugin_get_id (PkaPlugin *plugin) /* IN */
{
	g_return_val_if_fail(PKA_IS_PLUGIN(plugin), NULL);
	if (plugin->priv->info) {
		return plugin->priv->info->id;
	}
	return NULL;
}

/**
 * pka_plugin_load_from_file:
 * @plugin: A #PkaPlugin.
 * @filename: The plugin path.
 * @error: A location for a #GError, or %NULL.
 *
 * Loads a plugin from the shared library located at @filename.
 *
 * Returns: %TRUE if successful; otherwise %FALSE.
 * Side effects: The shared library is loaded into the process.
 */
gboolean
pka_plugin_load_from_file (PkaPlugin    *plugin,   /* IN */
                           const gchar  *filename, /* IN */
                           GError      **error)    /* OUT */
{
	PkaPluginInfo *plugin_info = NULL;
	PkaPluginPrivate *priv;
	GModuleFlags flags;
	GModule *module;
	gint i;

	g_return_val_if_fail(PKA_IS_PLUGIN(plugin), FALSE);
	g_return_val_if_fail(filename != NULL, FALSE);
	g_return_val_if_fail(plugin->priv->module == NULL, FALSE);

	ENTRY;
	priv = plugin->priv;
	flags = G_MODULE_BIND_LAZY | G_MODULE_BIND_LOCAL;
	if (!(module = g_module_open(filename, flags))) {
		GOTO(cleanup);
	}
	if (!g_module_symbol(module, "pka_plugin_info",
	                     (gpointer *)&plugin_info)) {
		GOTO(cleanup);
	}
	if (!plugin_info) {
		GOTO(cleanup);
	}
	for (i = 0; plugin_info->id[i]; i++) {
		if (!g_ascii_isalnum(plugin_info->id[i])) {
			WARNING(Plugin, "Invalid plugin id: %s", plugin_info->id);
			plugin_info = NULL;
			GOTO(cleanup);
		}
	}
	priv->info = plugin_info;
	priv->module = module;
  cleanup:
  	if (!plugin_info) {
  		if (module) {
			g_module_close(module);
		}
		g_set_error(error, PKA_PLUGIN_ERROR,
		            PKA_PLUGIN_ERROR_INVALID_MODULE,
		            "Not a plugin: %s", filename);
		RETURN(FALSE);
	}
	RETURN(TRUE);
}

/**
 * pka_plugin_create:
 * @plugin: A #PkaPlugin.
 * @error: A location for a #GError, or %NULL.
 *
 * Creates an instance of the plugin.
 *
 * Returns: A new instance of the plugin.
 * Side effects: None.
 */
GObject*
pka_plugin_create (PkaPlugin  *plugin, /* IN */
                   GError    **error)  /* IN */
{
	PkaPluginPrivate *priv;
	GObject *ret = NULL;

	g_return_val_if_fail(PKA_IS_PLUGIN(plugin), NULL);

	ENTRY;
	priv = plugin->priv;
	if (priv->info->factory) {
		ret = priv->info->factory(error);
		if (ret) {
			if (priv->info->plugin_type == PKA_PLUGIN_SOURCE) {
				pka_source_set_plugin(PKA_SOURCE(ret), plugin);
			}
		}
	}
	RETURN(ret);
}

/**
 * pka_plugin_is_disabled:
 * @plugin: A #PkaPlugin.
 *
 * Determines of a plugin is disabled by the configuration system.
 *
 * Returns: %TRUE if the plugin is disabled; otherwise %FALSE.
 * Side effects: None.
 */
gboolean
pka_plugin_is_disabled (PkaPlugin *plugin) /* IN */
{
	gchar group[32] = { 0 };
	gint i;

	g_return_val_if_fail(PKA_IS_PLUGIN(plugin), FALSE);

	ENTRY;
	switch (pka_plugin_get_plugin_type(plugin)) {
	case PKA_PLUGIN_SOURCE:
		g_snprintf(group, sizeof(group), "source.%s",
		           pka_plugin_get_id(plugin));
		break;
	case PKA_PLUGIN_ENCODER:
		g_snprintf(group, sizeof(group), "encoder.%s",
		           pka_plugin_get_id(plugin));
		break;
	case PKA_PLUGIN_LISTENER:
		g_snprintf(group, sizeof(group), "listener.%s",
		           pka_plugin_get_id(plugin));
		break;
	case PKA_PLUGIN_INVALID:
	default:
		RETURN(TRUE);
	}
	for (i = 0; group[i]; i++) {
		group[i] = g_ascii_tolower(group[i]);
	}
	RETURN(pka_config_get_boolean(group, "disabled", TRUE));
}

/**
 * pka_plugin_error_quark:
 *
 * Retrieves the error domain for #PkaPlugin.
 *
 * Returns: A #GQuark.
 * Side effects: None.
 */
GQuark
pka_plugin_error_quark (void)
{
	return g_quark_from_static_string("pka-plugin-error-quark");
}

/**
 * pka_plugin_finalize:
 * @object: A #PkaPlugin.
 *
 * Finalization of a #PkaPlugin.  Resources are released.
 *
 * Returns: None.
 * Side effects: The plugins module may be unloaded.
 */
static void
pka_plugin_finalize (GObject *object) /* IN */
{
	PkaPluginPrivate *priv = PKA_PLUGIN(object)->priv;

	if (priv->module) {
		g_module_close(priv->module);
	}

	G_OBJECT_CLASS(pka_plugin_parent_class)->finalize(object);
}

/**
 * pka_plugin_class_init:
 * @klass: A #PkaPluginClass.
 *
 * Initialization of the #PkaPluginClass.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
pka_plugin_class_init (PkaPluginClass *klass) /* IN */
{
	GObjectClass *object_class;

	object_class = G_OBJECT_CLASS(klass);
	object_class->finalize = pka_plugin_finalize;
	g_type_class_add_private(object_class, sizeof(PkaPluginPrivate));
}

/**
 * pka_plugin_init:
 * @plugin: A #PkaPlugin.
 *
 * Initialization of a newly instantiated #PkaPlugin.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
pka_plugin_init (PkaPlugin *plugin) /* IN */
{
	plugin->priv = G_TYPE_INSTANCE_GET_PRIVATE(plugin,
	                                           PKA_TYPE_PLUGIN,
	                                           PkaPluginPrivate);
}
