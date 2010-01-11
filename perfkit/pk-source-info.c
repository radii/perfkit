/* pk-source-info.c
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "pk-connection.h"
#include "pk-source-info.h"

G_DEFINE_TYPE(PkSourceInfo, pk_source_info, G_TYPE_OBJECT)

/**
 * SECTION:pk-source_info
 * @title: PkSourceInfo
 * @short_description: 
 *
 * 
 */

struct _PkSourceInfoPrivate
{
	PkConnection *conn;
};

enum
{
	PROP_0,
	PROP_CONN,
};

static void
pk_source_info_finalize (GObject *object)
{
	G_OBJECT_CLASS(pk_source_info_parent_class)->finalize(object);
}

static void
pk_source_info_dispose (GObject *object)
{
	G_OBJECT_CLASS(pk_source_info_parent_class)->dispose(object);
}

static void
pk_source_info_class_init (PkSourceInfoClass *klass)
{
	GObjectClass *object_class;

	object_class = G_OBJECT_CLASS (klass);
	object_class->finalize = pk_source_info_finalize;
	object_class->dispose = pk_source_info_dispose;
	g_type_class_add_private(object_class, sizeof(PkSourceInfoPrivate));
}

static void
pk_source_info_init (PkSourceInfo *source_info)
{
	source_info->priv = G_TYPE_INSTANCE_GET_PRIVATE(source_info,
	                                                PK_TYPE_SOURCE_INFO,
	                                                PkSourceInfoPrivate);
}
