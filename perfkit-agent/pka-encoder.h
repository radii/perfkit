/* pka-encoder.h
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

#if !defined (__PERFKIT_AGENT_INSIDE__) && !defined (PERFKIT_COMPILATION)
#error "Only <perfkit-agent/perfkit-agent.h> can be included directly."
#endif

#ifndef __PKA_ENCODER_H__
#define __PKA_ENCODER_H__

#include <glib-object.h>

#include "pka-manifest.h"
#include "pka-sample.h"

G_BEGIN_DECLS

#define PKA_TYPE_ENCODER             (pka_encoder_get_type())
#define PKA_ENCODER(o)               (G_TYPE_CHECK_INSTANCE_CAST((o),    PKA_TYPE_ENCODER, PkaEncoder))
#define PKA_IS_ENCODER(o)            (G_TYPE_CHECK_INSTANCE_TYPE((o),    PKA_TYPE_ENCODER))
#define PKA_ENCODER_GET_INTERFACE(o) (G_TYPE_INSTANCE_GET_INTERFACE((o), PKA_TYPE_ENCODER, PkaEncoderIface))

typedef struct _PkaEncoder      PkaEncoder;
typedef struct _PkaEncoderIface PkaEncoderIface;

struct _PkaEncoderIface
{
	GTypeInterface parent;

	gboolean (*encode_samples)  (PkaEncoder    *encoder,
	                             PkaManifest   *manifest,
	                             PkaSample    **sample,
	                             gint           n_samples,
	                             gchar        **data,
	                             gsize         *dapka_len);
	gboolean (*encode_manifest) (PkaEncoder    *encoder,
	                             PkaManifest   *manifest,
	                             gchar        **data,
	                             gsize         *dapka_len);
};

GType    pka_encoder_get_type        (void) G_GNUC_CONST;
gboolean pka_encoder_encode_samples  (PkaEncoder     *encoder,
                                      PkaManifest    *manifest,
                                      PkaSample     **samples,
                                      gint            n_samples,
                                      gchar         **data,
                                      gsize          *dapka_len);
gboolean pka_encoder_encode_manifest (PkaEncoder     *encoder,
                                      PkaManifest    *manifest,
                                      gchar         **data,
                                      gsize          *dapka_len);

G_END_DECLS

#endif /* __PKA_ENCODER_H__ */
