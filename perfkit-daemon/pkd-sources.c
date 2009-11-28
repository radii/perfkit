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
	GHashTable    *factories;
};

typedef struct
{
	const gchar          *factory;
	PkdSourceFactoryFunc  factory_func;
	gpointer              user_data;
} Factory;

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
	sources->priv->factories = g_hash_table_new (g_str_hash, g_str_equal);
}

GQuark
pkd_sources_error_quark (void)
{
	return g_quark_from_static_string ("pkd-sources-error-quark");
}

/**
 * pkd_sources_add:
 * @sources: A #PkdSources
 * @type: 
 * @error: a location for a #GError or %NULL
 *
 * Create a new #PkdSource which can be attached to a #PkdChannel.
 *
 * Return value: The newly created #PkdSource or %NULL
 */
PkdSource*
pkd_sources_add (PkdSources  *sources,
                 const gchar *factory,
                 GError     **error)
{
	PkdSourcesPrivate *priv;
	PkdSource         *source = NULL;
	Factory           *f;
	gboolean           result = FALSE;

	g_return_val_if_fail (PKD_IS_SOURCES (sources), FALSE);

	priv = sources->priv;

	g_static_rw_lock_writer_lock (&priv->rw_lock);

	if (!(f = g_hash_table_lookup (priv->factories, factory))) {
		g_set_error (error, PKD_SOURCES_ERROR, PKD_SOURCES_ERROR_INVALID_TYPE,
		             "\"%s\" is not a valid data source",
		             factory);
		goto unlock;
	}

	if (!(source = f->factory_func (factory, f->user_data))) {
		g_set_error (error, PKD_SOURCES_ERROR, PKD_SOURCES_ERROR_INVALID_TYPE,
		             "Could not create data source \"%s\"",
		             factory);
		goto unlock;
	}

	priv->sources = g_list_prepend (priv->sources, source);
	result = TRUE;

unlock:
	g_static_rw_lock_writer_unlock (&priv->rw_lock);

	return source;
}

void
pkd_sources_register (PkdSources           *sources,
                      const gchar          *factory,
                      PkdSourceFactoryFunc  factory_func,
                      gpointer              user_data)
{
	PkdSourcesPrivate *priv;
	Factory           *f;
	gchar             *key;

	g_return_if_fail (PKD_IS_SOURCES (sources));

	priv = sources->priv;

	if (g_hash_table_lookup (priv->factories, factory)) {
		g_warning ("Data source type \"%s\" already registered.\n",
		           factory);
		return;
	}

	key = g_strdup (factory);
	f= g_slice_new0 (Factory);
	f->factory = key;
	f->factory_func = factory_func;
	f->user_data = user_data;

	g_hash_table_insert (priv->factories, key, f);
}

static gboolean
pkd_sources_add_dbus (PkdSources   *sources,
                      const gchar  *type,
                      gchar       **path,
                      GError      **error)
{
	PkdSource *source;

	g_return_val_if_fail (PKD_IS_SOURCES (sources), FALSE);
	g_return_val_if_fail (type != NULL, FALSE);
	g_return_val_if_fail (path != NULL, FALSE);

	if (!(source = pkd_sources_add (sources, type, error)))
		return FALSE;

	*path = g_strdup_printf ("/com/dronelabs/Perfkit/Sources/%d",
	                         pkd_source_get_id (source));

	return TRUE;
}

static gboolean
pkd_sources_get_types_dbus (PkdSources   *sources,
                            gchar      ***names,
                            GError      **error)
{
	PkdSourcesPrivate *priv;
	GHashTableIter     iter;
	gchar             *key;
	gint               count,
					   i = 0;

	g_return_val_if_fail (names != NULL, FALSE);

	priv = sources->priv;

	count = g_hash_table_size (priv->factories) + 1;
	*names = g_malloc0 (count * sizeof (gchar*));

	g_hash_table_iter_init (&iter, priv->factories);
	while (g_hash_table_iter_next (&iter, (gpointer*)&key, NULL))
		(*names) [i++] = g_strdup (key);

	return TRUE;
}
