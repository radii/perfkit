/* ppg-config.c
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

#include <string.h>

#include "ppg-config.h"

typedef struct
{
	char     *filename;
	GKeyFile *keyfile;
} PpgConfig;

static PpgConfig config = { 0 };

/**
 * ppg_config_init:
 *
 * Initializes the configuration subsystem.
 *
 * Returns: None.
 * Side effects: Loads configuration file.
 */
void
ppg_config_init (void)
{
	config.filename = g_strdup("");
	config.keyfile = g_key_file_new();
}

/**
 * ppg_config_shutdown:
 *
 * Shuts down the configuration subsystem.
 *
 * Returns: None.
 * Side effects: Unloads configuration file.
 */
void
ppg_config_shutdown (void)
{
	g_free(config.filename);
	g_key_file_free(config.keyfile);
	memset(&config, 0, sizeof(config));
}
