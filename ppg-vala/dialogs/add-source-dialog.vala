/* add-source-dialog.vala
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
	public class AddSourceDialog: Dialog {
		Button _add_button;
		Entry _search_entry;

		construct {
			this.title = _("Perfkit Profiler");
			this.default_width = 350;
			this.default_height = 400;
			this.has_separator = false;

			var vbox = new VBox(false, 0);
			vbox.border_width = 6;
			this.vbox.add(vbox);
			vbox.show();

			var l = new Label(_(title_msg));
			l.xpad = 12;
			l.ypad = 6;
			vbox.pack_start(l, false, true, 0);
			l.show();

			var hbox = new HBox(false, 6);
			hbox.border_width = 6;
			vbox.pack_start(hbox, false, true, 0);
			hbox.show();

			_search_entry = new Entry();
			hbox.pack_start(_search_entry, true, true, 0);
			_search_entry.show();

			l = new Label(_("_Search:"));
			l.mnemonic_widget = _search_entry;
			l.use_underline = true;
			hbox.add_with_properties(l, "expand", false, "position", 0, null);
			l.show();

			var scroller = new ScrolledWindow(null, null);
			scroller.set("hscrollbar-policy", PolicyType.AUTOMATIC,
			             "vscrollbar-policy", PolicyType.AUTOMATIC,
			             "shadow-type", ShadowType.IN,
			             null);
			vbox.pack_start(scroller, true, true, 0);
			scroller.show();

			var icon_view = new IconView();
			scroller.add(icon_view);
			icon_view.show();

			this.add_buttons(STOCK_CLOSE, ResponseType.CLOSE,
			                 STOCK_ADD, ResponseType.OK,
			                 null);
			this.set_default_response(ResponseType.OK);
			this._add_button = (Button)this.get_widget_for_response(ResponseType.OK);
			this._add_button.sensitive = false;
		}

		static const string title_msg = "Choose one or more sources to add to your session.";
	}
}
