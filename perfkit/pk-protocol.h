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

#if !defined (__PERFKIT_INSIDE__) && !defined (PERFKIT_COMPILATION)
#error "Only <perfkit/perfkit.h> can be included directly."
#endif

#ifndef __PK_PROTOCOL_H__
#define __PK_PROTOCOL_H__

#include <glib-object.h>

#include "pk-channel.h"
#include "pk-encoder-info.h"
#include "pk-subscription.h"

G_BEGIN_DECLS

#define PK_TYPE_PROTOCOL             (pk_protocol_get_type())
#define PK_PROTOCOL(o)               (G_TYPE_CHECK_INSTANCE_CAST((o),    PK_TYPE_PROTOCOL, PkProtocol))
#define PK_IS_PROTOCOL(o)            (G_TYPE_CHECK_INSTANCE_TYPE((o),    PK_TYPE_PROTOCOL))
#define PK_PROTOCOL_GET_INTERFACE(o) (G_TYPE_INSTANCE_GET_INTERFACE((o), PK_TYPE_PROTOCOL, PkProtocolIface))

typedef struct _PkProtocol      PkProtocol;
typedef struct _PkProtocolIface PkProtocolIface;

struct _PkProtocolIface
{
	GTypeInterface parent;

	/*
	 * Manager RPCs.
	 */
	gboolean (*manager_ping)         (PkProtocol *protocol,
	                                  GTimeVal   *tv);
	GList*   (*manager_get_channels) (PkProtocol *protocol);
};

GType           pk_protocol_get_type             (void) G_GNUC_CONST;

/*
 * Manager RPCs.
 */
gboolean        pk_protocol_manager_ping                (PkProtocol       *protocol,
                                                         GTimeVal         *tv);
GList*          pk_protocol_manager_get_channels        (PkProtocol       *protocol);
PkChannel*      pk_protocol_manager_create_channel      (PkProtocol       *protocol,
                                                         GPid              pid,
                                                         const gchar      *target,
                                                         gchar           **args,
                                                         gchar           **env,
                                                         const gchar      *working_dir,
                                                         GError          **error);
PkSubscription* pk_protocol_manager_create_subscription (PkProtocol       *protocol,
                                                         PkChannel        *channel,
                                                         gulong            buffer_size,
                                                         gulong            buffer_timeout,
                                                         PkEncoderInfo    *encoder_info);

G_END_DECLS

#endif /* __PK_PROTOCOL_H__ */
