/* ppg-sessions-store.c
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

#include <glib.h>
#include <glib/gi18n.h>

#include "ppg-sessions-store.h"

GtkListStore*
ppg_sessions_store_new (void)
{
	GtkListStore *store;
	GtkTreeIter iter;

	store = gtk_list_store_new(3,
	                           G_TYPE_STRING,
	                           G_TYPE_STRING,
	                           G_TYPE_BOOLEAN);

	gtk_list_store_append(store, &iter);
	gtk_list_store_set(store, &iter,
	                   PPG_SESSIONS_STORE_COLUMN_ICON_NAME, GTK_STOCK_HOME,
	                   PPG_SESSIONS_STORE_COLUMN_TITLE, _("Home"),
	                   PPG_SESSIONS_STORE_COLUMN_SEPARATOR, FALSE,
	                   -1);

	gtk_list_store_append(store, &iter);
	gtk_list_store_set(store, &iter,
	                   PPG_SESSIONS_STORE_COLUMN_SEPARATOR, TRUE,
	                   -1);

	return store;
}
