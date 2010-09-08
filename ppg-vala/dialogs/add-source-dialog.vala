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
		Ppg.Session _session;

		Button _add_button;
		Entry _search_entry;
		ListStore model;
		TreeModelFilter filter;
		IconView icon_view;

		construct {
			this.title = _("Perfkit Profiler");
			this.border_width = 6;
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
			_search_entry.activates_default = true;
			hbox.pack_start(_search_entry, true, true, 0);
			_search_entry.changed.connect(() => {
				Gtk.TreeIter iter;
				string text = _search_entry.text.down();
				int matched = 0;

				if (model.get_iter_first(out iter)) {
					do {
						string id;

						model.get(iter, 1, out id, -1);
						if (id.contains(text)) {
							matched++;
							model.set(iter, 4, true, -1);
						} else {
							model.set(iter, 4, false, -1);
						}
					} while (model.iter_next(ref iter));
				}

				if (matched == 1) {
					/* Select the first row */
					filter.get_iter_first(out iter);
					var path = filter.get_path(iter);
					icon_view.select_path(path);
				}
			});
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

			icon_view = new IconView();
			scroller.add(icon_view);
			icon_view.show();

			icon_view.item_activated.connect(() => {
				add_selected();
			});

			model = new ListStore(5,
			                      typeof(string),     // Id
			                      typeof(string),     // Search text (lowercase)
			                      typeof(string),     // Title
			                      typeof(Gdk.Pixbuf), // Icon
			                      typeof(bool));      // Visible
			filter = new TreeModelFilter(model, null);
			filter.set_visible_column(4);
			icon_view.model = filter;
			icon_view.set_text_column(0);
			icon_view.set_pixbuf_column(3);
			icon_view.selection_changed.connect(() => {
				var items = icon_view.get_selected_items();
				this._add_button.sensitive = (items.length() > 0);
			});

			this.add_buttons(STOCK_CLOSE, ResponseType.CLOSE,
			                 STOCK_ADD, ResponseType.OK,
			                 null);
			this.set_default_response(ResponseType.OK);
			this._add_button = (Button)this.get_widget_for_response(ResponseType.OK);
			this._add_button.sensitive = false;

			this.response.connect((response) => {
				if (response == ResponseType.OK) {
					add_selected();
					Signal.stop_emission_by_name(this, "response");
				}
			});
		}

		public void load_sources (Ppg.Session session) {
			var conn = session.connection;
			Gtk.TreeIter iter;
			Gdk.Pixbuf pixbuf;
			string[] sources;
			int i;

			_session = session;

			try {
				var theme = IconTheme.get_default();
				pixbuf = theme.load_icon("perfkit-plugin", 32, 0);
			} catch (Error err) {
				debug("No plugin icon found: %s", err.message);
				pixbuf = null;
			}

			try {
				conn.manager_get_plugins(out sources);
				for (i = 0; sources[i] != null; i++) {
					model.append(out iter);
					model.set(iter,
					          0, sources[i],
					          1, sources[i].down(),
					          2, sources[i],
					          3, pixbuf,
					          4, true,
					          -1);
				}
			} catch (Error err) {
				/* TODO: Display to user */
				warning("Error retrieving sources: %s", err.message);
			}
		}

		void add_selected () {
			var items = icon_view.get_selected_items();

			if (items.length() > 0) {
				Gtk.TreeIter iter;
				string id;

				var path = items.nth_data(0);
				filter.get_iter(out iter, path);
				filter.get(iter, 0, out id, -1);

				debug("Adding item %s", id);

				_session.add_source_plugin(id);
			}
		}

		static const string title_msg = "Choose one or more sources to add to your session.";
	}
}
