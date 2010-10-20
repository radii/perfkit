/* ppg-sessions-store.h
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

#ifndef __PPG_SESSIONS_STORE_H__
#define __PPG_SESSIONS_STORE_H__

#include <gtk/gtk.h>

G_BEGIN_DECLS

enum _PpgSessionsStoreColumn
{
	PPG_SESSIONS_STORE_COLUMN_ICON_NAME,
	PPG_SESSIONS_STORE_COLUMN_TITLE,
	PPG_SESSIONS_STORE_COLUMN_SEPARATOR,
};

GtkListStore* ppg_sessions_store_new (void);

G_END_DECLS

#endif /* __PPG_SESSIONS_STORE_H__ */
