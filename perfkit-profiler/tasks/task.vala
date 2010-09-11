/* task.vala
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
	public errordomain TaskError {
		INVALID
	}

	public abstract class Task: GLib.InitiallyUnowned {
		Cancellable _cancellable = new Cancellable();

		public virtual signal void started () {
			this.ref();
		}

		public virtual signal void finished (bool success, GLib.Error? error) {
			this.unref();
		}

		public double progress { get; set; }

		public bool is_async { get; construct; default=true; }

		public Cancellable cancellable {
			get { return _cancellable; }
		}

		~Task () {
			debug("Disposing %s.", this.get_type().name());
		}

		public abstract void run ();

		public virtual void finish () {
			this.finished(true, null);
		}

		public virtual void finish_with_error (GLib.Error? error=null) {
			if (error == null) {
				var my_error = new TaskError.INVALID("Invalid task state");
				this.finished(false, my_error);
			} else {
				this.finished(false, error);
			}
		}

		public virtual void cancel_on_delete (Widget widget) {
			widget.delete_event.connect_after(() => {
				_cancellable.cancel();
				return false;
			});
		}

		public virtual void schedule () {
			if (is_async) {
				this.started();
				this.run();
			} else {
				error("Only asynchronous tasks are currently supported");
			}
		}
	}
}
