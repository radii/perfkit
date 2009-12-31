/* pkd-pipeline.h
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

#if !defined (__PERFKIT_DAEMON_INSIDE__) && !defined (PERFKIT_COMPILATION)
#error "Only <perfkit-daemon/perfkit-daemon.h> can be included directly."
#endif

#ifndef __PKD_PIPELINE_H__
#define __PKD_PIPELINE_H__

#include <glib.h>

#include "pkd-listener.h"
#include "pkd-channel.h"
#include "pkd-encoder-info.h"
#include "pkd-source-info.h"
#include "pkd-source.h"
#include "pkd-subscription.h"

G_BEGIN_DECLS

void   pkd_pipeline_init             (void);
void   pkd_pipeline_run              (void);
void   pkd_pipeline_quit             (void);
void   pkd_pipeline_shutdown         (void);
void   pkd_pipeline_add_channel      (PkdChannel      *channel);
void   pkd_pipeline_add_encoder_info (PkdEncoderInfo  *encoder_info);
void   pkd_pipeline_add_listener     (PkdListener     *listener);
void   pkd_pipeline_add_source       (PkdSource       *source);
void   pkd_pipeline_add_source_info  (PkdSourceInfo   *source_info);
void   pkd_pipeline_add_subscription (PkdSubscription *subscription);
GList* pkd_pipeline_get_channels     (void);

G_END_DECLS

#endif /* __PKD_PIPELINE_H__ */
