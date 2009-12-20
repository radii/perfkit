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

#include <glib.h>
#include <glib/gi18n.h>

#include "pkd-service.h"
#include "pkd-source-info.h"
#include "pkd-sources.h"

/**
 * SECTION:pkd-sources
 * @title: PkdSources
 * @short_description: Runtime management of #PkdSource<!-- -->s
 *
 * #PkdSources provides runtime management of #PkdSource instances.  It exposes
 * access to the sources over the DBUS.
 */

static void pkd_sources_init_service (PkdServiceIface *iface);

G_DEFINE_TYPE_EXTENDED (PkdSources, pkd_sources, G_TYPE_OBJECT, 0,
                        G_IMPLEMENT_INTERFACE (PKD_TYPE_SERVICE,
                                               pkd_sources_init_service));

struct _PkdSourcesPrivate
{
	GStaticRWLock  rw_lock;
	GList         *sources;
	GHashTable    *factories;
};

enum
{
	SOURCE_ADDED,
	SOURCE_INFO_ADDED,
	LAST_SIGNAL
};

static guint signals [LAST_SIGNAL];

GQuark
pkd_sources_error_quark (void)
{
	return g_quark_from_static_string ("pkd-sources-error-quark");
}

/**
 * pkd_sources_add:
 * @sources: A #PkdSources
 * @factory: the factory name
 * @error: a location for a #GError or %NULL
 *
 * Create a new #PkdSource which can be attached to a #PkdChannel.
 *
 * Return value: The newly created #PkdSource or %NULL
 *
 * Side effects: The new #PkdSource instance is added to @sources.
 */
PkdSource*
pkd_sources_add (PkdSources   *sources,
                 const gchar  *factory,
                 GError      **error)
{
	PkdSourcesPrivate *priv;
	PkdSourceInfo     *info;
	PkdSource         *source = NULL;
	gboolean           result = FALSE;

	g_return_val_if_fail (PKD_IS_SOURCES (sources), FALSE);

	priv = sources->priv;

	g_static_rw_lock_writer_lock (&priv->rw_lock);

	if (!(info = g_hash_table_lookup (priv->factories, factory))) {
		g_set_error (error, PKD_SOURCES_ERROR, PKD_SOURCES_ERROR_INVALID_TYPE,
		             "\"%s\" is not a valid data source type",
		             factory);
		goto unlock;
	}

	if (!(source = pkd_source_info_create (info))) {
		g_set_error (error, PKD_SOURCES_ERROR, PKD_SOURCES_ERROR_INVALID_TYPE,
		             "Could not create data source \"%s\"",
		             factory);
		goto unlock;
	}

	priv->sources = g_list_prepend (priv->sources, source);
	result = TRUE;

unlock:
	g_static_rw_lock_writer_unlock (&priv->rw_lock);

	if (result && source) {
		g_signal_emit (sources, signals [SOURCE_ADDED], 0, source);
	}

	return source;
}

/**
 * pkd_sources_register:
 * @sources: A #PkdSources
 * @factory: The name of the factory
 * @factory_func: A callback to create the factory instances
 * @user_data: user data for @factory_func
 *
 * Registers a #PkdSourceFactoryFunc with the #PkdSources service.  @factory
 * will be called when a new instance of the #PkdSource is needed.
 *
 * Side effects: None
 */
void
pkd_sources_register (PkdSources           *sources,
                      const gchar          *uid,
                      const gchar          *name,
                      const gchar          *version,
                      const gchar          *description,
                      PkdSourceFactoryFunc  factory_func,
                      gpointer              user_data)
{
	PkdSourcesPrivate *priv;
	PkdSourceInfo     *info;
	gchar             *key;

	g_return_if_fail (PKD_IS_SOURCES (sources));
	g_return_if_fail (uid != NULL);
	g_return_if_fail (name != NULL);
	g_return_if_fail (version != NULL);

	priv = sources->priv;

	if (g_hash_table_lookup (priv->factories, uid)) {
		g_warning (_("Data source type \"%s\" already registered."),
		           uid);
		return;
	}

	key = g_strdup (uid);
	info = g_object_new (PKD_TYPE_SOURCE_INFO,
	                     "uid", uid,
	                     "name", name,
	                     "version", version,
	                     "description", description,
	                     NULL);
	g_assert (info);
	pkd_source_info_set_factory_func (info, factory_func, user_data);
	g_hash_table_insert (priv->factories, key, info);
	g_message ("%s: Registered source type \"%s\".", G_STRFUNC, uid);
	g_signal_emit (sources, signals [SOURCE_INFO_ADDED], 0, info);
}

/**
 * pkd_sources_get_registered:
 * @sources: A #PkdSources
 *
 * Retrieves a list containing the #PkdSourceInfo source types available
 * for creation.
 *
 * Return value:
 *       A newly allocated #GList containing the #PkdSourceInfo instances.
 *       The list is owned by the caller and should be freed using
 *       g_list_free() after freeing each source info with g_object_unref().
 *
 * Side effects:
 *       None.
 */
GList*
pkd_sources_get_registered (PkdSources *sources)
{
	PkdSourcesPrivate *priv;
	GList *list = NULL;
	GHashTableIter iter;
	PkdSourceInfo *info;

	g_return_val_if_fail (PKD_IS_SOURCES (sources), NULL);

	priv = sources->priv;

	g_static_rw_lock_reader_lock (&priv->rw_lock);
	g_hash_table_iter_init (&iter, priv->factories);
	while (g_hash_table_iter_next (&iter, NULL, (gpointer*)&info)) {
		list = g_list_prepend (list, g_object_ref (info));
	}
	g_static_rw_lock_reader_unlock (&priv->rw_lock);

	return list;
}

void
pkd_sources_emit_source_added (PkdSources *sources,
                               PkdSource  *source)
{
	g_return_if_fail (PKD_IS_SOURCES (sources));
	g_return_if_fail (PKD_IS_SOURCE (source));
	g_signal_emit (sources, signals [SOURCE_ADDED], 0, source);
}

/**************************************************************************
 *                        GObject Class Methods                           *
 **************************************************************************/

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

	/**
	 * PkdSources::source-added:
	 * @source: a #PkdSource
	 *
	 * The "source-added" signal.  This signal is emmitted when a new source
	 * is created.
	 */
	signals [SOURCE_ADDED] = g_signal_new ("source-added",
	                                       PKD_TYPE_SOURCES,
	                                       G_SIGNAL_RUN_FIRST,
	                                       0,
	                                       NULL,
	                                       NULL,
	                                       g_cclosure_marshal_VOID__OBJECT,
	                                       G_TYPE_NONE,
	                                       1,
	                                       PKD_TYPE_SOURCE);

	/**
	 * PkdSource::source-info-added:
	 * @source_info: A #PkdSourceInfo
	 *
	 * The "source-info-added" signal.  This signal is emitted when a new
	 * #PkdSourceInfo is created.
	 */
	signals [SOURCE_INFO_ADDED] = g_signal_new ("source-info-added",
	                                            PKD_TYPE_SOURCES,
	                                            G_SIGNAL_RUN_FIRST,
	                                            0,
	                                            NULL,
	                                            NULL,
	                                            g_cclosure_marshal_VOID__OBJECT,
	                                            G_TYPE_NONE,
	                                            1,
	                                            PKD_TYPE_SOURCE_INFO);
}

static void
pkd_sources_init (PkdSources *sources)
{
	sources->priv = G_TYPE_INSTANCE_GET_PRIVATE (sources,
	                                             PKD_TYPE_SOURCES,
	                                             PkdSourcesPrivate);
	sources->priv->factories = g_hash_table_new_full (
			g_str_hash, g_str_equal, g_free, g_object_unref);
}

static void
pkd_sources_init_service (PkdServiceIface *iface)
{
}
