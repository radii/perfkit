/* get-channel-task.vala
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

namespace Ppg {
	public class GetChannelTask: Ppg.Task {
		public Ppg.Session session { get; set; }

		string _target;
		string[] _args;
		string[] _env;

		public string target {
			get { return _target; }
		}

		public string[] args {
			get { return _args; }
		}

		public string[] env {
			get { return _env; }
		}

		public override void run () {
			if (session == null ||
			    session.connection == null ||
			    session.channel < 0) {
				finish_with_error();
				return;
			}

			var conn = session.connection;
			var channel = session.channel;

			/*
			 * TODO: Once GObject introspection is fixed and vala to work
			 *       with it (issues with class virtual-methods), we should
			 *       convert these to be all asynchronous.
			 */

			try {
				conn.channel_get_target(channel, out _target);
				conn.channel_get_args(channel, out _args);
				conn.channel_get_env(channel, out _env);
				finish();
			} catch (GLib.Error error) {
				finish_with_error(error);
			}
		}
	}
}
