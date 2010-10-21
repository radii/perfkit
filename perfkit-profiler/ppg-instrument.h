/* ppg-instrument.h
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

#ifndef __PPG_INSTRUMENT_H__
#define __PPG_INSTRUMENT_H__

#include "ppg-session.h"
#include "ppg-visualizer.h"

G_BEGIN_DECLS

#define PPG_TYPE_INSTRUMENT            (ppg_instrument_get_type())
#define PPG_INSTRUMENT(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), PPG_TYPE_INSTRUMENT, PpgInstrument))
#define PPG_INSTRUMENT_CONST(obj)      (G_TYPE_CHECK_INSTANCE_CAST ((obj), PPG_TYPE_INSTRUMENT, PpgInstrument const))
#define PPG_INSTRUMENT_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  PPG_TYPE_INSTRUMENT, PpgInstrumentClass))
#define PPG_IS_INSTRUMENT(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), PPG_TYPE_INSTRUMENT))
#define PPG_IS_INSTRUMENT_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  PPG_TYPE_INSTRUMENT))
#define PPG_INSTRUMENT_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  PPG_TYPE_INSTRUMENT, PpgInstrumentClass))

typedef struct _PpgInstrument        PpgInstrument;
typedef struct _PpgInstrumentClass   PpgInstrumentClass;
typedef struct _PpgInstrumentPrivate PpgInstrumentPrivate;

struct _PpgInstrument
{
	GInitiallyUnowned parent;

	/*< private >*/
	PpgInstrumentPrivate *priv;
};

struct _PpgInstrumentClass
{
	GInitiallyUnownedClass parent_class;

	gboolean (*load)               (PpgInstrument  *instrument,
	                                PpgSession     *session,
	                                GError        **error);
	gboolean (*unload)             (PpgInstrument  *instrument,
	                                PpgSession     *session,
	                                GError        **error);

	void     (*visualizer_added)   (PpgInstrument *instrument,
	                                PpgVisualizer *visualizer);
	void     (*visualizer_removed) (PpgInstrument *instrument,
	                                PpgVisualizer *visualizer);
};

GType  ppg_instrument_get_type                (void) G_GNUC_CONST;
void   ppg_instrument_register_visualizer     (PpgInstrument      *instrument,
                                               PpgVisualizerEntry *entry,
                                               gpointer            user_data);
void   ppg_instrument_register_visualizers    (PpgInstrument      *instrument,
                                               PpgVisualizerEntry *entries,
                                               guint               n_entries,
                                               gpointer            user_data);
GList* ppg_instrument_get_visualizer_entries  (PpgInstrument      *instrument);
GList* ppg_instrument_get_visualizers         (PpgInstrument      *instrument);
void   ppg_instrument_add_visualizer          (PpgInstrument      *instrument,
                                               const gchar        *name);
void   ppg_instrument_remove_visualizer       (PpgInstrument      *instrument,
                                               PpgVisualizer      *visualizer);
void   ppg_instrument_remove_visualizer_named (PpgInstrument      *instrument,
                                               const gchar        *name);

G_END_DECLS

#endif /* __PPG_INSTRUMENT_H__ */
