/* pkd-config.c
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
#include <glib/gi18n.h>
#include <glib/gthread.h>

#include "pkd-config.h"

static GKeyFile *config = NULL;

/**
 * pkd_config_init:
 * @filename: A string containing the configuration path.
 *
 * Initializes the configuration subsystem.  The configuration file is loaded
 * into memory and will be used in the future to access specific configuration
 * variables.  This method should only be called once.
 *
 * See pkd_config_get_string() and others.
 *
 * Side effects: The backing file for the configuration is loaded into global
 * state.
 */
void
pkd_config_init (const gchar *filename)
{
	static gsize init = 0;
	GKeyFile *keyfile;
	GError *error;

	if (g_once_init_enter(&init)) {
		error = NULL;
		keyfile = g_key_file_new();

		/* Load the configuration file from the specified file.
		 * If we have an error, that is okay.  We will continue to use the
		 * GKeyFile as a backing store for our configuration queries.
		 */

		if (!g_key_file_load_from_file(keyfile, filename, 0, &error)) {
			g_warning(_("%s: Could not load configuration: %s"),
			          G_STRLOC, error->message);
			g_error_free(error);
		}

		config = keyfile;
		g_once_init_leave(&init, (gsize)keyfile);
	}
}

/**
 * pkd_config_get_string:
 * @group: A string containing the group of keys.
 * @key: A string containing the key within the group.
 * @default_: A string containing the default value if no value exists.
 *
 * Retrieves the value for the key matching @key in the group matching @group.
 * If the group or key do not exist, then a copy of @default_ is returned.
 *
 * Returns: A string containing either the value found or a copy of @default_.
 * The resulting string should be freed with g_free() when it is no longer
 * used.
 *
 * Side effects: None.
 */
gchar*
pkd_config_get_string (const gchar *group,
                      const gchar *key,
                      const gchar *default_)
{
	g_return_val_if_fail(group != NULL, NULL);
	g_return_val_if_fail(key != NULL, NULL);
	g_return_val_if_fail(config != NULL, NULL);

	if (!g_key_file_has_key(config, group, key, NULL)) {
		return g_strdup(default_);
	}

	return g_key_file_get_string(config, group, key, NULL);
}

/**
 * pkd_config_get_integer:
 * @group: A string containing the group of keys.
 * @key: A string containing the key within the group.
 * @default_: An integer containing the default value if no value exists.
 *
 * Retrieves the value for the key matching @key in the group matching @group.
 * If the group or key do not exist, then @default_ is returned.
 *
 * Returns: An integer containing the value found or @default_.
 *
 * Side effects: None.
 */
gint
pkd_config_get_integer (const gchar *group,
                       const gchar *key,
                       gint         default_)
{
	g_return_val_if_fail(group != NULL, 0);
	g_return_val_if_fail(key != NULL, 0);
	g_return_val_if_fail(config != NULL, 0);

	if (!g_key_file_has_key(config, group, key, NULL)) {
		return default_;
	}

	return g_key_file_get_integer(config, group, key, NULL);
}

/**
 * pkd_config_get_boolean:
 * @group: A string containing the group of keys.
 * @key: A string containing the key within the group.
 * @default_: An gboolean containing the default value if no value exists.
 *
 * Retrieves the value for the key matching @key in the group matching @group.
 * If the group or key do not exist, then @default_ is returned.
 *
 * Returns: A gboolean containing the value found or @default_.
 *
 * Side effects: None.
 */

gboolean
pkd_config_get_boolean (const gchar *group,
                       const gchar *key,
                       gboolean     default_)
{
	g_return_val_if_fail(group != NULL, FALSE);
	g_return_val_if_fail(key != NULL, FALSE);
	g_return_val_if_fail(config != NULL, FALSE);

	if (!g_key_file_has_key(config, group, key, NULL)) {
		return default_;
	}

	return g_key_file_get_boolean(config, group, key, NULL);
}
