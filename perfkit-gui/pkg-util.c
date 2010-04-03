/* pkg-util.c
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

#include "pkg-paths.h"
#include "pkg-util.h"

GtkBuilder*
pkg_util_get_builder (const gchar *name)
{
	GtkBuilder *builder = NULL;
	GError *error = NULL;
	gchar *path;

	path = pkg_paths_build_data_path("ui", name, NULL);
	if (!g_file_test(path, G_FILE_TEST_IS_REGULAR)) {
		g_critical("Could not locate UI file: %s", path);
		goto cleanup;
	}

	builder = gtk_builder_new();
	if (!gtk_builder_add_from_file(builder, path, &error)) {
		g_critical("Error loading UI file: %s", error->message);
		g_error_free(error);
		goto cleanup;
	}

cleanup:
	g_free(path);
	return builder;
}

void
pkg_util_reparent (GtkBuilder  *builder,
                   const gchar *name,
                   GtkWidget   *parent)
{
	GObject *child;

	g_return_if_fail(GTK_IS_BUILDER(builder));
	g_return_if_fail(name != NULL);
	g_return_if_fail(GTK_IS_CONTAINER(parent));

	/*
	 * Retrieve the child.
	 */
	child = gtk_builder_get_object(builder, name);
	if (!GTK_IS_WIDGET(child)) {
		return;
	}

	/*
	 * Reparent the child.
	 */
	gtk_widget_reparent(GTK_WIDGET(child),  parent);

	/*
	 * TODO: Handle AccelGroups.
	 */
}
