/* session.vala
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
	public enum SessionState {
		STOPPED = 1,
		STARTED = 2,
		PAUSED  = 3,
	}

	public class Session: GLib.Object {
		SessionState _state = SessionState.STOPPED;
		Perfkit.Connection _conn;
		int _channel = -1;

		public signal void state_changed (SessionState state);
		public signal void connected ();
		public signal void disconnected ();
		public signal void source_added (int source);
		public signal void source_removed (int source);

		public SessionState state {
			get { return _state; }
		}

		public Perfkit.Connection connection {
			get { return _conn; }
		}

		public int channel {
			get { return _channel; }
		}

		construct {
			_conn = new Perfkit.Connection.from_uri("dbus://");
			_conn.source_added.connect((_, source) => {
				GLib.Timeout.add(0, () => {
					debug("Source %d added.", source);
					this.source_added(source);
					return false;
				});
			});
			_conn.source_removed.connect((_, source) => {
				GLib.Timeout.add(0, () => {
					this.source_removed(source);
					return false;
				});
			});
			_conn.connect_async(null, (obj, res) => {
				try {
					_conn.connect_finish(res);
					_conn.manager_add_channel(out _channel);
					connected();
				} catch (Error err) {
					warning("Could not connect to perfkit agent: %s", err.message);
				}
			});
		}

		public void start () throws GLib.Error {
			_state = SessionState.STARTED;
			state_changed(_state);
		}

		public void stop () throws GLib.Error {
			_state = SessionState.STOPPED;
			state_changed(_state);
		}

		public void pause () throws GLib.Error {
			_state = SessionState.PAUSED;
			state_changed(_state);
		}

		public void unpause () throws GLib.Error {
			_state = SessionState.STARTED;
			state_changed(_state);
		}

		public void teardown () {
			if (_conn.is_connected() && _channel >= 0) {
				try {
					_conn.manager_remove_channel(_channel, null);
					_conn.disconnect();
				} catch (Error err) {
					warning("Failed to remove existing channel: %s", err.message);
				}
			}
		}

		public string get_source_name (int source) {
			try {
				string name;
				_conn.source_get_plugin(source, out name);
				return name;
			} catch (Error err) {
				warning("Error retrieving source name: %d", source);
				return "";
			}
		}

		public void remove_source (int source) {
			try {
				_conn.manager_remove_source(source);
			} catch (Error err) {
				warning("Error removing source: %d: %s", source, err.message);
			}
		}

		public void add_source_plugin (string type) {
			if (!_conn.is_connected()) {
				warning("Request to add plugin %s while not connected.", type);
				return;
			}

			try {
				int source;

				_conn.manager_add_source(type, out source);
				_conn.channel_add_source(this.channel, source);
			} catch (Error err) {
				warning("Error adding source: %s: %s", type, err.message);
			}

/*
			_conn.manager_add_source_async(type, null, (_, res) => {
				try {
					int source;

					_conn.manager_add_source_finish(res, out source);

					_conn.channel_add_source_async(channel, source, null, (_, res2) => {
						try {
							_conn.channel_add_source_finish(res2);
							message("Added source %d to channel %d", source, channel);
						} catch (Error err2) {
							warning("Failed to add source %d to channel %d: %s",
							        source, channel, err2.message);
						}
					});
				} catch (Error err) {
					warning("Failed to add source %s: %s", type, err.message);
				}
			});
*/
		}
	}
}
