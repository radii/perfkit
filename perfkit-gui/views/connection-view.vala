/* connection-view.vala
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

namespace PerfkitGui {
	public class ConnectionView: Gtk.Window, View {
		ConnectionController _controller;
		Gtk.Builder _builder;

		public ConnectionView() {
			this.title = _("Perfkit");
			this.set_default_size(640, 480);

			/*
			 * Load the UI from GtkBuilder.
			 */
			this._builder = View.attach_ui("connection-view", "view-toplevel", this);

			/*
			 * Connect visibility notify so we know when our window is
			 * shown.  When that happens, we tell the sourcetypes view
			 * to show source types for our connection.
			 */
			this.visibility_notify_event.connect((event) => {
				var vis = (Gdk.EventVisibility*)event;

				if (vis->state == Gdk.VisibilityState.UNOBSCURED) {
					var view = View.resolve("/SourceTypes");
					assert(view != null);
					view.set_controller(this._controller);
					view.show();
				}
			});
		}

		public void set_controller(Controller controller) {
			this._controller = (ConnectionController)controller;
			assert(this._controller != null);
		}
	}
}
