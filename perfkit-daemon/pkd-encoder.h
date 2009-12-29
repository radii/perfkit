/* pkd-encoder.h
 *
 * Copyright (C) 2009 Christian Hergert <chris@dronelabs.com>
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

#ifndef __PKD_ENCODER_H__
#define __PKD_ENCODER_H__

#include <glib-object.h>

#include "pkd-manifest.h"
#include "pkd-sample.h"

G_BEGIN_DECLS

#define PKD_TYPE_ENCODER             (pkd_encoder_get_type())
#define PKD_ENCODER(o)               (G_TYPE_CHECK_INSTANCE_CAST((o),    PKD_TYPE_ENCODER, PkdEncoder))
#define PKD_IS_ENCODER(o)            (G_TYPE_CHECK_INSTANCE_TYPE((o),    PKD_TYPE_ENCODER))
#define PKD_ENCODER_GET_INTERFACE(o) (G_TYPE_INSTANCE_GET_INTERFACE((o), PKD_TYPE_ENCODER, PkdEncoderIface))

typedef struct _PkdEncoder      PkdEncoder;
typedef struct _PkdEncoderIface PkdEncoderIface;

struct _PkdEncoderIface
{
	GTypeInterface parent;

	gboolean (*encode_samples)  (PkdEncoder   *encoder,
	                             PkdSample   **sample,
	                             gint         n_samples,
	                             gchar      **data,
	                             gsize       *dapkd_len);
	gboolean (*encode_manifest) (PkdEncoder   *encoder,
	                             PkdManifest  *manifest,
	                             gchar      **data,
	                             gsize       *dapkd_len);
};

GType    pkd_encoder_get_type        (void) G_GNUC_CONST;
gboolean pkd_encoder_encode_samples  (PkdEncoder    *encoder,
                                     PkdSample    **samples,
                                     gint          n_samples,
                                     gchar       **data,
                                     gsize        *dapkd_len);
gboolean pkd_encoder_encode_manifest (PkdEncoder    *encoder,
                                     PkdManifest   *manifest,
                                     gchar       **data,
                                     gsize        *dapkd_len);

G_END_DECLS

#endif /* __PKD_ENCODER_H__ */
