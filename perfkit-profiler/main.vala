/* main.vala
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

static int main (string[] args) {
	GtkClutter.init(ref args);
	Ppg.Actions.initialize();

	var theme = Gtk.IconTheme.get_default();
	theme.append_search_path(Ppg.Paths.get_icon_dir());

	Gtk.Window.set_default_icon_name("clock");

	var welcome = new Ppg.Welcome();
	welcome.delete_event.connect((event) => {
		if (Ppg.Window.count_windows() < 1) {
			Gtk.main_quit();
		}
		return false;
	});
	welcome.show();

	Gtk.main();
	return 0;
}
