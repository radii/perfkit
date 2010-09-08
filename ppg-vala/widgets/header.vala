/* header.vala
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
	public class Header: Gtk.DrawingArea {
		/*
		 * XXX: Vala has some problems with enum properties, so you 
		 *      have to call queue_draw() yourself after changing these.
		 */
		public int xpad { get; set; default=0; }
		public int ypad { get; set; default=0; }
		public int radius { get; set; default=7; }
		public CairoUtil.CornerType corners { get; set; default=CairoUtil.CornerType.TOP; }

		public override bool expose_event (Gdk.EventExpose event) {
			Allocation alloc;

			var cr = Gdk.cairo_create(event.window);
			this.get_allocation(out alloc);

			/* clip to visible region */
			Gdk.cairo_rectangle(cr, event.area);
			cr.clip();

			/* Draw header background */
			CairoUtil.rounded_rectangle(cr, xpad, ypad,
			                            alloc.width - (2 * xpad),
			                            alloc.height - (2 * ypad),
			                            radius, radius, corners);
			cr.set_source(create_bg_pattern(alloc.width, alloc.height));
			cr.fill();

			/* stroke header border */
			cr.set_line_width(1.0);
			Gdk.cairo_set_source_color(cr, get_style().dark[StateType.NORMAL]);
			if (corners != CairoUtil.CornerType.NONE) {
				CairoUtil.rounded_rectangle(cr, xpad + 0.5, ypad + 0.5,
											alloc.width - (2 * xpad) - 1.0,
											alloc.height - (2 * ypad) - 1.0,
											radius, radius, corners);
				cr.stroke();
			} else {
				cr.move_to(xpad, ypad + 0.5);
				cr.line_to(alloc.width - (2 * xpad), ypad + 0.5);
				cr.stroke();
			}

			return false;
		}

		protected Cairo.Pattern create_bg_pattern (int width,
		                                           int height)
		{
			var style = this.get_style();
			var light = style.light[StateType.NORMAL];
			var dark = style.dark[StateType.NORMAL];
			var p = new Cairo.Pattern.linear(0, 0, 0, height);
			CairoUtil.add_color_stop(p, 0, light);
			CairoUtil.add_color_stop(p, 0.75, dark);
			CairoUtil.add_color_stop(p, 0, light);
			return p;
		}
	}
}

/*
static void main (string[] args) {
	Gtk.init(ref args);

	var window = new Window(WindowType.TOPLEVEL);
	window.set_default_size(640, 480);
	window.border_width = 12;
	window.show();

	var vbox = new VBox(false, 6);
	window.add(vbox);
	vbox.show();

	var area1 = new Ppg.Header() {
		corners = CairoUtil.CornerType.ALL
	};
	var area2 = new Ppg.Header() {
		corners = CairoUtil.CornerType.BOTTOM
	};
	var area3 = new Ppg.Header() {
		corners = CairoUtil.CornerType.TOP
	};
	var area4 = new Ppg.Header() {
		corners = CairoUtil.CornerType.TOP_LEFT
	};
	var area5 = new Ppg.Ruler() {
		corners = CairoUtil.CornerType.TOP_RIGHT,
		lower = 0.0,
		upper = 100.0
	};

	area1.show();
	area2.show();
	area3.show();
	area4.show();
	area5.show();

	vbox.add(area1);
	vbox.add(area2);
	vbox.add(area3);

	var hbox = new HBox(false, 0);
	vbox.add(hbox);
	hbox.show();

	area5.width_request = 200;
	hbox.add(area4);
	hbox.add(area5);

	window.delete_event.connect(() => {
		Gtk.main_quit();
		return false;
	});

	Gtk.main();
}
*/
