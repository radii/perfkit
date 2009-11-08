/* pkd-sources.c
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

#include "pkd-sources.h"
#include "pkd-sources-dbus.h"

/**
 * SECTION:pkd-sources
 * @title: PkdSources
 * @short_description: Runtime management of #PkdSource<!-- -->s
 *
 * #PkdSources provides runtime management of #PkdSource instances.  It exposes
 * access to the sources over the DBUS.
 */

G_DEFINE_TYPE (PkdSources, pkd_sources, G_TYPE_OBJECT)

struct _PkdSourcesPrivate
{
	gpointer dummy;
};

static void
pkd_sources_finalize (GObject *object)
{
	G_OBJECT_CLASS (pkd_sources_parent_class)->finalize (object);
}

static void
pkd_sources_class_init (PkdSourcesClass *klass)
{
	GObjectClass *object_class;

	object_class = G_OBJECT_CLASS (klass);
	object_class->finalize = pkd_sources_finalize;
	g_type_class_add_private (object_class, sizeof (PkdSourcesPrivate));

	dbus_g_object_type_install_info (PKD_TYPE_SOURCES, &dbus_glib_pkd_sources_object_info);
}

static void
pkd_sources_init (PkdSources *sources)
{
	sources->priv = G_TYPE_INSTANCE_GET_PRIVATE ((sources),
	                                             PKD_TYPE_SOURCES,
	                                             PkdSourcesPrivate);
}

/**
 * pkd_sources_new:
 *
 * Creates a new instance of #PkdSources.
 *
 * Return value: the newly created #PkdSources instance.
 */
PkdSources*
pkd_sources_new (void)
{
	return g_object_new (PKD_TYPE_SOURCES, NULL);
}

