/* restart-action.vala
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
	public class RestartAction: Action {
		Widget _widget;
		Session _session;

		construct {
			set("name", "RestartAction",
			    "label", _("Restart"),
			    "tooltip", _("Restart this profiling session"),
			    "icon-name", STOCK_REFRESH,
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

		public override void activate () {
			if (_session != null) {
				try {
					debug("Restart action activated");
					_session.stop();
					_session.start();
				} catch (Error err) {
					warning("%s", err.message);
				}
			}
		}

		void state_changed (SessionState state) {
			switch (state) {
			case SessionState.STOPPED:
				sensitive = false;
				break;
			case SessionState.STARTED:
			case SessionState.PAUSED:
				sensitive = true;
				break;
			default:
				assert_not_reached();
			}
		}
	}
}
