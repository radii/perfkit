/* add-source-task.vala
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
	public class AddSourceTask: Ppg.Task {
		int _source = -1;

		public Connection connection { get; set; }
		public int channel { get; set; }
		public string plugin { get; set; }

		public int source {
			get { return _source; }
		}

		public override void run () {
			if (connection == null || channel == -1 || plugin == null) {
				finish_with_error();
				return;
			}
			connection.manager_add_source_async(plugin, cancellable,
			                                    manager_add_source_cb);
		}

		void manager_add_source_cb (GLib.Object? obj, AsyncResult res) {
			try {
				connection.manager_add_source_finish(res, out _source);
				connection.channel_add_source_async(channel, source,
				                                    cancellable,
				                                    channel_add_source_cb);
			} catch (GLib.Error err) {
				finish_with_error(err);
			}
		}

		void channel_add_source_cb (GLib.Object? obj, AsyncResult res) {
			try {
				connection.channel_add_source_finish(res);
				finish();
			} catch (GLib.Error err) {
				finish_with_error(err);
			}
		}
	}
}
