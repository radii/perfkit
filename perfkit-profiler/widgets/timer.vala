/* timer.vala
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
	public class Timer: DrawingArea {
		public override bool expose_event (Gdk.EventExpose expose) {
			var cr = Gdk.cairo_create(expose.window);
			Gdk.cairo_rectangle(cr, expose.area);
			cr.clip();

			draw_background(cr);
			draw_foreground(cr);

			return false;
		}

		void draw_background (Cairo.Context cr) {
			Gtk.Allocation alloc;

			this.get_allocation(out alloc);

			CairoUtil.rounded_rectangle(cr, 0, 0, alloc.width, alloc.height,
			                            6, 6, CairoUtil.CornerType.ALL);

			var dark = style.dark[StateType.NORMAL];
			var light = style.light[StateType.NORMAL];
			var d = new CairoUtil.Color.from_gdk(dark);
			var c = new CairoUtil.Color.from_gdk(light);

			d.shade(0.95);
			c.shade(1.1);

			var p = new Cairo.Pattern.linear(0, 0, 0, alloc.height);
			CairoUtil.add_color_stop(p, -0.2f, c);
			CairoUtil.add_color_stop(p, 0.9f, d);
			CairoUtil.add_color_stop(p, 0.0f, c);
			cr.set_source(p);

			cr.fill_preserve();

			d.shade(0.9);
			CairoUtil.set_source_color(cr, d);
			cr.stroke();
		}

		void draw_foreground (Cairo.Context cr) {
		}
	}
}
