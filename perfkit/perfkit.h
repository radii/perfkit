/* perfkit.h
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

#ifndef __PERFKIT_H__
#define __PERFKIT_H__

#define __PERFKIT_INSIDE__

#include "pk-connection.h"
#include "pk-connection-lowlevel.h"
#include "pk-channel.h"
#include "pk-encoder.h"
#include "pk-manifest.h"
#include "pk-plugin.h"
#include "pk-sample.h"
#include "pk-source.h"
#include "pk-subscription.h"
#include "pk-version.h"

typedef enum
{
	PK_CHANNEL_READY = 1,
	PK_CHANNEL_RUNNING,
	PK_CHANNEL_MUTED,
	PK_CHANNEL_STOPPED,
	PK_CHANNEL_FAILED,
} PkChannelState;

typedef struct
{
	gchar  *target;
	GPid    pid;
	gchar  *working_dir;
	gchar **env;
	gchar **args;
} PkSpawnInfo;

#undef __PERFKIT_INSIDE__

#endif /* __PERFKIT_H__ */
