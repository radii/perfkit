/* memory-instrument.vala
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
	public class MemoryInstrument: Ppg.Instrument {
		int _source_id;

		public override string title {
			get { return _("Memory"); }
		}

		public override string[] list_visualizers () {
			return new string[] { _("Virtual Memory"),
			                      _("Resident Memory"),
			                      _("Writable Memory") };
		}

		public override Widget? data_view {
			get { return null; }
		}

		public override bool is_supported (Ppg.Session session) throws GLib.Error {
			return session.has_plugin("Memory");
		}

		public override void load (Ppg.Session session) throws GLib.Error {
			_source_id = session.add_source_plugin("Memory");

			add_visualizer("combined");
			add_visualizer("system");
			add_visualizer("nice");
		}

		public override Visualizer? add_visualizer (string id) throws GLib.Error {
			Visualizer viz = null;

			if (id == "combined") {
				viz = new LineVisualizer() {
					color_hex = "#204a87",
					fill = true,
					title = _("Combined")
				};
			} else if (id == "system") {
				viz = new ColumnVisualizer() {
					title = _("System")
				};
			} else if (id == "nice") {
				viz = new LineVisualizer() {
					color_hex = "#204a87",
					title = _("Nice")
				};
			}

			if (viz != null) {
				visualizer_added(viz);
			}

			return viz;
		}
	}
}
