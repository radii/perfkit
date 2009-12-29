/* pkd-channel.h
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

#ifndef __PKD_CHANNEL_H__
#define __PKD_CHANNEL_H__

#include <glib-object.h>

#include "pkd-manifest.h"
#include "pkd-sample.h"
#include "pkd-source-info.h"
#include "pkd-spawn-info.h"

G_BEGIN_DECLS

#define PKD_TYPE_CHANNEL            (pkd_channel_get_type())
#define PKD_CHANNEL(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), PKD_TYPE_CHANNEL, PkdChannel))
#define PKD_CHANNEL_CONST(obj)      (G_TYPE_CHECK_INSTANCE_CAST ((obj), PKD_TYPE_CHANNEL, PkdChannel const))
#define PKD_CHANNEL_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  PKD_TYPE_CHANNEL, PkdChannelClass))
#define PKD_IS_CHANNEL(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), PKD_TYPE_CHANNEL))
#define PKD_IS_CHANNEL_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  PKD_TYPE_CHANNEL))
#define PKD_CHANNEL_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  PKD_TYPE_CHANNEL, PkdChannelClass))

typedef struct _PkdChannel        PkdChannel;
typedef struct _PkdChannelClass   PkdChannelClass;
typedef struct _PkdChannelPrivate PkdChannelPrivate;

struct _PkdChannel
{
	GObject parent;

	/*< private >*/
	PkdChannelPrivate *priv;
};

struct _PkdChannelClass
{
	GObjectClass parent_class;
};

GType         pkd_channel_get_type            (void) G_GNUC_CONST;
PkdChannel*   pkd_channel_new                 (const PkdSpawnInfo  *spawn_info);
const gchar*  pkd_channel_get_target          (PkdChannel          *channel);
const gchar*  pkd_channel_get_working_dir     (PkdChannel          *channel);
gchar**       pkd_channel_get_args            (PkdChannel          *channel);
gchar**       pkd_channel_get_env             (PkdChannel          *channel);
GPid          pkd_channel_get_pid             (PkdChannel          *channel);
gboolean      pkd_channel_start               (PkdChannel          *channel,
                                               GError            **error);
gboolean      pkd_channel_stop                (PkdChannel          *channel,
                                               gboolean            killpid,
                                               GError            **error);
void          pkd_channel_pause               (PkdChannel          *channel);
void          pkd_channel_unpause             (PkdChannel          *channel);
PkdSource*    pkd_channel_add_source          (PkdChannel          *channel,
                                               PkdSourceInfo       *source_info);

G_END_DECLS

#endif /* __PKD_CHANNEL_H__ */
