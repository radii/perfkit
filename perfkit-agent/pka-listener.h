/* pka-listener.h
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

#ifndef __PKA_LISTENER_H__
#define __PKA_LISTENER_H__

#include <glib-object.h>

#include "pka-channel.h"
#include "pka-encoder-info.h"
#include "pka-source.h"
#include "pka-source-info.h"
#include "pka-subscription.h"

G_BEGIN_DECLS

#define PKA_TYPE_LISTENER            (pka_listener_get_type())
#define PKA_LISTENER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), PKA_TYPE_LISTENER, PkaListener))
#define PKA_LISTENER_CONST(obj)      (G_TYPE_CHECK_INSTANCE_CAST ((obj), PKA_TYPE_LISTENER, PkaListener const))
#define PKA_LISTENER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  PKA_TYPE_LISTENER, PkaListenerClass))
#define PKA_IS_LISTENER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), PKA_TYPE_LISTENER))
#define PKA_IS_LISTENER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  PKA_TYPE_LISTENER))
#define PKA_LISTENER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  PKA_TYPE_LISTENER, PkaListenerClass))

typedef struct _PkaListener        PkaListener;
typedef struct _PkaListenerClass   PkaListenerClass;
typedef struct _PkaListenerPrivate PkaListenerPrivate;

struct _PkaListener
{
	GObject parent;

	/*< private >*/
	PkaListenerPrivate *priv;
};

struct _PkaListenerClass
{
	GObjectClass parent_class;

	/*
	 * Listener methods.
	 */
	gboolean (*start) (PkaListener  *listener,
	                   GError      **error);
	void     (*stop)  (PkaListener  *listener);

	/*
	 * Pipeline Events.
	 */
	void (*channel_added)      (PkaListener     *listener,
	                            PkaChannel      *channel);
	void (*encoder_added)      (PkaListener     *listener,
	                            PkaEncoder      *encoder);
	void (*encoder_info_added) (PkaListener     *listener,
	                            PkaEncoderInfo  *encoder_info);
	void (*source_info_added)  (PkaListener     *listener,
	                            PkaSourceInfo   *source_info);
	void (*source_added)       (PkaListener     *listener,
	                            PkaSource       *source);
	void (*subscription_added) (PkaListener     *listener,
	                            PkaSubscription *subscription);

	gpointer reserved[32];
};

GType    pka_listener_get_type (void) G_GNUC_CONST;
gboolean pka_listener_start    (PkaListener  *listener,
                                GError      **error);
void     pka_listener_stop     (PkaListener  *listener);

G_END_DECLS

#endif /* __PKA_LISTENER_H__ */
