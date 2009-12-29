/* pkd-dbus.h
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

#ifndef __PKD_DBUS_H__
#define __PKD_DBUS_H__

#include <glib-object.h>

#include <perfkit-daemon/perfkit-daemon.h>

G_BEGIN_DECLS

#define PKD_TYPE_DBUS            (pkd_dbus_get_type())
#define PKD_DBUS(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), PKD_TYPE_DBUS, PkdDBus))
#define PKD_DBUS_CONST(obj)      (G_TYPE_CHECK_INSTANCE_CAST ((obj), PKD_TYPE_DBUS, PkdDBus const))
#define PKD_DBUS_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  PKD_TYPE_DBUS, PkdDBusClass))
#define PKD_IS_DBUS(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), PKD_TYPE_DBUS))
#define PKD_IS_DBUS_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  PKD_TYPE_DBUS))
#define PKD_DBUS_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  PKD_TYPE_DBUS, PkdDBusClass))

typedef struct _PkdDBus        PkdDBus;
typedef struct _PkdDBusClass   PkdDBusClass;
typedef struct _PkdDBusPrivate PkdDBusPrivate;

struct _PkdDBus
{
	PkdListener parent;

	/*< private >*/
	PkdDBusPrivate *priv;
};

struct _PkdDBusClass
{
	PkdListenerClass parent_class;
};

GType        pkd_dbus_get_type (void) G_GNUC_CONST;
PkdListener* pkd_dbus_new      (void);

G_END_DECLS

#endif /* __PKD_DBUS_H__ */
