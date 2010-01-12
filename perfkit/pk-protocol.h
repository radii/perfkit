/* pk-protocol.h
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

#ifndef __PK_PROTOCOL_H__
#define __PK_PROTOCOL_H__

#include <glib-object.h>

G_BEGIN_DECLS

#define PK_TYPE_PROTOCOL            (pk_protocol_get_type())
#define PK_PROTOCOL(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), PK_TYPE_PROTOCOL, PkProtocol))
#define PK_PROTOCOL_CONST(obj)      (G_TYPE_CHECK_INSTANCE_CAST ((obj), PK_TYPE_PROTOCOL, PkProtocol const))
#define PK_PROTOCOL_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  PK_TYPE_PROTOCOL, PkProtocolClass))
#define PK_IS_PROTOCOL(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), PK_TYPE_PROTOCOL))
#define PK_IS_PROTOCOL_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  PK_TYPE_PROTOCOL))
#define PK_PROTOCOL_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  PK_TYPE_PROTOCOL, PkProtocolClass))

typedef struct _PkProtocol        PkProtocol;
typedef struct _PkProtocolClass   PkProtocolClass;
typedef struct _PkProtocolPrivate PkProtocolPrivate;

struct _PkProtocol
{
	GObject parent;

	/*< private >*/
	PkProtocolPrivate *priv;
};

struct _PkProtocolClass
{
	GObjectClass parent_class;

	gboolean (*manager_ping) (PkProtocol *protocol, GTimeVal *tv);
	GList* (*manager_get_channels) (PkProtocol *protocol);
};

GType        pk_protocol_get_type (void) G_GNUC_CONST;
const gchar* pk_protocol_get_uri  (PkProtocol *protocol);

gboolean pk_protocol_manager_ping (PkProtocol *protocol, GTimeVal *tv);
GList* pk_protocol_manager_get_channels (PkProtocol *protocol);

G_END_DECLS

#endif /* __PK_PROTOCOL_H__ */
