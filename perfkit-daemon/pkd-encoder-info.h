/* pkd-encoder-info.h
 *
 * Copyright (C) 2009 Christian Hergert
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __PKD_ENCODER_INFO_H__
#define __PKD_ENCODER_INFO_H__

#include <glib-object.h>

#include "pkd-encoder.h"

G_BEGIN_DECLS

#define PKD_TYPE_ENCODER_INFO            (pkd_encoder_info_get_type())
#define PKD_ENCODER_INFO(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), PKD_TYPE_ENCODER_INFO, PkdEncoderInfo))
#define PKD_ENCODER_INFO_CONST(obj)      (G_TYPE_CHECK_INSTANCE_CAST ((obj), PKD_TYPE_ENCODER_INFO, PkdEncoderInfo const))
#define PKD_ENCODER_INFO_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  PKD_TYPE_ENCODER_INFO, PkdEncoderInfoClass))
#define PKD_IS_ENCODER_INFO(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), PKD_TYPE_ENCODER_INFO))
#define PKD_IS_ENCODER_INFO_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  PKD_TYPE_ENCODER_INFO))
#define PKD_ENCODER_INFO_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  PKD_TYPE_ENCODER_INFO, PkdEncoderInfoClass))
#define PKD_ENCODER_INFO_ERROR           (pkd_encoder_info_error_quark())

typedef struct _PkdEncoderInfo        PkdEncoderInfo;
typedef struct _PkdEncoderInfoClass   PkdEncoderInfoClass;
typedef struct _PkdEncoderInfoPrivate PkdEncoderInfoPrivate;

typedef PkdEncoder* (*PkdEncoderFactory) (void);

/**
 * PkdStaticEncoderInfo:
 *
 * The #PkdStaticEncoderInfo structure provides a table of information for
 * encoder plugins to include in their namespace under the
 * "pkd_encoder_plugin" symbol.  This symbol will be read at runtime and
 * the data extracted.
 */
typedef struct
{
	const gchar      *uid;
	const gchar      *version;
	const gchar      *name;
	const gchar      *description;
	PkdEncoderFactory  factory;
} PkdStaticEncoderInfo;

/**
 * PkdEncoderInfoError:
 * @PKD_ENCODER_INFO_ERROR_FILENAME
 * @PKD_ENCODER_INFO_ERROR_MODULE
 * @PKD_ENCODER_INFO_ERROR_SYMBOL
 *
 * #PkdEncoderInfo error enumeration.
 */
typedef enum
{
	PKD_ENCODER_INFO_ERROR_FILENAME,
	PKD_ENCODER_INFO_ERROR_MODULE,
	PKD_ENCODER_INFO_ERROR_SYMBOL,
} PkdEncoderInfoError;

struct _PkdEncoderInfo
{
	GObject parent;

	/*< private >*/
	PkdEncoderInfoPrivate *priv;
};

struct _PkdEncoderInfoClass
{
	GObjectClass parent_class;
};

GType          pkd_encoder_info_get_type        (void) G_GNUC_CONST;
GQuark         pkd_encoder_info_error_quark     (void) G_GNUC_CONST;
GList*         pkd_encoder_info_find_all        (void);
PkdEncoderInfo* pkd_encoder_info_new             (void);
gboolean       pkd_encoder_info_load_from_file  (PkdEncoderInfo  *encoder_info,
                                               const gchar     *filename,
                                               GError         **error);
gboolean       pkd_encoder_info_conflicts       (PkdEncoderInfo  *encoder_info,
                                               PkdEncoderInfo   *other);
const gchar*   pkd_encoder_info_get_uid         (PkdEncoderInfo  *encoder_info);
const gchar*   pkd_encoder_info_get_name        (PkdEncoderInfo  *encoder_info);
const gchar*   pkd_encoder_info_get_description (PkdEncoderInfo  *encoder_info);
const gchar*   pkd_encoder_info_get_version     (PkdEncoderInfo  *encoder_info);
PkdEncoder*     pkd_encoder_info_create          (PkdEncoderInfo  *encoder_info);

G_END_DECLS

#endif /* __PKD_ENCODER_INFO_H__ */
