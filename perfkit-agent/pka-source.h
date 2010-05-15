/* pka-source.h
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

#ifndef __PKA_SOURCE_H__
#define __PKA_SOURCE_H__

#include <glib-object.h>

#include "pka-manifest.h"
#include "pka-sample.h"
#include "pka-spawn-info.h"

G_BEGIN_DECLS

#define PKA_TYPE_SOURCE            (pka_source_get_type())
#define PKA_SOURCE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), PKA_TYPE_SOURCE, PkaSource))
#define PKA_SOURCE_CONST(obj)      (G_TYPE_CHECK_INSTANCE_CAST ((obj), PKA_TYPE_SOURCE, PkaSource const))
#define PKA_SOURCE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  PKA_TYPE_SOURCE, PkaSourceClass))
#define PKA_IS_SOURCE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), PKA_TYPE_SOURCE))
#define PKA_IS_SOURCE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  PKA_TYPE_SOURCE))
#define PKA_SOURCE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  PKA_TYPE_SOURCE, PkaSourceClass))

typedef struct _PkaSource        PkaSource;
typedef struct _PkaSourceClass   PkaSourceClass;
typedef struct _PkaSourcePrivate PkaSourcePrivate;

struct _PkaSource
{
	GObject parent;

	/*< private >*/
	PkaSourcePrivate *priv;
};

struct _PkaSourceClass
{
	GObjectClass parent_class;

	gboolean (*conflicts)         (PkaSource     *source,
	                               PkaSource     *other,
	                               GError       **error);
	gboolean (*modify_spawn_info) (PkaSource     *source,
	                               PkaSpawnInfo  *spawn_info,
	                               GError       **error);
	void     (*notify_started)    (PkaSource     *source,
	                               PkaSpawnInfo  *spawn_info);
	void     (*notify_stopped)    (PkaSource     *source);
	void     (*notify_muted)      (PkaSource     *source);
	void     (*notify_unmuted)    (PkaSource     *source);

	gpointer reserved[16];
};

GType     pka_source_get_type          (void) G_GNUC_CONST;
guint     pka_source_get_id            (PkaSource      *source);
gboolean  pka_source_conflicts         (PkaSource      *source,
                                        PkaSource      *other,
                                        GError        **error);
void      pka_source_deliver_sample    (PkaSource      *source,
                                        PkaSample      *sample);
void      pka_source_deliver_manifest  (PkaSource      *source,
                                        PkaManifest    *manifest);
gboolean  pka_source_modify_spawn_info (PkaSource     *source,
                                        PkaSpawnInfo  *spawn_info,
                                        GError       **error);

G_END_DECLS

#endif /* __PKA_SOURCE_H__ */
