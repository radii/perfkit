/* instrument.vala
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
	public abstract class Instrument: GLib.Object {
		List<Visualizer> _visualizers;

		/**
		 * Signal emitted when a visualizer is added.
		 */
		public virtual signal void visualizer_added (Visualizer visualizer) {
			visualizer.instrument = this;
			_visualizers.prepend(visualizer);
		}

		/**
		 * Signal emitted when a visualizer is removed.
		 */
		public virtual signal void visualizer_removed (Visualizer visualizer) {
			_visualizers.remove(visualizer);
		}

		/**
		 * The title of the instrument to display within the UI.
		 */
		public abstract string title { get; }

		/**
		 * Retrieves the list of active visualizers.
		 */
		public List<Visualizer> visualizers {
			get { return _visualizers; }
		}

		/**
		 * The data view to show when the instrument is selected.
		 */
		public abstract Widget? data_view { get; }

		/**
		 * The current data selection.
		 */
		public virtual Selection selection {
			set { /* nothing */ }
		}

		/**
		 * The GtkStyle for the instrument.
		 */
		public Gtk.Style style { get; set; }

		construct {
			_visualizers = new List<Visualizer>();
		}

		/**
		 * If the instrument is supported on the remote agent.
		 */
		public virtual bool is_supported (Ppg.Session session) throws GLib.Error {
			return true;
		}

		/**
		 * Load the instrument on the remote agent.
		 */
		public virtual void load (Ppg.Session session) throws GLib.Error {
		}

		/**
		 * Unload the instrument on the remote session.
		 */
		public virtual void unload (Ppg.Session session) throws GLib.Error {
		}

		/**
		 * The instrument has been muted. It should mute the underlying
		 * sources if possible.
		 */
		public virtual void mute () throws GLib.Error {
		}

		/**
		 * The instrument should be unmuted. It should restart the underlying
		 * sources if possible.
		 */
		public virtual void unmute () throws GLib.Error {
		}

		/**
		 * List the available visualizers to be seen in this instrument.
		 */
		public abstract string[] list_visualizers ();

		/**
		 * Add the visualizer matching name.
		 */
		public virtual Visualizer? add_visualizer (string name) throws GLib.Error {
			return null;
		}

		/**
		 * Remove a visualizer.
		 */
		public virtual void remove_visualizer (Visualizer visualizer) throws GLib.Error {
			visualizer_removed(visualizer);
		}
	}
}
