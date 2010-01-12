/* pk-dbus.h
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

#ifndef __PK_DBUS_H__
#define __PK_DBUS_H__

#include <perfkit/perfkit.h>

G_BEGIN_DECLS

#define PK_TYPE_DBUS            (pk_dbus_get_type())
#define PK_DBUS(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), PK_TYPE_DBUS, PkDbus))
#define PK_DBUS_CONST(obj)      (G_TYPE_CHECK_INSTANCE_CAST ((obj), PK_TYPE_DBUS, PkDbus const))
#define PK_DBUS_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  PK_TYPE_DBUS, PkDbusClass))
#define PK_IS_DBUS(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), PK_TYPE_DBUS))
#define PK_IS_DBUS_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  PK_TYPE_DBUS))
#define PK_DBUS_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  PK_TYPE_DBUS, PkDbusClass))

typedef struct _PkDbus        PkDbus;
typedef struct _PkDbusClass   PkDbusClass;
typedef struct _PkDbusPrivate PkDbusPrivate;

struct _PkDbus
{
	PkProtocol parent;

	/*< private >*/
	PkDbusPrivate *priv;
};

struct _PkDbusClass
{
	PkProtocolClass parent_class;
};

GType pk_dbus_get_type (void) G_GNUC_CONST;

G_END_DECLS

#endif /* __PK_DBUS_H__ */
