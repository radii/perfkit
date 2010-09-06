/* actions.vala
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
	public class Actions {
		class ActionMap {
			public Type widget_type;
			public Type action_type;
		}

		static List<ActionMap> actions = new List<ActionMap>();

		public static void load (Widget widget,
		                         ActionGroup action_group) {
			foreach (var map in actions) {
				if (widget.get_type().is_a(map.widget_type)) {
					var action = (Action)GLib.Object.new(map.action_type, null);
					action_group.add_action(action);
					var pspec = action.get_class().find_property("widget");
					if (pspec != null) {
						if ((pspec.flags & ParamFlags.WRITABLE) != 0) {
							if (pspec.value_type.is_a(typeof(Widget))) {
								action.set("widget", widget, null);
							}
						}
					}
				}
			}
		}

		public static void register (Type widget_type,
		                             Type action_type) {
			var map = new ActionMap();
			map.widget_type = widget_type;
			map.action_type = action_type;
			actions.prepend(map);
		}

		public static void unregister (Type widget_type,
		                               Type action_type) {
			foreach (var map in actions) {
				if (map.widget_type == widget_type &&
				    map.action_type == action_type) {
					actions.remove(map);
					break;
				}
			}
		}

		public static void initialize () {
			Actions.register(typeof(Ppg.Window), typeof(Ppg.QuitAction));
			Actions.register(typeof(Ppg.Window), typeof(Ppg.PerfkitAction));
			Actions.register(typeof(Ppg.Window), typeof(Ppg.ProfileAction));
			Actions.register(typeof(Ppg.Window), typeof(Ppg.StopAction));
			Actions.register(typeof(Ppg.Window), typeof(Ppg.PauseAction));
			Actions.register(typeof(Ppg.Window), typeof(Ppg.RunAction));
			Actions.register(typeof(Ppg.Window), typeof(Ppg.RestartAction));
			Actions.register(typeof(Ppg.Window), typeof(Ppg.AddSourceAction));
			Actions.register(typeof(Ppg.Window), typeof(Ppg.EditAction));
			Actions.register(typeof(Ppg.Window), typeof(Ppg.PreferencesAction));
		}
	}
}
