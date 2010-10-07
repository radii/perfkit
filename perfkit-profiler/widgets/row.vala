/* row.vala
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

using Clutter;
using GLib;
using Gtk;

namespace Ppg {
	static const float DEFAULT_HEIGHT = 45.0f;

	public class Row: Clutter.CairoTexture {
		Ppg.Visualizer _visualizer;
		bool           _show_title;
		bool           _has_focus;

		construct {
			this.reactive = true;
			this.height = DEFAULT_HEIGHT;
		}

		public Visualizer visualizer {
			get { return _visualizer; }
			set {
				_visualizer = value;
				queue_redraw();
			}
		}

		public bool has_focus {
			get { return _has_focus; }
			set {
				_has_focus = value;
				queue_redraw();
			}
		}

		public bool show_title {
			get { return _show_title; }
			set {
				_show_title = value;
				queue_redraw();
			}
		}

		public override void allocate (Clutter.ActorBox box,
		                               Clutter.AllocationFlags flags) {
			base.allocate(box, flags);
			queue_redraw();
		}

		public override bool button_press_event (Clutter.ButtonEvent button) {
			debug("Button press event in row");

			return base.button_press_event(button);
		}

		public override void paint () {
			base.paint();
		}
	}
}
