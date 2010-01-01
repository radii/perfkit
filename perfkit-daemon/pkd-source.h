/* pkd-source.h
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

#ifndef __PKD_SOURCE_H__
#define __PKD_SOURCE_H__

#include <glib-object.h>

#include "pkd-manifest.h"
#include "pkd-sample.h"

G_BEGIN_DECLS

#define PKD_TYPE_SOURCE            (pkd_source_get_type())
#define PKD_SOURCE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), PKD_TYPE_SOURCE, PkdSource))
#define PKD_SOURCE_CONST(obj)      (G_TYPE_CHECK_INSTANCE_CAST ((obj), PKD_TYPE_SOURCE, PkdSource const))
#define PKD_SOURCE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  PKD_TYPE_SOURCE, PkdSourceClass))
#define PKD_IS_SOURCE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), PKD_TYPE_SOURCE))
#define PKD_IS_SOURCE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  PKD_TYPE_SOURCE))
#define PKD_SOURCE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  PKD_TYPE_SOURCE, PkdSourceClass))

typedef struct _PkdSource        PkdSource;
typedef struct _PkdSourceClass   PkdSourceClass;
typedef struct _PkdSourcePrivate PkdSourcePrivate;

struct _PkdSource
{
	GObject parent;

	/*< private >*/
	PkdSourcePrivate *priv;
};

struct _PkdSourceClass
{
	GObjectClass parent_class;

	gboolean (*conflicts) (PkdSource   *source,
	                       PkdSource   *other,
	                       GError     **error);

	void     (*notify_started)  (PkdSource *source);
	void     (*notify_stopped)  (PkdSource *source);
	void     (*notify_paused)   (PkdSource *source);
	void     (*notify_unpaused) (PkdSource *source);
};

GType     pkd_source_get_type         (void) G_GNUC_CONST;
guint     pkd_source_get_id           (PkdSource    *source);
gboolean  pkd_source_conflicts        (PkdSource    *source,
                                       PkdSource    *other,
                                       GError      **error);
void      pkd_source_deliver_sample   (PkdSource    *source,
                                       PkdSample    *sample);
void      pkd_source_deliver_manifest (PkdSource    *source,
                                       PkdManifest  *manifest);

G_END_DECLS

#endif /* __PKD_SOURCE_H__ */
