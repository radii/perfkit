/* pka-pipeline.h
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

#ifndef __PKA_PIPELINE_H__
#define __PKA_PIPELINE_H__

#include <glib.h>

#include "pka-listener.h"
#include "pka-channel.h"
#include "pka-encoder-info.h"
#include "pka-source-info.h"
#include "pka-source.h"
#include "pka-subscription.h"

G_BEGIN_DECLS

void   pka_pipeline_init                (void);
void   pka_pipeline_run                 (void);
void   pka_pipeline_quit                (void);
void   pka_pipeline_shutdown            (void);
void   pka_pipeline_add_channel         (PkaChannel      *channel);
void   pka_pipeline_add_encoder         (PkaEncoder      *encoder);
void   pka_pipeline_add_encoder_info    (PkaEncoderInfo  *encoder_info);
void   pka_pipeline_add_listener        (PkaListener     *listener);
void   pka_pipeline_add_source          (PkaSource       *source);
void   pka_pipeline_add_source_info     (PkaSourceInfo   *source_info);
void   pka_pipeline_add_subscription    (PkaSubscription *subscription);
void   pka_pipeline_remove_channel      (PkaChannel      *channel);
GList* pka_pipeline_get_channels        (void);
GList* pka_pipeline_get_encoder_plugins (void);
GList* pka_pipeline_get_source_plugins  (void);

G_END_DECLS

#endif /* __PKA_PIPELINE_H__ */
