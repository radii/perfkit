/* window.vala
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
using GtkClutter;

namespace Ppg {
	static int window_count = 0;

	public class Window: Gtk.Window {
		Session _session = new Session();

		Embed     embed;
		Statusbar statusbar;
		MenuBar   menubar;
		Toolbar   toolbar;
		UIManager ui_manager;
		Ruler     ruler;

		Adjustment hadj;
		Adjustment vadj;
		Adjustment zadj;

		construct {
			window_count++;

			set("title", _("Perfkit Profiler"),
			    "default-width", 1000,
			    "default-height", 620,
			    "window-position", WindowPosition.CENTER,
			    null);

			hadj = new Adjustment(0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f);
			vadj = new Adjustment(0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f);
			zadj = new Adjustment(1.0f, 0.001f, 2.0f, 0.1f, 0.5f, 0.0f);

			var vbox = new VBox(false, 0);
			this.add(vbox);
			vbox.show();

			var table = new Table(4, 3, false);
			vbox.pack_start(table, true, true, 0);
			table.show();

			ruler = new HRuler();
			ruler.show();
			table.add_with_properties(ruler,
			                          "left-attach", 1,
			                          "right-attach", 2,
			                          "top-attach", 0,
			                          "bottom-attach", 1,
			                          "y-options", AttachOptions.FILL,
			                          null);

			var vscrollbar = new VScrollbar(vadj);
			vscrollbar.show();
			table.add_with_properties(vscrollbar,
			                          "left-attach", 2,
			                          "right-attach", 3,
			                          "top-attach", 0,
			                          "bottom-attach", 2,
			                          "x-options", AttachOptions.FILL,
			                          null);

			var halign = new Alignment(0.5f, 0.0f, 1.0f, 0.0f);
			halign.show();
			table.add_with_properties(halign,
			                          "left-attach", 1,
			                          "right-attach", 2,
			                          "top-attach", 2,
			                          "bottom-attach", 3,
			                          "y-options", AttachOptions.FILL,
			                          null);

			var hscrollbar = new HScrollbar(hadj);
			hscrollbar.show();
			halign.add(hscrollbar);

			embed = new Embed();
			embed.show();
			table.add_with_properties(embed,
			                          "left-attach", 0,
			                          "right-attach", 2,
			                          "top-attach", 1,
			                          "bottom-attach", 2,
			                          null);

			var hbox = new HBox(false, 3);
			hbox.width_request = 200;
			hbox.show();
			table.add_with_properties(hbox,
			                          "left-attach", 0,
			                          "right-attach", 1,
			                          "top-attach", 2,
			                          "bottom-attach", 4,
			                          "x-options", AttachOptions.FILL,
			                          "y-options", AttachOptions.FILL,
			                          null);

			var zout = new Gtk.Image.from_stock(STOCK_ZOOM_OUT, IconSize.MENU);
			zout.show();
			hbox.pack_start(zout, false, true, 6);

			var hscale = new HScale(zadj);
			hscale.draw_value = false;
			hscale.digits = 3;
			hscale.add_mark(1.0, PositionType.BOTTOM, null);
			hscale.show();
			hbox.pack_start(hscale, true, true, 0);

			var zin = new Gtk.Image.from_stock(STOCK_ZOOM_IN, IconSize.MENU);
			zin.show();
			hbox.pack_start(zin, false, true, 6);

			statusbar = new Statusbar();
			statusbar.push(0, "Ready");
			vbox.pack_start(statusbar, false, true, 0);
			statusbar.show();

			var action_group = new ActionGroup("PpgWindow");
			Actions.load(this, action_group);
			ui_manager = new UIManager();
			ui_manager.insert_action_group(action_group, 0);
			try {
				ui_manager.add_ui_from_string(ui_data, -1);
			} catch (Error err) {
				assert_not_reached();
			}
			menubar = (MenuBar)ui_manager.get_widget("/Menubar");
			toolbar = (Toolbar)ui_manager.get_widget("/Toolbar");

			vbox.add_with_properties(menubar,
			                         "expand", false,
			                         "position", 0,
			                         null);
			vbox.add_with_properties(toolbar,
			                         "expand", false,
			                         "position", 1,
			                         null);

			this.delete_event.connect((event) => {
				window_count--;
				if (window_count < 1) {
					Gtk.main_quit();
				}
				return false;
			});
		}

		public Session session {
			get { return _session; }
		}

		public static int count_windows () {
			return window_count;
		}
	}

	static const string ui_data =
"""
<ui>
 <menubar name="Menubar">
  <menu action="PerfkitAction">
   <menuitem action="QuitAction"/>
  </menu>
  <menu action="EditAction">
   <menuitem action="PreferencesAction"/>
  </menu>
  <menu action="ProfileAction">
   <menuitem action="AddSourceAction"/>
   <menuitem action="RemoveSourceAction"/>
   <separator/>
   <menuitem action="StopAction"/>
   <menuitem action="PauseAction"/>
   <menuitem action="RunAction"/>
   <menuitem action="RestartAction"/>
   <separator/>
  </menu>
 </menubar>
 <toolbar name="Toolbar">
  <toolitem action="StopAction"/>
  <toolitem action="PauseAction"/>
  <toolitem action="RunAction"/>
  <toolitem action="RestartAction"/>
  <separator/>
  <toolitem action="AddSourceAction"/>
 </toolbar>
</ui>
""";
}
