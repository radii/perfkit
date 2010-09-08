/* cairo-util.vala
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

namespace CairoUtil {
	public enum CornerType {
		TOP_LEFT       = 1 << 0,
		TOP_RIGHT      = 1 << 1,
		BOTTOM_RIGHT   = 1 << 2,
		BOTTOM_LEFT    = 1 << 3,
		NONE           = 0,
		TOP            = TOP_LEFT | TOP_RIGHT,
		BOTTOM         = BOTTOM_LEFT | BOTTOM_RIGHT,
		ALL            = TOP | BOTTOM,
	}

	public static void rounded_rectangle (Cairo.Context cr,
	                                      double x,
	                                      double y,
	                                      double width,
	                                      double height,
	                                      double x_radius,
	                                      double y_radius,
	                                      CornerType corners)
	{
		double x1, x2;
		double y1, y2;
		double xr1, xr2;
		double yr1, yr2;

		x1 = x;
		y1 = y;
		x2 = x1 + width;
		y2 = y1 + height;

		x_radius = double.min(x_radius, width / 2.0);
		y_radius = double.min(y_radius, width / 2.0);

		xr1 = x_radius;
		yr1 = y_radius;
		xr2 = x_radius / 2.0;
		yr2 = y_radius / 2.0;

		cr.move_to(x1 + xr1, y1);
		cr.line_to(x2 - xr1, y1);

		if ((corners & CornerType.TOP_RIGHT) != 0) {
			cr.curve_to(x2 - xr2, y1, x2, y1 + yr2, x2, y1 + yr1);
		} else {
			cr.line_to(x2, y1);
			cr.line_to(x2, y1 + yr1);
		}

		cr.line_to(x2, y2 - yr1);

		if ((corners & CornerType.BOTTOM_RIGHT) != 0) {
			cr.curve_to(x2, y2 - yr2, x2 - xr2, y2, x2 - xr1, y2);
		} else {
			cr.line_to(x2, y2);
			cr.line_to(x2 - xr1, y2);
		}

		cr.line_to(x1 + xr1, y2);

		if ((corners & CornerType.BOTTOM_LEFT) != 0) {
			cr.curve_to(x1 + xr2, y2, x1, y2 - yr2, x1, y2 - yr1);
		} else {
			cr.line_to(x1, y2);
			cr.line_to(x1, y2 - yr1);
		}

		cr.line_to(x1, y1 + yr1);

		if ((corners & CornerType.TOP_LEFT) != 0) {
			cr.curve_to(x1, y1 + yr2, x1 + xr2, y1, x1 + xr1, y1);
		} else {
			cr.line_to(x1, y1);
			cr.line_to(x1 + xr1, y1);
		}

		cr.close_path();
	}

	public static void add_color_stop (Cairo.Pattern pattern,
	                                   double offset,
	                                   Gdk.Color color)
	{
			pattern.add_color_stop_rgb(offset,
			                           color.red / 65535.0,
			                           color.green / 65535.0,
			                           color.blue / 65535.0);
	}
}
