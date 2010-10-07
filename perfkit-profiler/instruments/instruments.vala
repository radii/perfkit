/* instruments.vala
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
using Ppg;

namespace Ppg.Instruments {
	static bool initialized = false;
	static List<InstrumentFactory> _factories;

	void initialize () {
		if (!initialized) {
			initialized = true;
			_factories = new List<InstrumentFactory>();

			/* Initialize core instruments. */
			register(typeof(CpuInstrument), "perfkit-cpu", _("Cpu"));
			register(typeof(MemoryInstrument), "perfkit-memory", _("Memory"));
		}
	}

	public void register (Type type, string id, string title) {
		initialize();

		var factory = new InstrumentFactory();
		factory.type = type;
		factory.id = id;
		factory.title = title;
		_factories.prepend(factory);
	}

	public List<InstrumentFactory> get_factories () {
		initialize();

		var copy = _factories.copy();
		foreach (var inst in copy) {
			inst.ref();
		}
		return copy;
	}
}
