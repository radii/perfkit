/* dialogs/preferences-dialog.vala
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
	public class PreferencesDialog: Gtk.Dialog {
		construct {
			this.title = _("Perfkit Profiler Preferences");
			this.border_width = 6;
			this.default_width = 400;
			this.default_height = 450;
			this.has_separator = false;
			this.modal = false;
			this.add_button(STOCK_CLOSE, ResponseType.CLOSE);
			this.set_default_response(ResponseType.CLOSE);

			var notebook = new Notebook();
			notebook.border_width = 6;
			notebook.show();
			this.vbox.pack_start(notebook, true, true, 0);

			var l = new Label(_("General"));
			var l2 = new Label(null);
			l.show();
			l2.show();
			notebook.append_page(l2, l);
		}
	}
}
