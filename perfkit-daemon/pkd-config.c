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

#include "pkd-config.h"

static GKeyFile *keyfile = NULL;

/**
 * pkd_config_init:
 * @filename: The configuration filename
 *
 * Initializes the runtime configuration using the filename specified.
 */
void
pkd_config_init (const gchar *filename)
{
	GError *error = NULL;

	g_return_if_fail (keyfile == NULL);

	keyfile = g_key_file_new ();
	g_assert (keyfile);

	/*
	 * Load the configuration from file.  If we cannot, we leave the keyfile
	 * around so we can use it store values.
	 */

	if (!g_file_test (filename, G_FILE_TEST_IS_REGULAR)) {
		g_warning ("Could not locate configuration file: %s", filename);
		return;
	}

	if (!g_key_file_load_from_file (keyfile, filename, 0, &error)) {
		g_warning ("Could not load configuration file: %s", error->message);
		g_error_free (error);
		return;
	}
}

/**
 * pkd_config_get_string:
 * @group: the name of the config group
 * @name: the name of the key
 *
 * Retrieves a value for the key from the active configuration.
 *
 * Return value:
 *       A string containing the result if successful; otherwise %NULL.
 *       The string should be freed with g_free().
 *
 * Side effects:
 *       None.
 */
gchar*
pkd_config_get_string (const gchar *group,
                       const gchar *name)
{
	g_return_val_if_fail (keyfile != NULL, NULL);
	g_return_val_if_fail (group != NULL, NULL);
	g_return_val_if_fail (name != NULL, NULL);

	return g_key_file_get_string (keyfile, group, name, NULL);
}

/**
 * pkd_config_get_boolean:
 * @group: the name of the config group
 * @name: the name of the key
 *
 * Retrieves a value for the key in the active configuration.
 *
 * Return value:
 *       A #gboolean containing the result if successful; otherwise %FALSE.
 *
 * Side effects:
 *       None.
 */
gboolean
pkd_config_get_boolean (const gchar *group,
                        const gchar *name)
{
	g_return_val_if_fail (keyfile != NULL, FALSE);
	g_return_val_if_fail (group != NULL, FALSE);
	g_return_val_if_fail (name != NULL, FALSE);

	return g_key_file_get_boolean (keyfile, group, name, NULL);
}

/**
 * pkd_config_get_boolean_default:
 * @group: the name of the config group
 * @name: the name of the key
 * @default_: the default value
 *
 * Retrieves a value for the key in the active configuration.  If the value
 * does not exist in the configuration, @default_ is returned.
 *
 * Return value:
 *       A #gboolean containing the result if successful; otherwise @default_.
 *
 * Side effects:
 *       None.
 */
gboolean
pkd_config_get_boolean_default (const gchar *group,
                                const gchar *name,
                                gboolean     default_)
{
	g_return_val_if_fail (keyfile != NULL, FALSE);
	g_return_val_if_fail (group != NULL, FALSE);
	g_return_val_if_fail (name != NULL, FALSE);

	if (!g_key_file_has_key (keyfile, group, name, NULL)) {
		return default_;
	}

	return g_key_file_get_boolean (keyfile, group, name, NULL);
}

/**
 * pkd_config_get_integer:
 * @group: the name of the config group
 * @name: the name of the key
 *
 * Retrieves the value for the key in the active configuration.
 *
 * Return value:
 *       A #gint containing the result if successful; otherwise 0.
 *
 * Side effects:
 *       None.
 */
gint
pkd_config_get_integer (const gchar *group,
                        const gchar *name)
{
	g_return_val_if_fail (keyfile != NULL, 0);
	g_return_val_if_fail (group != NULL, 0);
	g_return_val_if_fail (name != NULL, 0);

	return g_key_file_get_integer (keyfile, group, name, NULL);
}

/**
 * pkd_config_set_boolean:
 * @group: the name of the config group
 * @name: the name of the key
 * @value: the new value for the key
 *
 * Overrides a default value for a config key.
 *
 * Side effects:
 *       Modifies the existing value in the config.
 */
void
pkd_config_set_boolean (const gchar *group,
                        const gchar *name,
                        gboolean     value)
{
	g_return_if_fail (keyfile != NULL);
	g_return_if_fail (group != NULL);
	g_return_if_fail (name != NULL);

	g_key_file_set_boolean (keyfile, group, name, value);
}
