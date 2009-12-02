/* pk-channel-priv.h
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

#ifndef __PK_CHANNEL_PRIV_H__
#define __PK_CHANNEL_PRIV_H__

#include "pk-channel.h"
#include "pk-connection.h"

G_BEGIN_DECLS

PkChannel* pk_channel_new (PkConnection *connection,
                           gint          channel_id);

G_END_DECLS

#endif /* __PK_CHANNEL_PRIV_H__ */
