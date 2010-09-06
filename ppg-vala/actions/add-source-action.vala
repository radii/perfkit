/* actions/add-source-action.vala
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

using GLib;
using Gtk;

namespace Ppg {
	public class AddSourceAction: Gtk.Action {
		Widget _widget;

		construct {
			set("name", "AddSourceAction",
			    "label", _("Add source"),
			    "icon-name", STOCK_ADD,
			    "tooltip", _("Add a source to this profiling session"),
			    null);
		}

		public Widget widget {
			set {
				if (value.get_type().is_a(typeof(Ppg.Window))) {
					_widget = value;
				}
			}
		}

		public override void activate () {
			if (_widget != null) {
				var dialog = new AddSourceDialog();
				dialog.transient_for = (Window)_widget;
				dialog.run();
				dialog.destroy();
			}
		}
	}
}
