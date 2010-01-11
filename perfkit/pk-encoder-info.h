/* pk-encoder-info.h
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

#ifndef __PK_ENCODER_INFO_H__
#define __PK_ENCODER_INFO_H__

#include <glib-object.h>

G_BEGIN_DECLS

#define PK_TYPE_ENCODER_INFO            (pk_encoder_info_get_type())
#define PK_ENCODER_INFO(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), PK_TYPE_ENCODER_INFO, PkEncoderInfo))
#define PK_ENCODER_INFO_CONST(obj)      (G_TYPE_CHECK_INSTANCE_CAST ((obj), PK_TYPE_ENCODER_INFO, PkEncoderInfo const))
#define PK_ENCODER_INFO_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  PK_TYPE_ENCODER_INFO, PkEncoderInfoClass))
#define PK_IS_ENCODER_INFO(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), PK_TYPE_ENCODER_INFO))
#define PK_IS_ENCODER_INFO_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  PK_TYPE_ENCODER_INFO))
#define PK_ENCODER_INFO_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  PK_TYPE_ENCODER_INFO, PkEncoderInfoClass))

typedef struct _PkEncoderInfo        PkEncoderInfo;
typedef struct _PkEncoderInfoClass   PkEncoderInfoClass;
typedef struct _PkEncoderInfoPrivate PkEncoderInfoPrivate;

struct _PkEncoderInfo
{
	GObject parent;

	/*< private >*/
	PkEncoderInfoPrivate *priv;
};

struct _PkEncoderInfoClass
{
	GObjectClass parent_class;
};

GType pk_encoder_info_get_type (void) G_GNUC_CONST;

G_END_DECLS

#endif /* __PK_ENCODER_INFO_H__ */
