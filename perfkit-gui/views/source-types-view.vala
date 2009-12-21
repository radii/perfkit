/* source-types-view.vala
 *
 * Copyright (C) 2009 Christian Hergert <chris@dronelabs.com>
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

namespace PerfkitGui {
	public class SourceTypesView: Gtk.Window, View {
		ConnectionController _controller = null;
		ListStore _store = null;

		public SourceTypesView() {
			this.title = _("Sources");
			this.set_default_size(200, 400);
			this.set_type_hint(Gdk.WindowTypeHint.UTILITY);
			this._store = new ListStore(1, typeof (SourceInfo));

			var vbox = new Gtk.VBox(false, 0);
			this.add(vbox);
			vbox.show();

			var scroller = new Gtk.ScrolledWindow(null, null);
			scroller.set_policy(PolicyType.AUTOMATIC, PolicyType.AUTOMATIC);
			vbox.pack_start(scroller, true, true, 0);
			scroller.show();

			var treeview = new Gtk.TreeView();
			scroller.add(treeview);
			treeview.show();

			var hbox = new Gtk.HBox(false, 0);
			vbox.pack_start(hbox, false, true, 0);
			hbox.show();

			var refresh = new Gtk.Button();
			var image = new Gtk.Image.from_stock(STOCK_REFRESH, IconSize.MENU);
			refresh.add(image);
			refresh.set_relief(ReliefStyle.NONE);
			hbox.pack_start(refresh, false, true, 0);
			image.show();
			refresh.show();

			var column = new Gtk.TreeViewColumn();
			treeview.append_column(column);

			var cell = new Gtk.CellRendererText();
			column.pack_start(cell, true);
			column.add_attribute(cell, "text", 0);

			this.delete_event.connect(_ => {
				this.hide();
				return true;
			});
		}

		public void set_controller(Controller controller) {
			this._controller = (ConnectionController)controller;
		}
	}
}
