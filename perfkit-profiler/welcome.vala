/* welcome.vala
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
	enum Column {
		ICON_NAME =  0,
		TITLE     =  1,
		PATH      =  2,
		SEPARATOR =  3,
		LAST      = -1
	}

	public class Welcome: Gtk.Window {
		Button local_button;
		Button remote_button;
		Button open_button;
		Button import_button;
		Button video_button;

		ListStore model;

		construct {
			set("title", _("Perfkit Profiler"),
			    "width-request", 620,
			    "height-request", 420,
			    "resizable", false,
			    "type", WindowType.TOPLEVEL,
			    "type-hint", Gdk.WindowTypeHint.DIALOG,
			    null);

			var vbox = new VBox(false, 0);
			vbox.show();
			this.add(vbox);

			var hbox = new HBox(false, 0);
			vbox.pack_start(hbox, true, true, 0);
			hbox.show();

			var scroller = new ScrolledWindow(null, null);
			scroller.set("width-request", 200,
			             "hscrollbar-policy", PolicyType.AUTOMATIC,
			             "vscrollbar-policy", PolicyType.AUTOMATIC,
			             "shadow-type", ShadowType.NONE,
			             null);
			scroller.show();
			hbox.pack_start(scroller, false, true, 0);

			var treeview = new TreeView();
			treeview.headers_visible = false;
			treeview.set_row_separator_func((model, iter) => {
				bool is_sep = false;
				model.get(iter,
				          Column.SEPARATOR, &is_sep,
				          Column.LAST);
				return is_sep;
			});
			treeview.show();
			scroller.add(treeview);

			var column = new TreeViewColumn();
			var cpix = new CellRendererPixbuf();
			cpix.stock_size = IconSize.DND;
			cpix.ypad = 3;
			cpix.xpad = 3;
			column.pack_start(cpix, false);
			column.add_attribute(cpix, "icon-name", Column.ICON_NAME);
			var ctext = new CellRendererText();
			column.pack_start(ctext, true);
			column.add_attribute(ctext, "text", Column.TITLE);
			treeview.append_column(column);

			var content_vbox = new VBox(false, 0);
			content_vbox.border_width = 12;
			content_vbox.spacing = 6;
			hbox.pack_start(content_vbox, true, true, 0);
			content_vbox.show();

			var title = new Label(null);
			title.set("label", _(title_msg),
			          "ypad", 12,
			          "use-markup", true,
			          null);
			title.show();
			content_vbox.pack_start(title, false, true, 0);

			local_button = create_button("computer", 48, local_msg);
			local_button.show();
			content_vbox.pack_start(local_button, false, true, 0);
			local_button.clicked.connect(local_clicked);
			local_button.grab_focus();

			remote_button = create_button("network-server", 48, remote_msg);
			remote_button.sensitive = false; /* FIXME: disabled */
			remote_button.show();
			content_vbox.pack_start(remote_button, false, true, 0);

			open_button = create_button(STOCK_OPEN, 48, open_msg);
			open_button.sensitive = false; /* FIXME: disabled */
			open_button.show();
			content_vbox.pack_start(open_button, false, true, 0);

			var l = new Label(null);
			l.show();
			content_vbox.pack_start(l, true, true, 0);

			var sep = new HSeparator();
			sep.show();
			content_vbox.pack_start(sep, false, true, 0);

			import_button = create_button("document-import", 16, import_msg);
			import_button.sensitive = false; /* FIXME: disabled */
			import_button.show();
			content_vbox.pack_start(import_button, false, true, 0);

			video_button = create_button("media-video", 16, video_msg);
			video_button.sensitive = false; /* FIXME: disabled */
			video_button.show();
			content_vbox.pack_start(video_button, false, true, 0);

			var statusbar = new Statusbar();
			statusbar.show();
			vbox.pack_start(statusbar, false, true, 0);

			model = new ListStore(4,
			                      typeof(string),
			                      typeof(string),
			                      typeof(string),
			                      typeof(bool));
			treeview.model = model;

			TreeIter iter;
			model.append(out iter);
			model.set(iter,
			          Column.ICON_NAME, STOCK_HOME,
			          Column.TITLE, _("Home"),
			          Column.SEPARATOR, false,
			          Column.LAST);
			treeview.get_selection().select_iter(iter);

			model.append(out iter);
			model.set(iter,
			          Column.SEPARATOR, true,
			          Column.LAST);
		}

		void local_clicked (Button button) {
			var window = new Ppg.Window();
			window.present();
			this.destroy();
		}

		Button create_button (string icon_name,
		                      int pixel_size,
		                      string label)
		{
			var hbox = new HBox(false, 12);
			var img = new Image();
			var l = new Label(_(label));
			var button = new Button();

			img.pixel_size = pixel_size;
			img.icon_name = icon_name;

			l.use_markup = true;
			l.xalign = 0.0f;
			l.set_max_width_chars(-1);

			hbox.pack_start(img, false, true, 0);
			hbox.pack_start(l, true, true, 0);
			hbox.show_all();

			button.relief = ReliefStyle.NONE;
			button.add(hbox);
			button.show();

			return button;
		}

		static const string title_msg  = """<span size="larger" weight="bold"><span size="larger">Getting Started with Perfkit</span></span>""";
		static const string local_msg  = "Start profiling a new or existing process\non your local machine";
		static const string remote_msg = "Start profiling a new or existing process\non a remote machine";
		static const string open_msg   = "Open an existing profiling session";
		static const string import_msg = "Import a profiling template";
		static const string video_msg  = "View video tutorials";
	}
}
