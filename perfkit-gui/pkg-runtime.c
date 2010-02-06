/* pkg-runtime.c
 * 
 * Copyright (C) 2010 Christian Hergert
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

#include <stdlib.h>
#include <gtk/gtk.h>

#include "pkg-panels.h"
#include "pkg-prefs.h"
#include "pkg-runtime.h"
#include "pkg-window.h"

/**
 * SECTION:pkg-runtime
 * @title: Pkg
 * @short_description: runtime helpers for perfkit-gui.
 *
 * 
 */

static GList    *windows = NULL;
static gboolean  started = FALSE;

/**
 * pkg_runtime_initialize:
 *
 * Initializes the runtime system.
 */
gboolean
pkg_runtime_initialize (gint     *argc,
                             gchar  ***argv,
                             GError  **error)
{
	if (!pkg_prefs_init(argc, argv, error)) {
		return FALSE;
	}

	pkg_panels_init();

	started = TRUE;
}

/**
 * pkg_runtime_run:
 *
 * Starts the runtime system.  This method will block using a main loop for the
 * duration of applications life-time.
 */
void
pkg_runtime_run (void)
{
	gtk_main();
}

/**
 * pkg_runtime_quit:
 *
 * Stops the runtime system and gracefully shuts down the application.
 */
void
pkg_runtime_quit (void)
{
	if (!started) {
		exit(EXIT_FAILURE);
	}

	gtk_main_quit();
}

/**
 * pkg_runtime_shutdown:
 *
 * Gracefully cleans up after the runtime.  This should be called in the
 * main thread after the runtim has quit.
 */
void
pkg_runtime_shutdown (void)
{
}
