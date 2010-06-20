/* pka-listener.h
 *
 * Copyright 2010 Christian Hergert <chris@dronelabs.com>
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

#ifndef __PKA_LISTENER_H__
#define __PKA_LISTENER_H__

#include <gio/gio.h>

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

	gboolean (*listen)               (PkaListener  *listener,
	                                  GError      **error);
	void     (*close)                (PkaListener  *listener);

	void     (*plugin_added)         (PkaListener  *listener,
	                                  const gchar  *plugin);
	void     (*plugin_removed)       (PkaListener  *listener,
	                                  const gchar  *plugin);

	void     (*encoder_added)        (PkaListener  *listener,
	                                  gint          encoder);
	void     (*encoder_removed)      (PkaListener  *listener,
	                                  gint          encoder);

	void     (*source_added)         (PkaListener  *listener,
	                                  gint          source);
	void     (*source_removed)       (PkaListener  *listener,
	                                  gint          source);

	void     (*channel_added)        (PkaListener  *listener,
	                                  gint          channel);
	void     (*channel_removed)      (PkaListener  *listener,
	                                  gint          channel);

	void     (*subscription_added)   (PkaListener  *listener,
	                                  gint          subscription);
	void     (*subscription_removed) (PkaListener  *listener,
	                                  gint          subscription);
};

GType    pka_listener_get_type (void) G_GNUC_CONST;
gboolean pka_listener_listen   (PkaListener *listener, GError **error);
void     pka_listener_close    (PkaListener *listener);

void     pka_listener_plugin_added         (PkaListener *listener,
                                            const gchar *plugin);
void     pka_listener_plugin_removed       (PkaListener *listener,
                                            const gchar *plugin);
void     pka_listener_encoder_added        (PkaListener *listener,
                                            gint         encoder);
void     pka_listener_encoder_removed      (PkaListener *listener,
                                            gint         encoder);
void     pka_listener_source_added         (PkaListener *listener,
                                            gint         source);
void     pka_listener_source_removed       (PkaListener *listener,
                                            gint         source);
void     pka_listener_channel_added        (PkaListener *listener,
                                            gint         channel);
void     pka_listener_channel_removed      (PkaListener *listener,
                                            gint         channel);
void     pka_listener_subscription_added   (PkaListener *listener,
                                            gint         subscription);
void     pka_listener_subscription_removed (PkaListener *listener,
                                            gint         subscription);

G_END_DECLS

#endif /* __PKA_LISTENER_H__ */
