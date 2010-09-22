/* fullscreen-action.vala
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
	public class FullscreenAction: Gtk.ToggleAction {
		construct {
			set("name", "FullscreenAction",
			    "label", _("Fullscreen"),
			    "icon-name", STOCK_FULLSCREEN,
			    null);
		}

		public Widget widget { get; set; }

		public override void activate () {
			base.activate();
			this.label = active ? _("Unfullscreen") : _("Fullscreen");

			var window = (Ppg.Window)widget;
			if (window == null) {
				return;
			}

			if (active) {
				window.fullscreen();
				window.menubar.hide();
			} else {
				window.unfullscreen();
				window.menubar.show();
			}
		}
	}
}
