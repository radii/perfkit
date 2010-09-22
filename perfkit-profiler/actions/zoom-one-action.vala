/* zoom-one-action.vala
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
	public class ZoomOneAction: Action {
		construct {
			set("name", "ZoomOneAction",
			    "label", _("Normal Size"),
			    "tooltip", _("Show the image at its normal size"),
			    "icon-name", STOCK_ZOOM_100,
			    null);
		}

		public Widget widget { get; set; }

		public override void activate () {
			if (widget != null) {
				var window = (Ppg.Window)widget;
				if (window == null) {
					return;
				}

				window.zoom.value = 1.0;
			}
		}
	}
}
