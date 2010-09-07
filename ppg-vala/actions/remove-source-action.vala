/* remove-source-action.vala
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
	public class RemoveSourceAction: Gtk.Action {
		Ppg.Window _widget;

		construct {
			set("name", "RemoveSourceAction",
			    "label", _("Remove source"),
			    "icon-name", STOCK_REMOVE,
			    "sensitive", false,
			    null);
		}

		public Widget widget {
			set {
				if (value.get_type().is_a(typeof(Ppg.Window))) {
					_widget = (Ppg.Window)value;
					_widget.row_changed.connect(() => {
						if (_widget.get_selected_row() >= 0) {
							this.sensitive = true;
						} else {
							this.sensitive = false;
						}
					});
				}
			}
		}

		public override void activate () {
			if (_widget != null) {
				var dialog = new MessageDialog(_widget,
				                               DialogFlags.MODAL,
				                               MessageType.QUESTION,
				                               ButtonsType.OK_CANCEL,
				                               _("<span size=\"larger\" weight=\"bold\">Are you sure you want to delete the source named %s?</span>"),
				                               "SOME SOURCE");
				dialog.use_markup = true;
				dialog.secondary_text = _("Deleting a source cannot be undone. Doing so will result in the sources information being lost.");
				dialog.set_default_response(ResponseType.CANCEL);
				if (dialog.run() == ResponseType.OK) {
				}
				dialog.destroy();
			}
		}
	}
}
