/* ppg-instruments.h
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

#ifndef __PPG_INSTRUMENTS_H__
#define __PPG_INSTRUMENTS_H__

#include <gtk/gtk.h>

#include "ppg-instrument.h"
#include "ppg-session.h"

G_BEGIN_DECLS

typedef enum
{
	PPG_INSTRUMENTS_STORE_COLUMN_NAME,
	PPG_INSTRUMENTS_STORE_COLUMN_TITLE,
	PPG_INSTRUMENTS_STORE_COLUMN_ICON_NAME,
	PPG_INSTRUMENTS_STORE_COLUMN_PIXBUF,
	PPG_INSTRUMENTS_STORE_COLUMN_FULLTEXT,
} PpgInstrumentsStoreColumn;

void           ppg_instruments_init      (void);
void           ppg_instruments_register  (const gchar *name,
                                          const gchar *title,
                                          const gchar *icon_name,
                                          GType        type);
GtkListStore*  ppg_instruments_store_new (void);
PpgInstrument* ppg_instruments_create    (PpgSession  *session,
                                          const gchar *name);

G_END_DECLS

#endif /* __PPG_INSTRUMENTS_H__ */
