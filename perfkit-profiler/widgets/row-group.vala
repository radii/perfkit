/* row-group.vala
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
	public class RowGroup: Clutter.Group {
		GenericArray<Row>    _rows;
		Clutter.CairoTexture _header_bg;
		Clutter.Text         _header_text;
		Clutter.Rectangle    _data_bg;
		Clutter.Box          _data_box;
		Clutter.BoxLayout    _box_layout;
		Gtk.Style            _style;
		Gtk.StateType        _state;
		Instrument           _instrument;

		GenericArray<Visualizer> _visualizers;

		construct {
			var black = Clutter.Color.from_string("#000");

			reactive = true;
			_rows = new GenericArray<Row>();
			_visualizers = new GenericArray<Visualizer>();
			_state = StateType.NORMAL;

			_header_bg = new Clutter.CairoTexture(200, 45);
			_header_bg.set_size(200, 45);
			this.add_actor(_header_bg);

			_header_text = new Clutter.Text.full("Sans", "XXXXX", black);
			_header_text.ellipsize = Pango.EllipsizeMode.END;
			_header_text.width = 170.0f;
			_header_text.x = 15.0f;
			_header_text.y = (int)((45.0f - _header_text.height) / 2.0f);
			this.add_actor(_header_text);

			_data_bg = new Clutter.Rectangle();
			_data_bg.set_size(200, 45);
			this.add_actor(_data_bg);
			_data_bg.set_position(200.0f, 0.0f);

			_box_layout = new Clutter.BoxLayout() {
				easing_duration = 250,
				pack_start = true,
				spacing = 1,
				use_animations = true,
				vertical = true
			};
			_data_box = new Clutter.Box(_box_layout);
			_data_box.set_size(200, 45);
			this.add_actor(_data_box);
			_data_box.set_position(200.0f, 0.0f);

			this.notify["allocation"].connect(() => {
				GLib.Timeout.add(0, () => {
					_header_bg.height = this.height;
					_header_bg.set_surface_size((uint)_header_bg.width,
					                            (uint)_data_box.height);
					debug("Height %f", _data_box.height);
					paint_header();
					return false;
				});
			});
		}

		public string title {
			set {
				_header_text.text = Markup.printf_escaped("%s", value);
			}
		}

		public Instrument instrument {
			get { return _instrument; }
			set {
				assert(_instrument == null);
				assert(value != null);

				_instrument = value;
				title = _instrument.title;
				_instrument.style = style;

				foreach (var visual in _instrument.visualizers) {
					var actor = visual.actor;
					_visualizers.add(visual);
					assert(actor != null);
					actor.width = _data_box.width;
					_box_layout.pack(actor, true, true, true,
					                 BoxAlignment.START,
					                 BoxAlignment.START);
				}

				_data_box.height = float.max(45.0f, _visualizers.length * 30.0f + _visualizers.length);
				_data_bg.height = _data_box.height + 1.0f;
				_header_text.y = (_data_box.height - _header_text.height) / 2.0f;
			}
		}

		public Gtk.Style style {
			get { return _style; }
			set {
				_style = value;
				if (_instrument != null) {
					_instrument.style = style;
				}
				this.paint_header();
			}
		}

		public Gtk.StateType state {
			set {
				var do_paint = (value != _state);
				_state = value;
				if (do_paint) {
					this.paint_header();
				}
			}
		}

		public void update_width (Gtk.Widget embed) {
			Gtk.Allocation alloc;

			embed.get_allocation(out alloc);
			_data_bg.width = alloc.width - 200.0f;
			_data_box.width = alloc.width - 200.0f;
		}

		void paint_header () {
			if (style == null) {
				return;
			}

			var dark = Clutter.Color();
			dark.red = (uchar)(style.dark[_state].red / 256);
			dark.green = (uchar)(style.dark[_state].green / 256);
			dark.blue = (uchar)(style.dark[_state].blue / 256);
			dark.alpha = 0xFF;
			var dark_ = new CairoUtil.Color.from_clutter(dark);
			dark_.shade(0.8);

			var mid = Clutter.Color();
			mid.red = (uchar)(style.mid[_state].red / 256);
			mid.green = (uchar)(style.mid[_state].green / 256);
			mid.blue = (uchar)(style.mid[_state].blue / 256);
			mid.alpha = 0xFF;
			var mid_ = new CairoUtil.Color.from_clutter(mid);
			mid_.shade(0.9);

			var text = Clutter.Color();
			text.red = (uchar)(style.text[_state].red / 256);
			text.green = (uchar)(style.text[_state].green / 256);
			text.blue = (uchar)(style.text[_state].blue / 256);
			text.alpha = 0xFF;

			_header_text.color = text;
			_data_bg.color = mid;

			var cr = _header_bg.create();
			var p = new Cairo.Pattern.linear(0, 0, 0, this.height);
			p.add_color_stop_rgb(0.0, mid_.red, mid_.green, mid_.blue);
			p.add_color_stop_rgb(1.0, dark_.red, dark_.green, dark_.blue);
			cr.rectangle(0, 0, 200.0, this.height);
			cr.set_source(p);
			cr.fill();
		}

		public void add_row (Row row) {
			_box_layout.pack(row, true, true, true,
			                 Clutter.BoxAlignment.START,
			                 Clutter.BoxAlignment.START);
		}
	}
}
