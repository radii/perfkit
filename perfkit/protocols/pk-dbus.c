/* pk-dbus.c
 *
 * Copyright (C) 2010 Christian Hergert
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

#include "pk-dbus.h"

G_DEFINE_TYPE(PkDbus, pk_dbus, G_TYPE_OBJECT)

struct _PkDbusPrivate
{
	gpointer dummy;
};

static void
pk_dbus_finalize (GObject *object)
{
	G_OBJECT_CLASS(pk_dbus_parent_class)->finalize(object);
}

static void
pk_dbus_class_init (PkDbusClass *klass)
{
	GObjectClass *object_class;

	object_class = G_OBJECT_CLASS (klass);
	object_class->finalize = pk_dbus_finalize;
	g_type_class_add_private(object_class, sizeof(PkDbusPrivate));
}

static void
pk_dbus_init (PkDbus *dbus)
{
	dbus->priv = G_TYPE_INSTANCE_GET_PRIVATE(dbus, PK_TYPE_DBUS, PkDbusPrivate);
}
