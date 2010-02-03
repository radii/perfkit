/* connections.vala
 *
 * Copyright (C) 2009 Christian Hergert <chris@dronelabs.com>
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

using Gee;
using GLib;
using Perfkit;

namespace PerfkitGui {
	namespace Connections {
		static Gee.ArrayList<Perfkit.Connection> _conns;

		public static void init() {
			_conns = new Gee.ArrayList<Perfkit.Connection>();
		}

		public static void add(Perfkit.Connection conn) {
			if (!_conns.contains(conn)) {
				_conns.add(conn);
			}
		}

		public static Connection? lookup(string hash) {
			foreach (var conn in _conns) {
				var cur_hash = Connections.hash(conn);
				if (cur_hash == hash) {
					return conn;
				}
			}
			return null;
		}

		public static Gee.List<Perfkit.Connection>? all() {
			//return _conns.read_only_view;
			return null;
		}

		public string hash(Connection conn) {
			return string.joinv("_", conn.uri.split("/"));
		}
	}
}
