/* validation-box.vala
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
	public delegate bool ValidationFunc (Widget widget) throws GLib.Error;

	public class ValidationBox: VBox {
		HBox hbox;
		Label message;
		Image image;
		Widget child_;
		ValidationFunc func;

		construct {
			this.spacing = 3;

			hbox = new HBox(false, 6);
			this.pack_start(hbox, true, true, 0);
			hbox.show();

			image = new Image.from_stock(STOCK_DIALOG_ERROR, IconSize.MENU);
			freeze_size(image);
			hbox.pack_start(image, false, true, 0);
			image.icon_name = null;
			image.show();

			message = new Label(null);
			message.wrap = true;
			message.xalign = 0.0f;
			this.pack_start(message, false, true, 0);
			message.show();
		}

		public override void add (Widget widget) {
			if (widget == hbox || widget == message) {
				base.add(widget);
			} else {
				if (child_ != null) {
					child_.unparent();
				}
				child_ = widget;
				hbox.add_with_properties(widget,
				                         "expand", true,
				                         "fill", true,
				                         "position", 0,
				                         null);
			}
		}

		public void set_validation_func (ValidationFunc func) {
			this.func = func;
		}

		public bool validate () {
			if (this.func == null) {
				return true;
			}
			try {
				this.func(this.child_);
				message.label = "";
				image.icon_name = null;
				return true;
			} catch (GLib.Error error) {
				var markup = Markup.printf_escaped("%s", error.message);
				message.set_markup(markup);
				image.icon_name = STOCK_DIALOG_ERROR;
				return false;
			}
		}

		static void freeze_size (Widget widget) {
			Requisition req;

			widget.size_request(out req);
			widget.set_size_request(req.width, req.height);
		}
	}
}
