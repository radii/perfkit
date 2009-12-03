/* pk-channel.h
 *
 * Copyright (C) 2009 Christian Hergert <chris@dronelabs.com>
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

#ifndef __PK_CHANNEL_H__
#define __PK_CHANNEL_H__

#include <glib-object.h>

G_BEGIN_DECLS

#define PK_TYPE_CHANNEL            (pk_channel_get_type ())
#define PK_CHANNEL(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), PK_TYPE_CHANNEL, PkChannel))
#define PK_CHANNEL_CONST(obj)      (G_TYPE_CHECK_INSTANCE_CAST ((obj), PK_TYPE_CHANNEL, PkChannel const))
#define PK_CHANNEL_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  PK_TYPE_CHANNEL, PkChannelClass))
#define PK_IS_CHANNEL(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), PK_TYPE_CHANNEL))
#define PK_IS_CHANNEL_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  PK_TYPE_CHANNEL))
#define PK_CHANNEL_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  PK_TYPE_CHANNEL, PkChannelClass))

typedef struct _PkChannel        PkChannel;
typedef struct _PkChannelClass   PkChannelClass;
typedef struct _PkChannelPrivate PkChannelPrivate;

struct _PkChannel
{
	GObject parent;

	/*< private >*/
	PkChannelPrivate *priv;
};

struct _PkChannelClass
{
	GObjectClass parent_class;
};

GType   pk_channel_get_type   (void) G_GNUC_CONST;
gint    pk_channel_get_id     (PkChannel *channel);
gchar*  pk_channel_get_target (PkChannel *channel);
gchar** pk_channel_get_args   (PkChannel *channel);

G_END_DECLS

#endif /* __PK_CHANNEL_H__ */
