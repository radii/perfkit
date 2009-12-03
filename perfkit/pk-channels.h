/* pk-channels.h
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

#ifndef __PK_CHANNELS_H__
#define __PK_CHANNELS_H__

#include <glib-object.h>

#include "pk-channel.h"

G_BEGIN_DECLS

#define PK_TYPE_CHANNELS            (pk_channels_get_type ())
#define PK_CHANNELS(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), PK_TYPE_CHANNELS, PkChannels))
#define PK_CHANNELS_CONST(obj)      (G_TYPE_CHECK_INSTANCE_CAST ((obj), PK_TYPE_CHANNELS, PkChannels const))
#define PK_CHANNELS_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  PK_TYPE_CHANNELS, PkChannelsClass))
#define PK_IS_CHANNELS(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), PK_TYPE_CHANNELS))
#define PK_IS_CHANNELS_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  PK_TYPE_CHANNELS))
#define PK_CHANNELS_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  PK_TYPE_CHANNELS, PkChannelsClass))

typedef struct _PkChannels        PkChannels;
typedef struct _PkChannelsClass   PkChannelsClass;
typedef struct _PkChannelsPrivate PkChannelsPrivate;

struct _PkChannels
{
	GObject parent;

	/*< private >*/
	PkChannelsPrivate *priv;
};

struct _PkChannelsClass
{
	GObjectClass parent_class;
};

GType      pk_channels_get_type (void) G_GNUC_CONST;
GList*     pk_channels_find_all (PkChannels *channels);
PkChannel* pk_channels_get      (PkChannels *channels,
                                 gint        channel_id);
PkChannel* pk_channels_add      (PkChannels *channels);

G_END_DECLS

#endif /* __PK_CHANNELS_H__ */
