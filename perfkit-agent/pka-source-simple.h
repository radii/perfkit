/* pka-source-simple.h
 *
 * Copyright (C) 2010 Christian Hergert <chris@dronelabs.com>
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

#ifndef __PKA_SOURCE_SIMPLE_H__
#define __PKA_SOURCE_SIMPLE_H__

#include "pka-source.h"

G_BEGIN_DECLS

#define PKA_TYPE_SOURCE_SIMPLE            (pka_source_simple_get_type())
#define PKA_SOURCE_SIMPLE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), PKA_TYPE_SOURCE_SIMPLE, PkaSourceSimple))
#define PKA_SOURCE_SIMPLE_CONST(obj)      (G_TYPE_CHECK_INSTANCE_CAST ((obj), PKA_TYPE_SOURCE_SIMPLE, PkaSourceSimple const))
#define PKA_SOURCE_SIMPLE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  PKA_TYPE_SOURCE_SIMPLE, PkaSourceSimpleClass))
#define PKA_IS_SOURCE_SIMPLE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), PKA_TYPE_SOURCE_SIMPLE))
#define PKA_IS_SOURCE_SIMPLE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  PKA_TYPE_SOURCE_SIMPLE))
#define PKA_SOURCE_SIMPLE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  PKA_TYPE_SOURCE_SIMPLE, PkaSourceSimpleClass))

typedef struct _PkaSourceSimple        PkaSourceSimple;
typedef struct _PkaSourceSimpleClass   PkaSourceSimpleClass;
typedef struct _PkaSourceSimplePrivate PkaSourceSimplePrivate;
typedef void (*PkaSourceSimpleFunc) (PkaSourceSimple *source, gpointer user_data);
typedef void (*PkaSourceSimpleSpawn) (PkaSourceSimple *source, PkaSpawnInfo *spawn_info, gpointer user_data);

struct _PkaSourceSimple
{
	PkaSource parent;

	/*< private >*/
	PkaSourceSimplePrivate *priv;
};

struct _PkaSourceSimpleClass
{
	PkaSourceClass parent_class;
};

GType      pka_source_simple_get_type             (void) G_GNUC_CONST;
PkaSource* pka_source_simple_new                  (void);
PkaSource* pka_source_simple_new_full             (PkaSourceSimpleFunc   callback,
                                                   PkaSourceSimpleSpawn  spawn,
                                                   gpointer              user_data,
                                                   GDestroyNotify        notify);
void       pka_source_simple_set_sample_callback  (PkaSourceSimple     *source,
                                                   PkaSourceSimpleFunc  callback,
                                                   gpointer             user_data,
                                                   GDestroyNotify       notify);
void       pka_source_simple_set_sample_closure   (PkaSourceSimple *source,
                                                   GClosure        *closure);
void       pka_source_simple_set_spawn_callback   (PkaSourceSimple       *source,
                                                   PkaSourceSimpleSpawn   spawn,
                                                   gpointer               user_data,
                                                   GDestroyNotify         notify);
void       pka_source_simple_set_spawn_closure    (PkaSourceSimple       *source,
                                                   GClosure              *closure);
gboolean   pka_source_simple_get_use_thread       (PkaSourceSimple *source);
void       pka_source_simple_set_use_thread       (PkaSourceSimple *source,
                                                   gboolean         use_thread);
void       pka_source_simple_set_frequency        (PkaSourceSimple *source,
                                                   const GTimeVal  *frequency);

G_END_DECLS

#endif /* __PKA_SOURCE_SIMPLE_H__ */
