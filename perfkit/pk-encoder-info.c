/* pk-encoder-info.c
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
#include "pk-encoder-info.h"

G_DEFINE_TYPE (PkEncoderInfo, pk_encoder_info, G_TYPE_OBJECT)

/**
 * SECTION:pk-encoder_info
 * @title: PkEncoderInfo
 * @short_description: 
 *
 * 
 */

struct _PkEncoderInfoPrivate
{
	PkConnection *conn;
};

static void
pk_encoder_info_finalize (GObject *object)
{
	PkEncoderInfoPrivate *priv;

	g_return_if_fail (PK_IS_ENCODER_INFO (object));

	priv = PK_ENCODER_INFO (object)->priv;

	G_OBJECT_CLASS (pk_encoder_info_parent_class)->finalize (object);
}

static void
pk_encoder_info_class_init (PkEncoderInfoClass *klass)
{
	GObjectClass *object_class;

	object_class = G_OBJECT_CLASS (klass);
	object_class->finalize = pk_encoder_info_finalize;
	g_type_class_add_private (object_class, sizeof (PkEncoderInfoPrivate));
}

static void
pk_encoder_info_init (PkEncoderInfo *encoder_info)
{
	encoder_info->priv = G_TYPE_INSTANCE_GET_PRIVATE (encoder_info,
	                                                  PK_TYPE_ENCODER_INFO,
	                                                  PkEncoderInfoPrivate);
}
