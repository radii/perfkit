/* ppg-instruments.c
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

#include <glib/gi18n.h>

#include <string.h>

#include "ppg-instrument.h"
#include "ppg-instruments.h"
#include "ppg-memory-instrument.h"

#define GET_ICON_NAMED(n) \
    gtk_icon_theme_load_icon(gtk_icon_theme_get_default(), (n), \
                             32, 0, NULL)

extern void ppg_session_add_instrument (PpgSession    *session,
                                        PpgInstrument *instrument);

typedef struct
{
	gchar     *name;
	gchar     *title;
	gchar     *icon_name;
	GType      type;
	GdkPixbuf *pixbuf;
} PpgInstrumentFactory;

static GArray    *factories      = NULL;
static GdkPixbuf *default_pixbuf = NULL;

void
ppg_instruments_init (void)
{
	factories = g_array_new(FALSE, FALSE, sizeof(PpgInstrumentFactory));
	default_pixbuf = GET_ICON_NAMED("perfkit-plugin");

	ppg_instruments_register("perfkit-memory",
	                         _("Memory"),
	                         "gnome-dev-memory",
	                         PPG_TYPE_MEMORY_INSTRUMENT);
	ppg_instruments_register("perfkit-cpu",
	                         _("CPU"),
	                         NULL,
	                         G_TYPE_NONE);
	ppg_instruments_register("perfkit-network",
	                         _("Network"),
	                         "network-wired",
	                         G_TYPE_NONE);
}

void
ppg_instruments_register (const gchar *name,
                          const gchar *title,
                          const gchar *icon_name,
                          GType        type)
{
	PpgInstrumentFactory factory;

	g_return_if_fail(factories != NULL);
	/*
	g_return_if_fail(g_type_is_a(type, PPG_TYPE_INSTRUMENT));
	*/

	memset(&factory, 0, sizeof(factory));
	factory.name = g_strdup(name);
	factory.title = g_strdup(title);
	factory.icon_name = g_strdup(icon_name);
	factory.type = type;
	factory.pixbuf = default_pixbuf;

	if (icon_name) {
		if (!(factory.pixbuf = GET_ICON_NAMED(icon_name))) {
			factory.pixbuf = default_pixbuf;
		}
	}

	g_array_append_val(factories, factory);
}

PpgInstrument*
ppg_instruments_create (PpgSession  *session,
                        const gchar *name)
{
	PpgInstrument *instrument = NULL;
	PpgInstrumentFactory *factory;
	gint i;

	g_return_val_if_fail(factories != NULL, NULL);

	for (i = 0; i < factories->len; i++) {
		factory = &g_array_index(factories, PpgInstrumentFactory, i);
		if (g_str_equal(name, factory->name)) {
			instrument = g_object_new(factory->type,
			                          "name", factory->title,
			                          "session", session,
			                          NULL);
			if (instrument) {
				ppg_session_add_instrument(session, instrument);
			}
			break;
		}
	}

	return instrument;
}

GtkListStore*
ppg_instruments_store_new (void)
{
	PpgInstrumentFactory *factory;
	GtkListStore *store;
	GtkTreeIter iter;
	gchar *fulltext;
	gchar *lowered;
	gint i;

	g_return_val_if_fail(factories != NULL, NULL);

	store = gtk_list_store_new(5,
	                           G_TYPE_STRING,
	                           G_TYPE_STRING,
	                           G_TYPE_STRING,
	                           GDK_TYPE_PIXBUF,
	                           G_TYPE_STRING);

	for (i = 0; i < factories->len; i++) {
		factory = &g_array_index(factories, PpgInstrumentFactory, i);
		fulltext = g_strdup_printf("%s %s", factory->name, factory->title);
		lowered = g_utf8_strdown(fulltext, -1);
		gtk_list_store_append(store, &iter);
		gtk_list_store_set(store, &iter,
		                   PPG_INSTRUMENTS_STORE_COLUMN_NAME, factory->name,
		                   PPG_INSTRUMENTS_STORE_COLUMN_TITLE, factory->title,
		                   PPG_INSTRUMENTS_STORE_COLUMN_ICON_NAME, factory->icon_name,
		                   PPG_INSTRUMENTS_STORE_COLUMN_PIXBUF, factory->pixbuf,
		                   PPG_INSTRUMENTS_STORE_COLUMN_FULLTEXT, lowered,
		                   -1);
		g_free(fulltext);
		g_free(lowered);
	}

	return store;
}
