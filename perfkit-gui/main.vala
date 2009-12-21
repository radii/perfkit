/* main.vala
 *
 * Copyright (C) 2009 Christian Hergert <chris@dronelabs.com>
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

using GLib;
using Gtk;
using Perfkit;

namespace PerfkitGui {
	static const OptionEntry[] entries = {
		{ "version", 'v', 0, OptionArg.NONE, ref show_version, N_("Show version"), null },
		{ null }
	};

	static bool show_version = false;

	static int main (string[] args) {
		/*
		 * Initialize I18N.
		 */

		GLib.Intl.textdomain(Config.GETTEXT_PACKAGE);
		GLib.Intl.bindtextdomain(Config.GETTEXT_PACKAGE, Config.LOCALE_DIR);
		GLib.Intl.bind_textdomain_codeset(Config.GETTEXT_PACKAGE, "UTF-8");
		GLib.Environment.set_application_name(_("Perfkit"));

		/*
		 * Parse command line arguments.
		 */

		var context = new OptionContext("");
		context.add_main_entries(entries, Config.GETTEXT_PACKAGE);
		context.add_group(Gtk.get_option_group(false));

		try {
		 	context.parse(ref args);
		}
		catch (Error e) {
		 	stderr.printf("%s\n", e.message);
		 	return 1;
		}

		/*
		 * Handle commands.
		 */

		if (show_version) {
			stdout.printf (_("Perfkit %s"), Config.VERSION);
			return 0;
		}

		/*
		 * Initialize libraries.
		 */
		Gtk.init(ref args);

		/*
		 * Initialize and execute runtime system.
		 */

		Runtime.load();
		Runtime.run();
		Runtime.unload();

		return 0;
	}
}
