/* ppg-row.h
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

#ifndef __PPG_ROW_H__
#define __PPG_ROW_H__

#include <clutter/clutter.h>

#include "ppg-instrument.h"

G_BEGIN_DECLS

#define PPG_TYPE_ROW            (ppg_row_get_type())
#define PPG_ROW(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), PPG_TYPE_ROW, PpgRow))
#define PPG_ROW_CONST(obj)      (G_TYPE_CHECK_INSTANCE_CAST ((obj), PPG_TYPE_ROW, PpgRow const))
#define PPG_ROW_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  PPG_TYPE_ROW, PpgRowClass))
#define PPG_IS_ROW(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), PPG_TYPE_ROW))
#define PPG_IS_ROW_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  PPG_TYPE_ROW))
#define PPG_ROW_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  PPG_TYPE_ROW, PpgRowClass))

typedef struct _PpgRow        PpgRow;
typedef struct _PpgRowClass   PpgRowClass;
typedef struct _PpgRowPrivate PpgRowPrivate;

struct _PpgRow
{
	ClutterGroup parent;

	/*< private >*/
	PpgRowPrivate *priv;
};

struct _PpgRowClass
{
	ClutterGroupClass parent_class;
};

GType          ppg_row_get_type       (void) G_GNUC_CONST;
PpgInstrument* ppg_row_get_instrument (PpgRow *row);

G_END_DECLS

#endif /* __PPG_ROW_H__ */
