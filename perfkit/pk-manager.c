/* pk-manager.c
 *
 * Copyright (C) 2010 Christian Hergert <chris@dronelabs.com>
 *
 * This file is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 * 
 * This file is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "pk-connection-lowlevel.h"
#include "pk-log.h"
#include "pk-manager.h"
#include "pk-plugin.h"

/**
 * SECTION:pk-manager
 * @title: PkManager
 * @short_description: 
 *
 * Section overview.
 */

G_DEFINE_TYPE(PkManager, pk_manager, G_TYPE_OBJECT)

struct _PkManagerPrivate
{
	PkConnection *conn;
};

GList*
pk_manager_get_plugins (PkManager *manager) /* IN */
{
	PkManagerPrivate *priv;
	gchar **plugins = NULL;
	GError *error = NULL;
	GList *list = NULL;
	gint i;

	g_return_val_if_fail(PK_IS_MANAGER(manager), NULL);

	ENTRY;
	priv = manager->priv;
	if (!pk_connection_manager_get_plugins(priv->conn, &plugins, &error)) {
		GOTO(error);
	}
	g_assert(plugins);
	for (i = 0; plugins[i]; i++) {
		list = g_list_prepend(list, g_object_new(PK_TYPE_PLUGIN,
		                                         "id", plugins[i],
		                                         "connection", priv->conn,
		                                         NULL));
	}
	g_strfreev(plugins);
  error:
	RETURN(list);
}

static void
pk_manager_finalize (GObject *object) /* IN */
{
	G_OBJECT_CLASS(pk_manager_parent_class)->finalize(object);
}

static void
pk_manager_class_init (PkManagerClass *klass) /* IN */
{
	GObjectClass *object_class;

	object_class = G_OBJECT_CLASS(klass);
	object_class->finalize = pk_manager_finalize;
	g_type_class_add_private(object_class, sizeof(PkManagerPrivate));
}

static void
pk_manager_init (PkManager *manager) /* IN */
{
	manager->priv = G_TYPE_INSTANCE_GET_PRIVATE(manager,
	                                            PK_TYPE_MANAGER,
	                                            PkManagerPrivate);
}
