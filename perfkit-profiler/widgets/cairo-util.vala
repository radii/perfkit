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

	//
	// The following was ported from Hyena to Vala.
	//
	// Author:
	//   Aaron Bockover <abockover@novell.com>
	//
	// Copyright (C) 2007 Novell, Inc.
	//
	// Permission is hereby granted, free of charge, to any person obtaining
	// a copy of this software and associated documentation files (the
	// "Software"), to deal in the Software without restriction, including
	// without limitation the rights to use, copy, modify, merge, publish,
	// distribute, sublicense, and/or sell copies of the Software, and to
	// permit persons to whom the Software is furnished to do so, subject to
	// the following conditions:
	//
	// The above copyright notice and this permission notice shall be
	// included in all copies or substantial portions of the Software.
	//
	// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
	// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
	// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
	// NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
	// LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
	// OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
	// WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
	//

	public class Color {
		public double red;
		public double green;
		public double blue;
		public double alpha;

		public Color () {
			this.red = 0.0;
			this.green = 0.0;
			this.blue = 0.0;
			this.alpha = 1.0;
		}

		public Color.from_gdk (Gdk.Color color) {
			this.red = color.red / 65535.0;
			this.green = color.green / 65535.0;
			this.blue = color.blue / 65535.0;
			this.alpha = 1.0;
		}

		public void shade (double ratio) {
			double h;
			double s;
			double b;

			this.to_hsb(out h, out s, out b);
			b = double.max(double.min(b * ratio, 1), 0);
			s = double.max(double.min(s * ratio, 1), 0);
			this.from_hsb(h, s, b);
		}

		public void from_hsb (double hue,
		                      double saturation,
		                      double brightness) {
			int i;
            double [] hue_shift = { 0, 0, 0 };
            double [] color_shift = { 0, 0, 0 };
            double m1, m2, m3;

            m2 = brightness <= 0.5
                ? brightness * (1 + saturation)
                : brightness + saturation - brightness * saturation;

            m1 = 2 * brightness - m2;

            hue_shift[0] = hue + 120;
            hue_shift[1] = hue;
            hue_shift[2] = hue - 120;

            color_shift[0] = color_shift[1] = color_shift[2] = brightness;

            i = saturation == 0 ? 3 : 0;

            for(; i < 3; i++) {
                m3 = hue_shift[i];

                if(m3 > 360) {
                    m3 = Math.fmod(m3, 360);
                } else if(m3 < 0) {
                    m3 = 360 - Math.fmod(Math.fabs(m3), 360);
                }

                if(m3 < 60) {
                    color_shift[i] = m1 + (m2 - m1) * m3 / 60;
                } else if(m3 < 180) {
                    color_shift[i] = m2;
                } else if(m3 < 240) {
                    color_shift[i] = m1 + (m2 - m1) * (240 - m3) / 60;
                } else {
                    color_shift[i] = m1;
                }
            }

			this.red = color_shift[0];
			this.green = color_shift[1];
			this.blue = color_shift[2];
		}

		public void to_hsb (out double hue,
		                    out double saturation,
		                    out double brightness) {
			double min, max, delta;
            double red = this.red;
            double green = this.green;
            double blue = this.blue;

            hue = 0;
            saturation = 0;
            brightness = 0;

            if(red > green) {
                max = double.max(red, blue);
                min = double.min(green, blue);
            } else {
                max = double.max(green, blue);
                min = double.min(red, blue);
            }

            brightness = (max + min) / 2;

            if(Math.fabs(max - min) < 0.0001) {
                hue = 0;
                saturation = 0;
            } else {
                saturation = brightness <= 0.5
                    ? (max - min) / (max + min)
                    : (max - min) / (2 - max - min);

                delta = max - min;

                if(red == max) {
                    hue = (green - blue) / delta;
                } else if(green == max) {
                    hue = 2 + (blue - red) / delta;
                } else if(blue == max) {
                    hue = 4 + (red - green) / delta;
                }

                hue *= 60;
                if(hue < 0) {
                    hue += 360;
                }
            }
		}
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

	public static void set_source_color (Cairo.Context cr,
	                                     CairoUtil.Color color)
	{
		cr.set_source_rgba(color.red, color.green, color.blue, color.alpha);
	}

	public static void add_color_stop (Cairo.Pattern pattern,
	                                   double offset,
	                                   CairoUtil.Color color)
	{
			pattern.add_color_stop_rgb(offset, color.red, color.green,
			                           color.blue);
	}
}
