/* perfkit-shell.c
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <glib-object.h>
#include <egg-line.h>

static EggLineEntry* channel_iter (EggLine *line, const gchar *text, gchar **end);

static EggLineEntry entries[] =
{
	{ "channel", channel_iter, NULL, "Manage perfkit data channels" },
	{ "source", NULL, NULL, "Manage perfkit data sources" },
	{ "help", NULL, NULL, "Get help on a command" },
	{ NULL }
};

static EggLineEntry channel_entries[] =
{
	{ "show", NULL, NULL, "Show perfkit data channels" },
	{ "add", NULL, NULL, "Add a new perfkit data channel" },
	{ "remove", NULL, NULL, "Remove an existing perfkit data channel" },
	{ NULL }
};

gint
main (gint   argc,
      gchar *argv[])
{
	GOptionContext *context;
	GError         *error   = NULL;
	EggLine        *line;

	g_type_init ();
	
	line = egg_line_new ();
	egg_line_set_prompt (line, "perfkit> ");
	egg_line_set_entries (line, entries);
	egg_line_run (line);

	return EXIT_SUCCESS;
}

static EggLineEntry*
channel_iter (EggLine      *line,
              const gchar  *text,
              gchar       **end)
{
	return channel_entries;
}

