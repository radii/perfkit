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

#ifndef __PKD_CHANNEL_H__
#define __PKD_CHANNEL_H__

#include <glib-object.h>

G_BEGIN_DECLS

#define PKD_TYPE_CHANNEL            (pkd_channel_get_type ())
#define PKD_CHANNEL(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), PKD_TYPE_CHANNEL, PkdChannel))
#define PKD_CHANNEL_CONST(obj)      (G_TYPE_CHECK_INSTANCE_CAST ((obj), PKD_TYPE_CHANNEL, PkdChannel const))
#define PKD_CHANNEL_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  PKD_TYPE_CHANNEL, PkdChannelClass))
#define PKD_IS_CHANNEL(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), PKD_TYPE_CHANNEL))
#define PKD_IS_CHANNEL_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  PKD_TYPE_CHANNEL))
#define PKD_CHANNEL_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  PKD_TYPE_CHANNEL, PkdChannelClass))
#define PKD_CHANNEL_ERROR           (pkd_channel_error_quark ())

/**
 * PkdChannelError:
 * @PKD_CHANNEL_ERROR_INVALID: The channel is in an invalid state.
 *
 * #PkdChannelError error enumeration.
 */
typedef enum
{
	PKD_CHANNEL_ERROR_INVALID,
} PkdChannelError;

/**
 * PkdChannelState:
 * @PKD_CHANNEL_READY:
 * @PKD_CHANNEL_STARTED:
 * @PKD_CHANNEL_PAUSED:
 * @PKD_CHANNEL_STOPPED:
 *
 * The "PkdChannelState" enumeration.
 */
typedef enum
{
	PKD_CHANNEL_READY,
	PKD_CHANNEL_STARTED,
	PKD_CHANNEL_PAUSED,
	PKD_CHANNEL_STOPPED,
} PkdChannelState;

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

GType           pkd_channel_get_type    (void) G_GNUC_CONST;
GQuark          pkd_channel_error_quark (void) G_GNUC_CONST;
gint            pkd_channel_get_id      (PkdChannel *channel);
gchar*          pkd_channel_get_dir     (PkdChannel *channel);
void            pkd_channel_set_dir     (PkdChannel *channel, const gchar  *dir);
gchar**         pkd_channel_get_args    (PkdChannel *channel);
void            pkd_channel_set_args    (PkdChannel *channel, const gchar **args);
GPid            pkd_channel_get_pid     (PkdChannel *channel);
void            pkd_channel_set_pid     (PkdChannel *channel, GPid pid);
gchar*          pkd_channel_get_target  (PkdChannel *channel);
void            pkd_channel_set_target  (PkdChannel *channel, const gchar  *target);
gchar**         pkd_channel_get_env     (PkdChannel *channel);
void            pkd_channel_set_env     (PkdChannel *channel, const gchar **env);
gboolean        pkd_channel_start       (PkdChannel *channel, GError      **error);
gboolean        pkd_channel_stop        (PkdChannel *channel, GError      **error);
gboolean        pkd_channel_pause       (PkdChannel *channel, GError      **error);
gboolean        pkd_channel_unpause     (PkdChannel *channel, GError      **error);
PkdChannelState pkd_channel_get_state   (PkdChannel *channel);

G_END_DECLS

#endif /* __PKD_CHANNEL_H__ */
