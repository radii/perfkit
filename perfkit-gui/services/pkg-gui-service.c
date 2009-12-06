/* pkg-gui_service.c
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

#include "pkg-gui-service.h"

/**
 * SECTION:pkg-gui_service
 * @title: PkgGuiService
 * @short_description: 
 *
 * TODO
 */

G_DEFINE_TYPE (PkgGuiService, pkg_gui_service, G_TYPE_OBJECT)

struct _PkgGuiServicePrivate
{
	GList *windows;
};

/**************************************************************************
 *                        GObject Class Methods                           *
 **************************************************************************/

static void
pkg_gui_service_finalize (GObject *object)
{
	PkgGuiServicePrivate *priv;

	priv = PKG_GUI_SERVICE (object)->priv;

	g_list_foreach (priv->windows, (GFunc)g_object_unref, NULL);
	g_list_free (priv->windows);

	G_OBJECT_CLASS (pkg_gui_service_parent_class)->finalize (object);
}

static void
pkg_gui_service_class_init (PkgGuiServiceClass *klass)
{
	GObjectClass *object_class;

	object_class = G_OBJECT_CLASS (klass);
	object_class->finalize = pkg_gui_service_finalize;
	g_type_class_add_private (object_class, sizeof (PkgGuiServicePrivate));
}

static void
pkg_gui_service_init (PkgGuiService *gui_service)
{
	gui_service->priv = G_TYPE_INSTANCE_GET_PRIVATE (gui_service,
	                                                 PKG_TYPE_GUI_SERVICE,
	                                                 PkgGuiServicePrivate);
}
