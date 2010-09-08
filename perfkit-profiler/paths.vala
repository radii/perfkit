/* paths.vala
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

namespace Ppg {
	namespace Paths {
		static string data_dir = null;

		public string get_data_dir () {
			if (data_dir == null) {
				if (Environment.get_variable("PPG_DATA_DIR") != null) {
					data_dir = Environment.get_variable("PPG_DATA_DIR");
				} else {
					data_dir = Path.build_filename(Config.PACKAGE_DATA_DIR,
					                               "perfkit", null);
				}
			}
			return data_dir;
		}

		public string get_icon_dir () {
			return Path.build_filename(Ppg.Paths.get_data_dir(),
			                           "icons", null);
		}
	}
}
