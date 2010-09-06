/* pause-action.vala
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
	public class PauseAction: ToggleAction {
		Widget _widget;
		Session _session;
		bool _block;

		construct {
			set("name", "PauseAction",
			    "label", _("Pause"),
			    "tooltip", _("Pause this profiling session"),
			    "icon-name", STOCK_MEDIA_PAUSE,
			    "sensitive", false,
			    null);
		}

		public Widget widget {
			set {
				if (value.get_type().is_a(typeof(Ppg.Window))) {
					_widget = value;
					_widget.get("session", &_session, null);
					_session.state_changed.connect(state_changed);
				}
			}
		}

		public override void toggled () {
			if (_session != null && !_block) {
				try {
					debug("Pause action toggled");
					if (_session.state == SessionState.PAUSED) {
						_session.unpause();
					} else {
						_session.pause();
					}
				} catch (Error err) {
					warning("%s", err.message);
				}
			}
		}

		void state_changed (SessionState state) {
			bool active;
			bool sensitive;

			switch (state) {
			case SessionState.STOPPED:
				active = false;
				sensitive = false;
				break;
			case SessionState.STARTED:
				active = false;
				sensitive = true;
				break;
			case SessionState.PAUSED:
				active = true;
				sensitive = true;
				break;
			default:
				assert_not_reached();
			}

			_block = true;
			set("active", active,
			    "sensitive", sensitive,
			    null);
			_block = false;
		}
	}
}
