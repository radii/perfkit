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
		Pango.FontDescription font_desc;
		Button button;

		public uint hour { get; set; }
		public uint minute { get; set; }
		public uint second { get; set; }
		public uint ten_thousandth { get; set; }

		construct
		{
			font_desc = new Pango.FontDescription();
			font_desc.set_family("Monospace");
			font_desc.set_size(Pango.SCALE * 16);
			button = new Button();
			button.show();
		}

		public override bool expose_event (Gdk.EventExpose expose)
		{
			Gtk.Allocation alloc;

			get_allocation(out alloc);

			var cr = Gdk.cairo_create(expose.window);
			Gdk.cairo_rectangle(cr, expose.area);
			cr.clip();

			Gtk.paint_box(style,
			              expose.window,
			              StateType.NORMAL,
			              ShadowType.ETCHED_IN,
			              expose.area,
			              button,
			              "button",
			              0,
			              0,
			              alloc.width,
			              alloc.height);

			int width;
			int height;

			var layout = Pango.cairo_create_layout(cr);
			Gdk.cairo_set_source_color(cr, style.fg[StateType.NORMAL]);
			layout.set_font_description(font_desc);
			layout.set_text("%02u:%02u:%02u.%02u".printf(hour, minute, second, ten_thousandth / 100), -1);
			layout.get_pixel_size(out width, out height);
			cr.move_to((alloc.width - width) / 2, (alloc.height - height) / 2);
			Pango.cairo_show_layout(cr, layout);

			return false;
		}
	}
}
