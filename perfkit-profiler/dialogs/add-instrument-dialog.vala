/* add-instrument-dialog.vala
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
	public class AddInstrumentDialog: Dialog {
		Ppg.Session     _session;

		Button          _add_button;
		Entry           _search_entry;
		IconView        _icon_view;
		Spinner         _spinner;
		InfoBar         _info_bar;
		Label           _message;

		ListStore       _model;
		TreeModelFilter _filter;

		construct {
			this.title = _("Perfkit Profiler");
			this.border_width = 6;
			this.default_width = 350;
			this.default_height = 400;
			this.has_separator = false;

			_info_bar = new InfoBar();
			_message = new Label(null);
			_message.xalign = 0.0f;
			_message.wrap = true;
			_info_bar.add(_message);
			this.vbox.pack_start(_info_bar, false, true, 0);

			var vbox = new VBox(false, 0);
			vbox.border_width = 6;
			this.vbox.add(vbox);
			vbox.show();

			var l = new Label(_("Choose one or more instruments to add to your session."));
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
			_search_entry.changed.connect(on_search_changed);
			_search_entry.show();

			l = new Label(_("_Search:"));
			l.mnemonic_widget = _search_entry;
			l.use_underline = true;
			hbox.add_with_properties(l, "expand", false, "position", 0, null);
			l.show();

			var scroller = new ScrolledWindow(null, null);
			scroller.hscrollbar_policy = PolicyType.AUTOMATIC;
			scroller.vscrollbar_policy = PolicyType.AUTOMATIC;
			scroller.shadow_type = ShadowType.IN;
			vbox.pack_start(scroller, true, true, 0);
			scroller.show();

			_icon_view = new IconView();
			scroller.add(_icon_view);
			_icon_view.show();
			_icon_view.item_activated.connect(() => { add_selected(); });

			_model = new ListStore(6,
			                       typeof(string),             // Id
			                       typeof(string),             // Search text (lowercase)
			                       typeof(string),             // Title
			                       typeof(Gdk.Pixbuf),         // Icon
			                       typeof(bool),               // Visible
			                       typeof(InstrumentFactory)); // Factory object
			_filter = new TreeModelFilter(_model, null);
			_filter.set_visible_column(4);
			_icon_view.model = _filter;
			_icon_view.set_text_column(2);
			_icon_view.set_pixbuf_column(3);
			_icon_view.selection_changed.connect(() => {
				var items = _icon_view.get_selected_items();
				this._add_button.sensitive = (items.length() > 0);
			});

			this.add_button(STOCK_CLOSE, ResponseType.CLOSE);
			this._add_button = (Button)this.add_button(STOCK_ADD, ResponseType.OK);
			this._add_button.sensitive = false;
			this.set_default_response(ResponseType.OK);

			var align = new Alignment(1.0f, 0.5f, 0.0f, 0.0f);
			this.action_area.add_with_properties(align,
			                                     "expand", false,
			                                     "fill", true,
			                                     "position", 0,
			                                     null);
			align.show();

			_spinner = new Spinner();
			_spinner.set_size_request(16, 16);
			align.add(_spinner);
			_spinner.start();

			this.response.connect((response) => {
				if (response == ResponseType.OK) {
					add_selected();
					Signal.stop_emission_by_name(this, "response");
				}
			});
		}

		void on_search_changed (Editable editable) {
			Gtk.TreeIter iter;
			string text = _search_entry.text.down();
			int matched = 0;

			if (_model.get_iter_first(out iter)) {
				do {
					string id;

					_model.get(iter, 1, out id, -1);
					if (id.contains(text)) {
						matched++;
						_model.set(iter, 4, true, -1);
					} else {
						_model.set(iter, 4, false, -1);
					}
				} while (_model.iter_next(ref iter));
			}

			/* If only one item is available, select it. */
			if (matched == 1) {
				_filter.get_iter_first(out iter);
				var path = _filter.get_path(iter);
				_icon_view.select_path(path);
			}
		}

		public void load_instruments (Ppg.Session session) {
			Gdk.Pixbuf pixbuf;

			_session = session;

			try {
				var theme = IconTheme.get_default();
				pixbuf = theme.load_icon("perfkit-plugin", 32, 0);
			} catch (Error err) {
				debug("No plugin icon found: %s", err.message);
				pixbuf = null;
			}

			foreach (var factory in Instruments.get_factories()) {
				Gtk.TreeIter iter;

				var fulltext = "%s %s".printf(factory.id.down(),
				                              factory.title.down());
				_model.append(out iter);
				_model.set(iter,
				           0, factory.id,
				           1, fulltext,
				           2, factory.title,
				           3, pixbuf,
				           4, true,
				           5, factory,
				           -1);
			}
		}

		void add_selected () {
			var items = _icon_view.get_selected_items();

			if (items.length() > 0) {
				Gtk.TreeIter iter;
				InstrumentFactory factory;

				var path = items.nth_data(0);
				_filter.get_iter(out iter, path);
				_filter.get(iter, 5, out factory, -1);

				assert(factory != null);

				try {
					_session.add_instrument(factory);
				} catch (GLib.Error error) {
					var dialog = new Gtk.MessageDialog(
						this, DialogFlags.MODAL,
						MessageType.ERROR, ButtonsType.CLOSE,
						_("Failed to add instrument to session."));
					dialog.secondary_text = "%s".printf(error.message);
					dialog.run();
					dialog.destroy();
				}
			}
		}
	}
}
