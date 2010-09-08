/* ruler.vala
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
	public class Ruler: Header {
		double _lower = 0.0;
		double _upper = 0.0;
		double _pos = 0.0;

		public double lower {
			get { return _lower; }
			set {
				_lower = value;
				this.queue_draw();
			}
		}

		public double upper {
			get { return _upper; }
			set {
				_upper = value;
				this.queue_draw();
			}
		}

		public double position {
			get { return _pos; }
			set {
				_pos = value;
				/*
				 * XXX: Optimize to just redraw two damage regions.
				 *      One for old position, one for new position.
				 */
				this.queue_draw();
			}
		}

		public void set_range(double lower,
		                      double upper,
		                      double position,
		                      double max_size)
		{
			this._lower = lower;
			this._upper = upper;
			this._pos = position;
			this.queue_draw();
		}

		public override bool expose_event (Gdk.EventExpose event) {
			Allocation alloc;

			base.expose_event(event);
			this.get_allocation(out alloc);

			var style = this.get_style();
			var color = new CairoUtil.Color.from_gdk(style.fg[StateType.NORMAL]);

			var cr = Gdk.cairo_create(event.window);
			cr.set_line_width(1.0);
			CairoUtil.set_source_color(cr, color);

			/*
			 * Draw the base line.
			 */
			cr.move_to(1, alloc.height - 1.5);
			cr.line_to(alloc.width - 2, alloc.height - 1.5);

			/*
			 * XXX: Draw some lines.
			 */
			 for (int i = 1; i < alloc.width; i += 20) {
			 	cr.move_to(i + 0.5, alloc.height - 1.5);
			 	cr.line_to(i + 0.5, 10);
			 }
			 for (int i = 11; i < alloc.width; i += 20) {
			 	cr.move_to(i + 0.5, alloc.height - 1.5);
			 	cr.line_to(i + 0.5, 15);
			 }

			/*
			 * Stroke the whole thing.
			 */
			cr.stroke();

			return false;
		}
	}
}
