/* pk-connection-dbus.h
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

#ifndef __PK_CONNECTION_DBUS_H__
#define __PK_CONNECTION_DBUS_H__

#include "pk-connection.h"

G_BEGIN_DECLS

#define PK_TYPE_CONNECTION_DBUS            (pk_connection_dbus_get_type())
#define PK_CONNECTION_DBUS(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), PK_TYPE_CONNECTION_DBUS, PkConnectionDBus))
#define PK_CONNECTION_DBUS_CONST(obj)      (G_TYPE_CHECK_INSTANCE_CAST ((obj), PK_TYPE_CONNECTION_DBUS, PkConnectionDBus const))
#define PK_CONNECTION_DBUS_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  PK_TYPE_CONNECTION_DBUS, PkConnectionDBusClass))
#define PK_IS_CONNECTION_DBUS(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), PK_TYPE_CONNECTION_DBUS))
#define PK_IS_CONNECTION_DBUS_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  PK_TYPE_CONNECTION_DBUS))
#define PK_CONNECTION_DBUS_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  PK_TYPE_CONNECTION_DBUS, PkConnectionDBusClass))
#define PK_CONNECTION_DBUS_ERROR           (pk_connection_dbus_error_quark())

/**
 * PkConnectionDBusError:
 * @PK_CONNECTION_DBUS_ERROR_STATE: The connection is in an invalid state
 *    for the operation.
 * @PK_CONNECTION_DBUS_ERROR_NOT_AVAILABLE: The DBus destination could
 *    not be reached.
 *
 * The #PkConnectionDBus error enumeration.
 */
typedef enum
{
	PK_CONNECTION_DBUS_ERROR_STATE,
	PK_CONNECTION_DBUS_ERROR_NOT_AVAILABLE,
	PK_CONNECTION_DBUS_ERROR_DBUS,
} PkConnectionDBusError;

typedef struct _PkConnectionDBus        PkConnectionDBus;
typedef struct _PkConnectionDBusClass   PkConnectionDBusClass;
typedef struct _PkConnectionDBusPrivate PkConnectionDBusPrivate;

struct _PkConnectionDBus
{
	PkConnection parent;

	/*< private >*/
	PkConnectionDBusPrivate *priv;
};

struct _PkConnectionDBusClass
{
	PkConnectionClass parent_class;
};

GQuark pk_connection_dbus_error_quark (void) G_GNUC_CONST;
GType  pk_connection_dbus_get_type    (void) G_GNUC_CONST;

G_END_DECLS

#endif /* __PK_CONNECTION_DBUS_H__ */
