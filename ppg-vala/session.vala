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

namespace Ppg {
	public enum SessionState {
		STOPPED = 1,
		STARTED = 2,
		PAUSED  = 3,
	}

	public class Session: GLib.Object {
		SessionState _state = SessionState.STOPPED;

		public signal void state_changed (SessionState state);

		public SessionState state {
			get { return _state; }
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
	}
}
