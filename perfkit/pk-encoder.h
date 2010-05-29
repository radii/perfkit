/* pk-encoder.h
 *
 * Copyright 2010 Christian Hergert <chris@dronelabs.com>
 *
 * This file is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 * 
 * This file is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __PK_ENCODER_H__
#define __PK_ENCODER_H__

#include "pk-connection.h"
#include "pk-plugin.h"

G_BEGIN_DECLS

#define PK_TYPE_ENCODER            (pk_encoder_get_type())
#define PK_ENCODER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), PK_TYPE_ENCODER, PkEncoder))
#define PK_ENCODER_CONST(obj)      (G_TYPE_CHECK_INSTANCE_CAST ((obj), PK_TYPE_ENCODER, PkEncoder const))
#define PK_ENCODER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  PK_TYPE_ENCODER, PkEncoderClass))
#define PK_IS_ENCODER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), PK_TYPE_ENCODER))
#define PK_IS_ENCODER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  PK_TYPE_ENCODER))
#define PK_ENCODER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  PK_TYPE_ENCODER, PkEncoderClass))

typedef struct _PkEncoder        PkEncoder;
typedef struct _PkEncoderClass   PkEncoderClass;
typedef struct _PkEncoderPrivate PkEncoderPrivate;

struct _PkEncoder
{
	GObject parent;

	/*< private >*/
	PkEncoderPrivate *priv;
};

struct _PkEncoderClass
{
	GObjectClass parent_class;
};

GType         pk_encoder_get_type                (void) G_GNUC_CONST;
gint          pk_encoder_get_id                  (PkEncoder             *encoder) G_GNUC_PURE;
PkConnection* pk_encoder_get_connection          (PkEncoder             *encoder) G_GNUC_PURE;
gboolean      pk_encoder_get_plugin              (PkEncoder             *encoder,
                                                  PkPlugin             **plugin,
                                                  GError               **error);
void          pk_encoder_get_plugin_async        (PkEncoder             *encoder,
                                                  GCancellable          *cancellable,
                                                  GAsyncReadyCallback    callback,
                                                  gpointer               user_data);
gboolean      pk_encoder_get_plugin_finish       (PkEncoder             *encoder,
                                                  GAsyncResult          *result,
                                                  PkPlugin             **plugin,
                                                  GError               **error);

G_END_DECLS

#endif /* __PK_ENCODER_H__ */
