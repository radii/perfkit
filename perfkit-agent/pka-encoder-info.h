/* pka-encoder-info.h
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

#if !defined (__PERFKIT_AGENT_INSIDE__) && !defined (PERFKIT_COMPILATION)
#error "Only <perfkit-agent/perfkit-agent.h> can be included directly."
#endif

#ifndef __PKA_ENCODER_INFO_H__
#define __PKA_ENCODER_INFO_H__

#include <glib-object.h>

#include "pka-encoder.h"

G_BEGIN_DECLS

#define PKA_TYPE_ENCODER_INFO            (pka_encoder_info_get_type())
#define PKA_ENCODER_INFO(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), PKA_TYPE_ENCODER_INFO, PkaEncoderInfo))
#define PKA_ENCODER_INFO_CONST(obj)      (G_TYPE_CHECK_INSTANCE_CAST ((obj), PKA_TYPE_ENCODER_INFO, PkaEncoderInfo const))
#define PKA_ENCODER_INFO_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  PKA_TYPE_ENCODER_INFO, PkaEncoderInfoClass))
#define PKA_IS_ENCODER_INFO(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), PKA_TYPE_ENCODER_INFO))
#define PKA_IS_ENCODER_INFO_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  PKA_TYPE_ENCODER_INFO))
#define PKA_ENCODER_INFO_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  PKA_TYPE_ENCODER_INFO, PkaEncoderInfoClass))
#define PKA_ENCODER_INFO_ERROR           (pka_encoder_info_error_quark())

typedef struct _PkaEncoderInfo        PkaEncoderInfo;
typedef struct _PkaEncoderInfoClass   PkaEncoderInfoClass;
typedef struct _PkaEncoderInfoPrivate PkaEncoderInfoPrivate;

typedef PkaEncoder* (*PkaEncoderFactory) (void);

/**
 * PkaStaticEncoderInfo:
 *
 * The #PkaStaticEncoderInfo structure provides a table of information for
 * encoder plugins to include in their namespace under the
 * "pka_encoder_plugin" symbol.  This symbol will be read at runtime and
 * the data extracted.
 */
typedef struct
{
	const gchar      *uid;
	const gchar      *version;
	const gchar      *name;
	const gchar      *description;
	PkaEncoderFactory  factory;
} PkaStaticEncoderInfo;

/**
 * PkaEncoderInfoError:
 * @PKA_ENCODER_INFO_ERROR_FILENAME
 * @PKA_ENCODER_INFO_ERROR_MODULE
 * @PKA_ENCODER_INFO_ERROR_SYMBOL
 *
 * #PkaEncoderInfo error enumeration.
 */
typedef enum
{
	PKA_ENCODER_INFO_ERROR_FILENAME,
	PKA_ENCODER_INFO_ERROR_MODULE,
	PKA_ENCODER_INFO_ERROR_SYMBOL,
} PkaEncoderInfoError;

struct _PkaEncoderInfo
{
	GObject parent;

	/*< private >*/
	PkaEncoderInfoPrivate *priv;
};

struct _PkaEncoderInfoClass
{
	GObjectClass parent_class;
};

GType            pka_encoder_info_get_type        (void) G_GNUC_CONST;
GQuark           pka_encoder_info_error_quark     (void) G_GNUC_CONST;
GList*           pka_encoder_info_find_all        (void);
PkaEncoderInfo*  pka_encoder_info_new             (void);
gboolean         pka_encoder_info_load_from_file  (PkaEncoderInfo   *encoder_info,
                                                   const gchar      *filename,
                                                   GError          **error);
gboolean         pka_encoder_info_conflicts       (PkaEncoderInfo   *encoder_info,
                                                   PkaEncoderInfo   *other);
const gchar*     pka_encoder_info_get_uid         (PkaEncoderInfo   *encoder_info);
const gchar*     pka_encoder_info_get_name        (PkaEncoderInfo   *encoder_info);
const gchar*     pka_encoder_info_get_description (PkaEncoderInfo   *encoder_info);
const gchar*     pka_encoder_info_get_version     (PkaEncoderInfo   *encoder_info);
PkaEncoder*      pka_encoder_info_create          (PkaEncoderInfo   *encoder_info);

G_END_DECLS

#endif /* __PKA_ENCODER_INFO_H__ */
