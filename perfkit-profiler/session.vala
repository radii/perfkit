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
		INVALID = 0,
		STOPPED = 1,
		STARTED = 2,
		PAUSED  = 3,
	}

	public errordomain SessionError {
		NOT_CONNECTED,
	}

	public class Session: GLib.Object {
		GLib.Timer _timer;
		SessionState _state = SessionState.STOPPED;
		Perfkit.Connection _conn;
		int _channel = -1;

		public signal void state_changed (SessionState state);
		public signal void connected ();
		public signal void disconnected ();
		public signal void source_added (int source);
		public signal void source_removed (int source);
		public signal void instrument_added (Instrument instrument);
		public signal void instrument_removed (Instrument instrument);

		public GLib.Timer timer {
			get { return _timer; }
		}

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

		/*
		 * TODO: This should simply request the call and a signal notify
		 *       us that the operation has completed and we should update
		 *       the UI.  That way, the gui doesn't get inconsistent.
		 */
		public void start () throws GLib.Error {
			if (!connection.is_connected()) {
				throw new SessionError.NOT_CONNECTED("Session not connected");
			}

			connection.channel_start_async(channel, null, (_, res) => {
				try {
					connection.channel_start_finish(res);
					_timer = new GLib.Timer();
					_state = SessionState.STARTED;
					state_changed(_state);
				} catch (GLib.Error error) {
					warning("Could not start session: %s", error.message);
				}
			});
		}

		/*
		 * TODO: Same as above.
		 */
		public void stop () throws GLib.Error {
			if (!connection.is_connected()) {
				throw new SessionError.NOT_CONNECTED("Session not connected");
			}

			connection.channel_stop_async(channel, null, (_, res) => {
				try {
					connection.channel_stop_finish(res);
					_timer.stop();
					_state = SessionState.STOPPED;
					state_changed(_state);
				} catch (GLib.Error error) {
					warning("Could not stop session: %s", error.message);
				}
			});
		}

		/* TODO: Same as above. */
		public void pause () throws GLib.Error {
			if (!connection.is_connected()) {
				throw new SessionError.NOT_CONNECTED("Session not connected");
			}

			connection.channel_mute_async(channel, null, (_, res) => {
				try {
					connection.channel_mute_finish(res);
					_state = SessionState.PAUSED;
					state_changed(_state);
				} catch (GLib.Error error) {
					warning("Could not pause session: %s", error.message);
				}
			});
		}

		/* TODO: Same as above. */
		public void unpause () throws GLib.Error {
			if (!connection.is_connected()) {
				throw new SessionError.NOT_CONNECTED("Session not connected");
			}

			connection.channel_unmute_async(channel, null, (_, res) => {
				try {
					connection.channel_unmute_finish(res);
					_state = SessionState.STARTED;
					state_changed(_state);
				} catch (GLib.Error error) {
					warning("Could not pause session: %s", error.message);
				}
			});
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

		public bool has_plugin (string id) {
			try {
				/* do a simple rpc to test id */
				string name;
				_conn.plugin_get_name(id, out name);
				return true;
			} catch (GLib.Error error) {
				return false;
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

		public int add_source_plugin (string type) throws GLib.Error {
			if (!_conn.is_connected()) {
				throw new SessionError.NOT_CONNECTED("Request to add plugin %s while not connected.", type);
			}

			int source;

			_conn.manager_add_source(type, out source);
			_conn.channel_add_source(this.channel, source);
			return source;
		}

		public void add_instrument (InstrumentFactory factory) throws GLib.Error {
			var instrument = factory.create();
			if (instrument.is_supported(this)) {
				message("Adding instrument %s", instrument.title);
				instrument.load(this);
				instrument_added(instrument);
			}
		}
	}
}
