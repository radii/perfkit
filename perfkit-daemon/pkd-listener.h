/* pkd-listener.h
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

#ifndef __PKD_LISTENER_H__
#define __PKD_LISTENER_H__

#include <glib-object.h>

#include "pkd-channel.h"
#include "pkd-encoder-info.h"
#include "pkd-source.h"
#include "pkd-source-info.h"
#include "pkd-subscription.h"

G_BEGIN_DECLS

#define PKD_TYPE_LISTENER            (pkd_listener_get_type())
#define PKD_LISTENER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), PKD_TYPE_LISTENER, PkdListener))
#define PKD_LISTENER_CONST(obj)      (G_TYPE_CHECK_INSTANCE_CAST ((obj), PKD_TYPE_LISTENER, PkdListener const))
#define PKD_LISTENER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  PKD_TYPE_LISTENER, PkdListenerClass))
#define PKD_IS_LISTENER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), PKD_TYPE_LISTENER))
#define PKD_IS_LISTENER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  PKD_TYPE_LISTENER))
#define PKD_LISTENER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  PKD_TYPE_LISTENER, PkdListenerClass))

typedef struct _PkdListener        PkdListener;
typedef struct _PkdListenerClass   PkdListenerClass;
typedef struct _PkdListenerPrivate PkdListenerPrivate;

struct _PkdListener
{
	GObject parent;

	/*< private >*/
	PkdListenerPrivate *priv;
};

struct _PkdListenerClass
{
	GObjectClass parent_class;

	/*
	 * Listener methods.
	 */
	gboolean (*start) (PkdListener  *listener,
	                   GError      **error);
	void     (*stop)  (PkdListener  *listener);

	/*
	 * Pipeline Events.
	 */
	void (*channel_added)      (PkdListener     *listener,
	                            PkdChannel      *channel);
	void (*encoder_info_added) (PkdListener     *listener,
	                            PkdEncoderInfo  *encoder_info);
	void (*source_info_added)  (PkdListener     *listener,
	                            PkdSourceInfo   *source_info);
	void (*source_added)       (PkdListener     *listener,
	                            PkdSource       *source);
	void (*subscription_added) (PkdListener     *listener,
	                            PkdSubscription *subscription);
};

GType    pkd_listener_get_type (void) G_GNUC_CONST;
gboolean pkd_listener_start    (PkdListener  *listener,
                                GError      **error);
void     pkd_listener_stop     (PkdListener  *listener);

G_END_DECLS

#endif /* __PKD_LISTENER_H__ */
