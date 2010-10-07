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
			var accel_group = new Gtk.AccelGroup();
			if (widget.get_type().is_a(typeof(Gtk.Window))) {
				((Gtk.Window)widget).add_accel_group(accel_group);
			}
			foreach (var map in actions) {
				if (widget.get_type().is_a(map.widget_type)) {
					var action = (Action)GLib.Object.new(map.action_type, null);
					action.set_accel_path(create_accel_path(map.widget_type, map.action_type));
					action.set_accel_group(accel_group);
					action_group.add_action(action);
					var pspec = action.get_class().find_property("widget");
					if (pspec != null) {
						if ((pspec.flags & ParamFlags.WRITABLE) != 0) {
							if (pspec.value_type.is_a(typeof(Widget))) {
								action.set("widget", widget, null);
							}
						}
					}
					action.connect_accelerator();
				}
			}
		}

		static string create_accel_path (Type widget_type,
		                                 Type action_type)
		{
			return "<Perfkit>/%s/%s".printf(widget_type.name(),
			                                action_type.name());
		}

		public static void register (Type widget_type,
		                             Type action_type,
		                             string? accelerator) {
			var map = new ActionMap();
			map.widget_type = widget_type;
			map.action_type = action_type;
			actions.prepend(map);

			if (accelerator != null) {
				string accel_path;
				Gdk.ModifierType modifier;
				uint key;

				Gtk.accelerator_parse(accelerator, out key, out modifier);
				if (key != 0) {
					accel_path = create_accel_path(widget_type, action_type);
					Gtk.AccelMap.add_entry(accel_path, key, modifier);
				}
			}
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
			Actions.register(typeof(Ppg.Window), typeof(Ppg.QuitAction), null);
			Actions.register(typeof(Ppg.Window), typeof(Ppg.PerfkitAction), null);
			Actions.register(typeof(Ppg.Window), typeof(Ppg.ProfileAction), null);
			Actions.register(typeof(Ppg.Window), typeof(Ppg.StopAction), "<Control>E");
			Actions.register(typeof(Ppg.Window), typeof(Ppg.PauseAction), "<Control>Z");
			Actions.register(typeof(Ppg.Window), typeof(Ppg.RunAction), "<Control>B");
			Actions.register(typeof(Ppg.Window), typeof(Ppg.RestartAction), "<Control>R");
			Actions.register(typeof(Ppg.Window), typeof(Ppg.EditAction), null);
			Actions.register(typeof(Ppg.Window), typeof(Ppg.PreferencesAction), null);
			Actions.register(typeof(Ppg.Window), typeof(Ppg.ViewAction), null);
			Actions.register(typeof(Ppg.Window), typeof(Ppg.FullscreenAction), "F11");
			Actions.register(typeof(Ppg.Window), typeof(Ppg.ZoomInAction), "<Control>equal");
			Actions.register(typeof(Ppg.Window), typeof(Ppg.ZoomOutAction), "<Control>minus");
			Actions.register(typeof(Ppg.Window), typeof(Ppg.ZoomOneAction), "<Control>0");
			Actions.register(typeof(Ppg.Window), typeof(Ppg.InstrumentsAction), null);
			Actions.register(typeof(Ppg.Window), typeof(Ppg.AddInstrumentAction), "<Control>N");
		}
	}
}
