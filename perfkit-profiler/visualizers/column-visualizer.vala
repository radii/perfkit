/* column-visualizer.vala
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
	public class ColumnVisualizer: Visualizer {
		Clutter.CairoTexture _actor;
		Gdk.Color _color;

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
				paint_data();
			}
		}

		construct {
			Gdk.Color.parse("#73d216", out _color);
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

			cr.rectangle(0, 0, _actor.width, _actor.height);
			cr.set_source_rgb(1, 1, 1);
			cr.fill();

			var span = 15.0f;
			var sep = 3.0f;
			float x;

			var fill = new CairoUtil.Color.from_gdk(_color);
			var stroke = new CairoUtil.Color.from_gdk(_color);
			stroke.shade(0.7);

			for (x = sep; x < _actor.width; x += span + sep) {
				var height = Random.double_range(0.0f, _actor.height - 1.0f);
				cr.rectangle(x + 0.5, _actor.height - 0.5, span, -height);

				CairoUtil.set_source_color(cr, fill);
				cr.fill_preserve();

				CairoUtil.set_source_color(cr, stroke);
				cr.stroke();
			}
		}
	}
}
