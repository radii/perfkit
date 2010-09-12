/* remove-source-task.vala
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
using Gtk;
using Perfkit;

namespace Ppg {
	public class RemoveSourceTask: Task {
		public Connection connection { get; set; }
		public int source { get; set; }

		public override void run () {
			if (connection == null || source < 0) {
				finish_with_error();
				return;
			}

			connection.manager_remove_source_async(source, cancellable,
			                                       manager_remove_source_cb);
		}

		void manager_remove_source_cb (GLib.Object? obj, AsyncResult result) {
			try {
				connection.manager_remove_source_finish(result);
				finish();
			} catch (GLib.Error err) {
				finish_with_error(err);
			}
		}
	}
}
