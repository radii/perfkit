/* pkg-cmds.c
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

#include "pkg-cmds.h"
#include "pkg-panels.h"
#include "pkg-prefs.h"
#include "pkg-runtime.h"
#include "pkg-version.h"

static const gchar* program = "Perfkit";
static const gchar* authors[] = {
	"Andrew Stiegmann",
	"Andy Isaacson",
	"Christian Hergert",
	"Shapor Naghibzadeh",
	NULL,
};
static const gchar* website = "http://perfkit.org";
static const gchar* website_label = "Perfkit Website";
static const gchar* copyright =
	"Copyright © 2009-2010 Christian Hergert\n"
	"Copyright © 2010 Others";
static const gchar* license =
	"This program is free software: you can redistribute it and/or modify\n"
	"it under the terms of the GNU General Public License as published by\n"
	"the Free Software Foundation, either version 3 of the License, or\n"
	"(at your option) any later version.\n"
	"\n"
	"This program is distributed in the hope that it will be useful,\n"
	"but WITHOUT ANY WARRANTY; without even the implied warranty of\n"
	"MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n"
	"GNU General Public License for more details.\n"
	"\n"
	"You should have received a copy of the GNU General Public License\n"
	"along with this program.  If not, see <http://www.gnu.org/licenses/>.";

void
pkg_cmd_quit (PkgCommand *command)
{
	pkg_runtime_quit();
}

void
pkg_cmd_close (PkgCommand *command)
{
	pkg_window_close(command->window);
}

void
pkg_cmd_show_prefs (PkgCommand *command)
{
	pkg_prefs_show();
}

void
pkg_cmd_show_sources (PkgCommand *command)
{
	pkg_panels_show_sources();
}

void
pkg_cmd_show_about (PkgCommand *command)
{
	GtkWidget *about;

	/*
	 * Create and setup the about dialog.
	 */
	about = gtk_about_dialog_new();
	gtk_window_set_transient_for(GTK_WINDOW(about),
	                             GTK_WINDOW(command->window));
	gtk_window_set_modal(GTK_WINDOW(about), TRUE);
	g_object_set(about,
	             "authors", authors,
	             "copyright", copyright,
	             "program-name", program,
	             "license", license,
	             "version", PKG_VERSION_S,
	             "wrap-license", FALSE,
	             "website", website,
	             "website-label", website_label,
	             NULL);

	/*
	 * Show and run the dialog main loop.
	 */
	gtk_widget_show(about);
	gtk_dialog_run(GTK_DIALOG(about));
	gtk_widget_destroy(about);
}
