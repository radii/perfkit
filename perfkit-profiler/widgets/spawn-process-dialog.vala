/* spawn-process-dialog.vala
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
	public errordomain DialogError {
		INVALID,
	}

	public class SpawnProcessDialog: Gtk.Dialog {
		Entry target_entry;
		Entry args_entry;
		ValidationBox target_box;
		ValidationBox args_box;
		TreeView env_tree;
		ListStore env_model;
		TreeViewColumn key_column;
		TreeViewColumn value_column;

		construct {
			this.border_width = 6;
			this.default_width = 350;
			this.has_separator = false;
			this.title = "";

			var vbox = new VBox(false, 6);
			vbox.border_width = 6;
			((Container)this.vbox).add(vbox);
			vbox.show();

			var l = new Label(_("<b>_Target</b>"));
			l.xalign = 0.0f;
			l.use_markup = true;
			l.use_underline = true;
			vbox.pack_start(l, false, true, 0);
			l.show();

			target_box = new ValidationBox();
			vbox.pack_start(target_box, false, true, 0);
			target_box.show();

			target_entry = new Entry();
			target_entry.activates_default = true;
			target_box.add(target_entry);
			l.mnemonic_widget = target_entry;
			target_entry.show();

			l = new Label(_("<b>_Arguments</b>"));
			l.xalign = 0.0f;
			l.use_markup = true;
			l.use_underline = true;
			vbox.pack_start(l, false, true, 0);
			l.show();

			args_box = new ValidationBox();
			vbox.pack_start(args_box, false, true, 0);
			args_box.show();

			args_entry = new Entry();
			args_entry.activates_default = true;
			args_box.add(args_entry);
			l.mnemonic_widget = args_entry;
			args_entry.show();

			l = new Label(_("<b>_Environment</b>"));
			l.xalign = 0.0f;
			l.use_underline = true;
			l.use_markup = true;
			vbox.pack_start(l, false, true, 0);
			l.show();

			var s = new ScrolledWindow(null, null);
			s.set_shadow_type(ShadowType.IN);
			s.set_policy(PolicyType.AUTOMATIC, PolicyType.AUTOMATIC);
			vbox.pack_start(s, true, true, 0);
			s.show();

			env_tree = new TreeView();
			env_tree.width_request = 250;
			s.add(env_tree);
			env_tree.show();

			env_model = new ListStore(2, typeof(string), typeof(string));
			env_tree.model = env_model;

			key_column = new TreeViewColumn();
			key_column.expand = true;
			key_column.title = _("Key");
			env_tree.append_column(key_column);

			var cell = new CellRendererText();
			cell.editable = true;
			cell.edited.connect((_, path_str, text) => {
				TreeIter iter;
				TreePath path = new TreePath.from_string(path_str);

				if (env_model.get_iter(out iter, path)) {
					env_model.set(iter, 0, text, -1);
				}
			});
			key_column.pack_start(cell, true);
			key_column.add_attribute(cell, "text", 0);

			value_column = new TreeViewColumn();
			value_column.expand = true;
			value_column.title = _("Value");
			env_tree.append_column(value_column);

			cell = new CellRendererText();
			cell.editable = true;
			cell.edited.connect((_, path_str, text) => {
				TreeIter iter;
				TreePath path = new TreePath.from_string(path_str);

				if (env_model.get_iter(out iter, path)) {
					env_model.set(iter, 1, text, -1);
				}
			});
			value_column.pack_start(cell, true);
			value_column.add_attribute(cell, "text", 1);

			var hbox = new HBox(false, 6);
			vbox.pack_start(hbox, false, true, 0);
			hbox.show();

			var add_image = new Image.from_stock(STOCK_ADD, IconSize.MENU);
			var rem_image = new Image.from_stock(STOCK_REMOVE, IconSize.MENU);
			add_image.show();
			rem_image.show();

			var add_button = new Button();
			add_button.add(add_image);
			add_button.set_tooltip_text(_("Add an environment variable"));
			add_button.relief = ReliefStyle.NONE;
			hbox.pack_start(add_button, false, true, 0);
			add_button.show();
			add_button.clicked.connect(() => {
				TreeIter iter;
				TreePath path;

				env_model.append(out iter);
				env_model.set(iter, 0, "KEY", 1, "VALUE", -1);
				path = env_model.get_path(iter);
				env_tree.get_selection().select_path(path);
				env_tree.set_cursor(path, key_column, true);
			});

			var rem_button = new Button();
			rem_button.add(rem_image);
			rem_button.set_tooltip_text(_("Remove an environment variable"));
			rem_button.relief = ReliefStyle.NONE;
			rem_button.sensitive = false;
			hbox.pack_start(rem_button, false, true, 0);
			rem_button.show();

			rem_button.clicked.connect(() => {
				var sel = env_tree.get_selection();
				TreeModel model;
				TreeIter iter;

				if (sel.get_selected(out model, out iter)) {
					env_model.remove(iter);
				}
			});

			env_tree.get_selection().changed.connect((sel) => {
				rem_button.sensitive = (sel.count_selected_rows() > 0);
			});

			target_entry.changed.connect(() => {
				target_box.validate();
			});

			args_entry.changed.connect(() => {
				args_box.validate();
			});

			this.response.connect((_, response) => {
				bool valid = true;
				Widget? grab = null;

				if (response != ResponseType.OK) {
					return;
				}
				
				if (!target_box.validate()) {
					valid = false;
					if (grab == null) {
						grab = target_entry;
					}
				}
				if (!args_box.validate()) {
					valid = false;
					if (grab == null) {
						grab = args_entry;
					}
				}
				if (!valid) {
					if (grab != null) {
						grab.grab_focus();
					}
					Signal.stop_emission_by_name(this, "response");
				}
			});

			target_box.set_validation_func((c) => {
				if (target_entry.text[0] == '\0') {
					throw new DialogError.INVALID("Please specify a target.");
				}
				return true;
			});

			args_box.set_validation_func((c) => {
				var text = args_entry.text;
				if (text[0] != '\0') {
					string[] argv;
					Shell.parse_argv(text, out argv);
				}
				return true;
			});

			this.add_button(STOCK_CANCEL, ResponseType.CANCEL);
			this.add_button(STOCK_OK, ResponseType.OK);
			this.set_default_response(ResponseType.OK);
		}
	}
}
