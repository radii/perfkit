/* edit-channel-task.vala
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
	public class EditChannelTask: Ppg.Task {
		public Session session { get; set; }

		int _pid;
		bool _pid_set;

		string _target;
		bool _target_set;

		string[] _env;
		bool _env_set;

		string[] _args;
		bool _args_set;

		int ops;
		int errcount;

		public int pid {
			get { return _pid; }
			set {
				_pid = value;
				_pid_set = true;
			}
		}

		public string target {
			get { return _target; }
			set {
				_target = value;
				_target_set = true;
			}
		}

		public string[] env {
			get { return _env; }
			set {
				_env = value;
				_env_set = true;
			}
		}

		public string[] args {
			get { return _args; }
			set {
				_args = value;
				_args_set = true;
			}
		}

		public override void run () {
			if (session == null) {
				finish_with_error();
				return;
			}

			var conn = session.connection;
			var channel = session.channel;

			if (conn == null || channel < 0) {
				finish_with_error();
				return;
			}

			if (_pid_set) ops++;
			if (_args_set) ops++;
			if (_env_set) ops++;
			if (_target_set) ops++;

			if (ops == 0) {
				finish();
				return;
			}

			if (_pid_set) sync_pid(conn, channel);
			if (_args_set) sync_args(conn, channel);
			if (_env_set) sync_env(conn, channel);
			if (_target_set) sync_target(conn, channel);
		}

		void sync_target (Perfkit.Connection conn,
		                  int channel) {
			conn.channel_set_target_async(channel, _target, cancellable, (o, res) => {
				try {
					var c = (Perfkit.Connection)o;
					c.channel_set_target_finish(res);
					handle_success();
				} catch (GLib.Error error) {
					handle_error(error);
				}
			});
		}

		void sync_args (Perfkit.Connection conn,
		                int channel) {
			conn.channel_set_args_async(channel, _args, cancellable, (o, res) => {
				try {
					var c = (Perfkit.Connection)o;
					c.channel_set_args_finish(res);
					handle_success();
				} catch (GLib.Error error) {
					handle_error(error);
				}
			});
		}

		void sync_env (Perfkit.Connection conn,
		               int channel) {
			conn.channel_set_env_async(channel, _env, cancellable, (o, res) => {
				try {
					var c = (Perfkit.Connection)o;
					c.channel_set_env_finish(res);
					handle_success();
				} catch (GLib.Error error) {
					handle_error(error);
				}
			});
		}

		void sync_pid (Perfkit.Connection conn,
		               int channel) {
			conn.channel_set_pid_async(channel, _pid, cancellable, (o, res) => {
				try {
					var c = (Perfkit.Connection)o;
					c.channel_set_pid_finish(res);
					handle_success();
				} catch (GLib.Error error) {
					handle_error(error);
				}
			});
		}

		void handle_success () {
			if (AtomicInt.dec_and_test(ref ops)) {
				if (errcount == 0) {
					finish();
				} else {
					finish_with_error();
				}
			}
		}

		void handle_error (GLib.Error error) {
			AtomicInt.inc(ref errcount);
			if (AtomicInt.dec_and_test(ref ops)) {
				debug("DOING FINISH ERROR");
				finish_with_error();
			}
		}
	}
}
