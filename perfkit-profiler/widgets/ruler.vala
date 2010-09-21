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
		static const int ARROW_WIDTH = 17;
		static const int ARROW_HEIGHT = 17;

		Gdk.Pixmap arrow;
		Gdk.Pixmap ruler;

		double _lower = 0.0;
		double _upper = 0.0;
		double _pos = 100.0;

		construct {
			this.add_events(Gdk.EventMask.POINTER_MOTION_MASK);
		}

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
				_pos = value.clamp(_lower, _upper);
				/*
				 * XXX: Optimize to just redraw two damage regions.
				 *      One for old position, one for new position.
				 */
				this.queue_draw();
			}
		}

		public bool contains (double value) {
			return (value >= _lower && value <= _upper);
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

		public override bool motion_notify_event (Gdk.EventMotion event) {
			Gtk.Allocation alloc;

			this.get_allocation(out alloc);
			position = _lower + (event.x / alloc.width * (_upper - _lower));
			return false;
		}

		public override bool expose_event (Gdk.EventExpose event) {
			Gtk.Allocation alloc;

			this.get_allocation(out alloc);

			base.expose_event(event);

			var cr = Gdk.cairo_create(event.window);

			cr.rectangle(0, 0, alloc.width, alloc.height);
			Gdk.cairo_set_source_pixmap(cr, this.ruler, 0, 0);
			cr.fill();

			int x = (int)(((_pos - lower) / (upper - lower) * alloc.width) - (ARROW_WIDTH / 2.0));
			int y = alloc.height - ARROW_HEIGHT;
			Gdk.cairo_set_source_pixmap(cr, arrow, x, y);
			cr.rectangle(x, y, ARROW_WIDTH, ARROW_HEIGHT);
			cr.fill();

			return false;
		}

		public override void realize () {
			base.realize();
			this.queue_resize();

			/*
			 * Build pixmap for arrow.
			 */
			this.arrow = new Gdk.Pixmap(null, ARROW_WIDTH, ARROW_HEIGHT, 32);
			var visual = Gdk.Visual.get_best_with_depth(32);
			var colormap = new Gdk.Colormap(visual, false);
			arrow.set_colormap(colormap);

			/*
			 * Render the arrow to the drawable.
			 */
			var cr = Gdk.cairo_create(arrow);
			draw_arrow(cr, 0, 0, ARROW_WIDTH, ARROW_HEIGHT);
		}

		public override void size_request (out Gtk.Requisition req) {
			base.size_request(out req);

			var window = this.get_window();
			if (window != null) {
				int width;
				int height;

				var layout = Pango.cairo_create_layout(Gdk.cairo_create(window));
				layout.set_text("00:00:00", -1);
				layout.get_pixel_size(out width, out height);
				height += 12;

				if (req.height < height) {
					req.height = height;
				}
			}
		}

		public override void size_allocate (Gdk.Rectangle alloc) {
			base.size_allocate(alloc);

			ruler = new Gdk.Pixmap(null, alloc.width, alloc.height, 32);
			var visual = Gdk.Visual.get_best_with_depth(32);
			var colormap = new Gdk.Colormap(visual, false);
			ruler.set_colormap(colormap);

			if (this.is_drawable()) {
				var cr = Gdk.cairo_create(ruler);
				draw_ruler(cr);
			}
		}

		public override void style_set (Gtk.Style? old_style) {
			base.style_set(old_style);

			if (this.is_drawable()) {
				var cr = Gdk.cairo_create(arrow);
				draw_arrow(cr, 0, 0, ARROW_WIDTH, ARROW_HEIGHT);

				cr = Gdk.cairo_create(ruler);
				draw_ruler(cr);
			}
		}

		void draw_ruler (Cairo.Context cr) {
			Allocation alloc;
			int text_height;

			this.get_allocation(out alloc);
			var style = this.get_style();

			cr.save();
			cr.set_operator(Cairo.Operator.CLEAR);
			cr.rectangle(0, 0, alloc.width, alloc.height);
			cr.fill();
			cr.restore();

			var color = new CairoUtil.Color.from_gdk(style.dark[StateType.NORMAL]);
			var lighter = color.copy();
			color.shade(0.5);
			lighter.shade(0.75);

			cr.set_line_width(1.0);
			CairoUtil.set_source_color(cr, color);

			/*
			 * Create layout for cairo text.
			 */
			var layout = Pango.cairo_create_layout(cr);
			layout.set_markup("<span size=\"smaller\">00:00:00</span>", -1);
			layout.get_pixel_size(null, out text_height);

			/*
			 * Draw the base line.
			 */
			cr.move_to(1, alloc.height - 0.5);
			cr.line_to(alloc.width - 2, alloc.height - 0.5);
			CairoUtil.set_source_color(cr, lighter);
			cr.stroke();

			CairoUtil.set_source_color(cr, color);

			/*
			 * XXX: Draw some lines.
			 */
			for (int i = 0; i < alloc.width; i += 20) {
				cr.move_to(i + 0.5, alloc.height - 1.5);
				cr.line_to(i + 0.5, text_height + 1);

				if ((i / 10) % 10 == 0) {
					cr.line_to(i + 0.5, 0);

					cr.move_to(i + 0.5, 0);
					Pango.cairo_show_layout(cr, layout);
				}
			 }
			 for (int i = 10; i < alloc.width; i += 20) {
			 	cr.move_to(i + 0.5, alloc.height - 1.5);
				cr.line_to(i + 0.5, text_height + 1 + 3);
			 }

			 cr.stroke();
		}

		void draw_arrow (Cairo.Context cr,
		                 double x,
		                 double y,
		                 int width,
		                 int height)
		{
			cr.save();
			cr.set_operator(Cairo.Operator.CLEAR);
			cr.rectangle(0, 0, ARROW_WIDTH, ARROW_HEIGHT);
			cr.fill();
			cr.restore();

			var size = int.min(width, height);
			var half = size / 2;
			var line_width = half / 6;

			var style = this.get_style();
			var base_light = new CairoUtil.Color.from_gdk(style.dark[StateType.SELECTED]);
			var base_dark = new CairoUtil.Color.from_gdk(style.dark[StateType.SELECTED]);

			//var str = "#8ae234";
			//var str = "#666666";
			//var base_light = new CairoUtil.Color.from_string(str);
			//var base_dark = new CairoUtil.Color.from_string(str);

			base_light.shade(1.1);
			base_dark.shade(0.95);
			var hl_light = base_light.copy();
			var hl_dark = base_dark.copy();
			hl_light.shade(1.1);
			hl_dark.shade(0.95);

			var center = x + (width / 2);
			var middle = y + (height / 2);
			var top = middle - half + line_width + 0.5;
			var bottom = middle + half - line_width - 0.5;
			var left = center - half + line_width + 0.5;
			var right = center + half - line_width - 0.5;

			cr.set_line_width(line_width);

			cr.move_to(left + line_width, top + line_width);
			cr.line_to(right + line_width, top + line_width);
			cr.line_to(right + line_width, middle + line_width);
			cr.line_to(center + line_width, bottom + line_width);
			cr.line_to(left + line_width, middle + line_width);
			cr.line_to(left + line_width, top + line_width);
			cr.close_path();
			cr.set_source_rgba(0, 0, 0, 0.5);
			cr.fill();

			cr.move_to(left, top);
			cr.line_to(center, top);
			cr.line_to(center, bottom);
			cr.line_to(left, middle);
			cr.line_to(left, top);
			cr.close_path();
			CairoUtil.set_source_color(cr, base_light);
			cr.fill();

			cr.move_to(center, top);
			cr.line_to(right, top);
			cr.line_to(right, middle);
			cr.line_to(center, bottom);
			cr.line_to(center, top);
			cr.close_path();

			CairoUtil.set_source_color(cr, base_dark);
			cr.fill();

			cr.move_to(left + line_width, top + line_width);
			cr.line_to(right - line_width, top + line_width);
			cr.line_to(right - line_width, middle);
			cr.line_to(center, bottom - line_width - 0.5);
			cr.line_to(left + line_width, middle);
			cr.line_to(left + line_width, top + line_width);
			cr.close_path();
			hl_light.alpha = 0.5;
			CairoUtil.set_source_color(cr, hl_light);
			cr.stroke();

			cr.move_to(left, top);
			cr.line_to(right, top);
			cr.line_to(right, middle);
			cr.line_to(center, bottom);
			cr.line_to(left, middle);
			cr.line_to(left, top);
			cr.close_path();

			base_dark.shade(0.5);
			CairoUtil.set_source_color(cr, base_dark);
			cr.stroke();
		}
	}
}
