/* cpu-instrument.vala
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
	public class CpuInstrument: Ppg.Instrument {
		public override string title {
			get { return _("CPU"); }
		}

		public override Widget? data_view {
			get { return null; }
		}

		public override string[] list_visualizers () {
			return new string[] { _("System %"),
			                      _("Nice %"),
			                      _("Use %"),
			                      _("Idle %") };
		}
	}
}
