/* runtime.vala
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

using Gee;
using GLib;
using Gtk;
using Perfkit;

namespace PerfkitGui {
	namespace Runtime {

		/*
		 *---------------------------------------------------------------------
		 *
		 * PerfkitGui::Runtime::load --
		 *
		 *      Loads the runtime and prepares for execution.
		 *
		 * Results:
		 *      None.
		 *
		 * Side effects:
		 *      The subsystems are initialized.
		 *
		 *---------------------------------------------------------------------
		 */

		public static void load() {
			/*
			 * Initialize subsystems.
			 */

			Connections.init();
			Controller.init();
			View.init();
		}

		/*
		 *---------------------------------------------------------------------
		 *
		 * PerfkitGui::Runtime::unload --
		 *
		 *      Unload the runtime and cleans up after execution.
		 *
		 * Results:
		 *      None.
		 *
		 * Side effects:
		 *      Structures allocated during runtime are freed.
		 *
		 *---------------------------------------------------------------------
		 */

		public static void unload() {
		}

		/*
		 *---------------------------------------------------------------------
		 *
		 * PerfkitGui::Runtime::run --
		 *
		 *      Runs the applications main loop.  A connection to the local
		 *      perfkit daemon is created over DBus and the user interface
		 *      for the connection is shown.
		 *
		 * Results:
		 *      None.
		 *
		 * Side effects:
		 *      None.
		 *
		 *---------------------------------------------------------------------
		 */

		public static void run() {
			/* create connection to localhost */
			var conn = new Perfkit.Connection.from_uri("dbus://");
			assert(conn != null);
			Connections.add(conn);

			/*
			 * Get the controller and view then wire them up.
			 */
			var path = "/Connection/%s".printf(Connections.hash(conn));
			var controller = Controller.resolve(path);
			var view = View.resolve("/Connection");

			assert(controller != null);
			assert(view != null);

			view.set_controller(controller);
			view.show();

			/* block on the main loop */
			Gtk.main();
		}

        /*
		 *---------------------------------------------------------------------
		 *
		 * PerfkitGui::Runtime::quit --
		 *
		 *      Quits the application main loop.
		 *
		 * Results:
		 *      None.
		 *
		 * Side effects:
		 *      None.
		 *
		 *---------------------------------------------------------------------
		 */

		public static void quit() {
			Gtk.main_quit();
		}

		public static void show_connection(Perfkit.Connection conn) {
		}
	}
}
