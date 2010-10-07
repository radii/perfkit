/* line-visualizer.vala
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
	public class LineVisualizer: Visualizer {
		Clutter.CairoTexture _actor;
		Gdk.Color _color;
		bool _fill;

		public override Clutter.Actor actor {
			get { return _actor; }
		}

		public Gdk.Color color {
			get { return _color; }
			set {
				_color = value;
				paint_data();
			}
		}

		public string color_hex {
			set {
				Gdk.Color.parse(value, out _color);
			}
		}

		public bool fill {
			get { return _fill; }
			set {
				_fill = value;
				paint_data();
			}
		}

		construct {
			Gdk.Color.parse("#2e3436", out _color);
			_actor = new Clutter.CairoTexture(1, 1);
			_actor.notify["allocation"].connect(() => {
				GLib.Timeout.add(0, () => {
					_actor.set_surface_size((uint)_actor.width, (uint)_actor.height);
					paint_data();
					return false;
				});
			});
		}

		public void paint_data () {
			var cr = _actor.create();
			cr.set_line_width(1.0);

			cr.set_source_rgb(1, 1, 1);
			cr.rectangle(0, 0, _actor.width, _actor.height);
			cr.fill();

			var color = new CairoUtil.Color.from_gdk(_color);
			CairoUtil.set_source_color(cr, color);

			int i = 0;

			for (i = 0; i < _actor.width; i += 3) {
				cr.line_to(i, (int)Random.double_range(0.0, _actor.height));
			}

			if (_fill) {
				cr.line_to(i, _actor.height);
				cr.line_to(0, _actor.height);
				cr.close_path();
				cr.fill_preserve();
			}

			cr.stroke();
		}
	}
}
