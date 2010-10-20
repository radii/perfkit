/* ppg-memory-instrument.h
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

#ifndef __PPG_MEMORY_INSTRUMENT_H__
#define __PPG_MEMORY_INSTRUMENT_H__

#include "ppg-instrument.h"

G_BEGIN_DECLS

#define PPG_TYPE_MEMORY_INSTRUMENT            (ppg_memory_instrument_get_type())
#define PPG_MEMORY_INSTRUMENT(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), PPG_TYPE_MEMORY_INSTRUMENT, PpgMemoryInstrument))
#define PPG_MEMORY_INSTRUMENT_CONST(obj)      (G_TYPE_CHECK_INSTANCE_CAST ((obj), PPG_TYPE_MEMORY_INSTRUMENT, PpgMemoryInstrument const))
#define PPG_MEMORY_INSTRUMENT_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  PPG_TYPE_MEMORY_INSTRUMENT, PpgMemoryInstrumentClass))
#define PPG_IS_MEMORY_INSTRUMENT(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), PPG_TYPE_MEMORY_INSTRUMENT))
#define PPG_IS_MEMORY_INSTRUMENT_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  PPG_TYPE_MEMORY_INSTRUMENT))
#define PPG_MEMORY_INSTRUMENT_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  PPG_TYPE_MEMORY_INSTRUMENT, PpgMemoryInstrumentClass))

typedef struct _PpgMemoryInstrument        PpgMemoryInstrument;
typedef struct _PpgMemoryInstrumentClass   PpgMemoryInstrumentClass;
typedef struct _PpgMemoryInstrumentPrivate PpgMemoryInstrumentPrivate;

struct _PpgMemoryInstrument
{
	PpgInstrument parent;

	/*< private >*/
	PpgMemoryInstrumentPrivate *priv;
};

struct _PpgMemoryInstrumentClass
{
	PpgInstrumentClass parent_class;
};

GType ppg_memory_instrument_get_type (void) G_GNUC_CONST;

G_END_DECLS

#endif /* __PPG_MEMORY_INSTRUMENT_H__ */
