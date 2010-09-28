/* target-tool-button.vala
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
	public class TargetToolButton: Gtk.ToolItem {
		Alignment align;
		ToggleButton button;
		Label label;
		Menu menu;

		construct {
			align = new Alignment(1.0f, 0.5f, 0.0f, 0.0f);
			this.add(align);
			align.show();

			button = new ToggleButton();
			button.width_request = 175;
			button.toggled.connect(this.button_toggled);
			align.add(button);
			button.show();

			var hbox = new HBox(false, 3);
			hbox.border_width = 1;
			button.add(hbox);
			hbox.show();

			label = new Label(_("No target process"));
			label.xalign = 0.0f;
			label.ellipsize = Pango.EllipsizeMode.MIDDLE;
			hbox.pack_start(label, true, true, 3);
			label.show();

			var arrow = new Arrow(ArrowType.DOWN, ShadowType.NONE);
			hbox.pack_start(arrow, false, true, 0);
			arrow.show();

			menu = new Menu();
			menu.deactivate.connect(() => {
				button.active = false;
				((Gtk.Window)button.get_toplevel()).set_focus(null);
			});

			var menu_item = new MenuItem();
			menu_item.label = _("_Spawn a new process");
			menu_item.use_underline = true;
			menu.append(menu_item);
			menu_item.activate.connect(() => {
				this.run_spawn_dialog();
			});
			menu_item.show();

			menu_item = new MenuItem();
			menu_item.label = _("Attach to _existing process");
			menu_item.use_underline = true;
			menu.append(menu_item);
			menu_item.show();

			var sep_item = new SeparatorMenuItem();
			menu.append(sep_item);
			sep_item.show();

			/*
			 * TODO: Templates.
			 */

			/*
			 * TODO: Update existing process.
			 */
		}

		void button_toggled (Gtk.Widget widget) {
			if (button.active) {
				menu.popup(null, null, this.menu_position_func, 1,
				           Gtk.get_current_event_time());
			} else {
				menu.popdown();
			}
		}

		void menu_position_func (Gtk.Menu menu,
		                         out int x,
		                         out int y,
		                         out bool push_in) {
			Gtk.Requisition menu_req;
			Allocation alloc;
			int _x;
			int _y;

			menu.size_request(out menu_req);
			button.get_allocation(out alloc);
			button.window.get_origin(out _x, out _y);
			x = _x + alloc.x + alloc.width - menu_req.width;
			y = _y + alloc.y + alloc.height;
			push_in = true;
		}

		void run_spawn_dialog () {
			var dialog = new SpawnProcessDialog();
			var window = (Ppg.Window)this.get_toplevel();

			dialog.transient_for = window;
			dialog.session = window.session;

			if (dialog.run() == ResponseType.OK) {
				label.label = dialog.target + " " + string.joinv(" ", dialog.parse_args());
			}

			dialog.destroy();
		}
	}
}
