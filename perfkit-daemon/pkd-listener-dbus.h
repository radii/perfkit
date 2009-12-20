/* pkd-listener-dbus.h
 *
 * Copyright (C) 2009 Christian Hergert <chris@dronelabs.com>
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

#ifndef __PKD_LISTENER_DBUS_H__
#define __PKD_LISTENER_DBUS_H__

#include <glib-object.h>

#include "pkd-listener.h"

G_BEGIN_DECLS

#define PKD_TYPE_LISTENER_DBUS            (pkd_listener_dbus_get_type())
#define PKD_LISTENER_DBUS(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), PKD_TYPE_LISTENER_DBUS, PkdListenerDBus))
#define PKD_LISTENER_DBUS_CONST(obj)      (G_TYPE_CHECK_INSTANCE_CAST ((obj), PKD_TYPE_LISTENER_DBUS, PkdListenerDBus const))
#define PKD_LISTENER_DBUS_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  PKD_TYPE_LISTENER_DBUS, PkdListenerDBusClass))
#define PKD_IS_LISTENER_DBUS(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), PKD_TYPE_LISTENER_DBUS))
#define PKD_IS_LISTENER_DBUS_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  PKD_TYPE_LISTENER_DBUS))
#define PKD_LISTENER_DBUS_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  PKD_TYPE_LISTENER_DBUS, PkdListenerDBusClass))

typedef struct _PkdListenerDBus        PkdListenerDBus;
typedef struct _PkdListenerDBusClass   PkdListenerDBusClass;
typedef struct _PkdListenerDBusPrivate PkdListenerDBusPrivate;

struct _PkdListenerDBus
{
	GObject parent;

	/*< private >*/
	PkdListenerDBusPrivate *priv;
};

struct _PkdListenerDBusClass
{
	GObjectClass parent_class;
};

GType pkd_listener_dbus_get_type (void) G_GNUC_CONST;
void  pkd_listener_dbus_register (void);

G_END_DECLS

#endif /* __PKD_LISTENER_DBUS_H__ */
