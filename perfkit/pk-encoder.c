/* pk-encoder.c
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
#include "pk-encoder.h"

G_DEFINE_TYPE(PkEncoder, pk_encoder, G_TYPE_OBJECT)

/**
 * SECTION:pk-encoder
 * @title: PkEncoder
 * @short_description: 
 *
 * 
 */

struct _PkEncoderPrivate
{
	PkConnection *conn;
};

/**
 * pk_encoder_new:
 *
 * Creates a new instance of #PkEncoder.
 *
 * Return value: the newly created #PkEncoder instance.
 */
PkEncoder*
pk_encoder_new (void)
{
	return g_object_new (PK_TYPE_ENCODER, NULL);
}

static void
pk_encoder_finalize (GObject *object)
{
	G_OBJECT_CLASS(pk_encoder_parent_class)->finalize(object);
}

static void
pk_encoder_dispose (GObject *object)
{
	G_OBJECT_CLASS(pk_encoder_parent_class)->dispose(object);
}

static void
pk_encoder_class_init (PkEncoderClass *klass)
{
	GObjectClass *object_class;

	object_class = G_OBJECT_CLASS (klass);
	object_class->finalize = pk_encoder_finalize;
	object_class->dispose = pk_encoder_dispose;
	g_type_class_add_private(object_class, sizeof(PkEncoderPrivate));
}

static void
pk_encoder_init (PkEncoder *encoder)
{
	encoder->priv = G_TYPE_INSTANCE_GET_PRIVATE(encoder,
	                                            PK_TYPE_ENCODER,
	                                            PkEncoderPrivate);
}
