/* pkg-window.c
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

#include <glib/gi18n.h>
#include <gtk/gtk.h>

#include "pkg-paths.h"
#include "pkg-window.h"
#include "pkg-runtime.h"

G_DEFINE_TYPE(PkgWindow, pkg_window, GTK_TYPE_WINDOW)

/**
 * SECTION:pkg-window
 * @title: PkgWindow
 * @short_description: 
 *
 * 
 */

struct _PkgWindowPrivate
{
	GtkBuilder *builder;
};

static GList *windows = NULL;

/**
 * pkg_window_new:
 *
 * Creates a new instance of #PkgWindow.
 *
 * Return value: the newly created #PkgWindow instance.
 */
GtkWidget*
pkg_window_new (void)
{
	return g_object_new(PKG_TYPE_WINDOW, NULL);
}

gint
pkg_window_count_windows (void)
{
	return g_list_length(windows);
}

static void
pkg_window_finalize (GObject *object)
{
	G_OBJECT_CLASS(pkg_window_parent_class)->finalize(object);
}

static void
pkg_window_class_init (PkgWindowClass *klass)
{
	GObjectClass *object_class;

	object_class = G_OBJECT_CLASS (klass);
	object_class->finalize = pkg_window_finalize;
	g_type_class_add_private(object_class, sizeof(PkgWindowPrivate));
}

static void
pkg_window_init (PkgWindow *window)
{
	windows = g_list_prepend(windows, g_object_ref(window));

	window->priv = G_TYPE_INSTANCE_GET_PRIVATE(window,
	                                           PKG_TYPE_WINDOW,
	                                           PkgWindowPrivate);

	gtk_window_set_title(GTK_WINDOW(window), _("Perfkit"));
	gtk_window_set_default_size(GTK_WINDOW(window), 800, 550);
}
