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
#include "pkd-sources-glue.h"
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
	GStaticRWLock  rw_lock;
	GList         *sources;
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
	sources->priv = G_TYPE_INSTANCE_GET_PRIVATE (sources,
	                                             PKD_TYPE_SOURCES,
	                                             PkdSourcesPrivate);
}

GQuark
pkd_sources_error_quark (void)
{
	return g_quark_from_static_string ("pkd-sources-error-quark");
}

PkdSource*
pkd_sources_add (PkdSources  *sources,
                 GType        type,
                 GError     **error)
{
	PkdSourcesPrivate *priv;
	PkdSource         *source;

	g_return_val_if_fail (PKD_IS_SOURCES (sources), FALSE);

	priv = sources->priv;

	if (!g_type_is_a (type, PKD_TYPE_SOURCE)) {
		g_set_error (error, PKD_SOURCES_ERROR, PKD_SOURCES_ERROR_INVALID_TYPE,
		             "\"%s\" is not a valid PkdSource",
		             g_type_name (type));
		return FALSE;
	}

	source = g_object_new (type, NULL);

	g_static_rw_lock_writer_lock (&priv->rw_lock);
	priv->sources = g_list_prepend (priv->sources, source);
	g_static_rw_lock_writer_unlock (&priv->rw_lock);

	return source;
}

static gboolean
pkd_sources_add_dbus (PkdSources   *sources,
                      const gchar  *type,
                      gchar       **path,
                      GError      **error)
{
	PkdSource *source;
	GType      g_type;

	g_return_val_if_fail (PKD_IS_SOURCES (sources), FALSE);
	g_return_val_if_fail (type != NULL, FALSE);
	g_return_val_if_fail (path != NULL, FALSE);

	g_type = g_type_from_name (type);
	if (!(source = pkd_sources_add (sources, g_type, error)))
		return FALSE;

	*path = g_strdup_printf ("/com/dronelabs/Perfkit/Sources/%d",
	                         pkd_source_get_id (source));

	return TRUE;
}
