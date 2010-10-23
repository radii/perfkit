/* ppg-window-ui.h
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

static char ppg_window_ui[] =
	"<ui>"
	"  <menubar name=\"menubar\">"
	"    <menu action=\"file\">"
	"      <menuitem action=\"close\"/>"
	"      <menuitem action=\"quit\"/>"
	"    </menu>"
	"    <menu action=\"edit\">"
	"      <menuitem action=\"cut\"/>"
	"      <menuitem action=\"copy\"/>"
	"      <menuitem action=\"paste\"/>"
	"      <separator/>"
	"      <menuitem action=\"preferences\"/>"
	"    </menu>"
	"    <menu action=\"view\">"
	"      <menuitem action=\"zoom-in\"/>"
	"      <menuitem action=\"zoom-out\"/>"
	"      <menuitem action=\"zoom-one\"/>"
	"      <separator/>"
	"      <menuitem action=\"fullscreen\"/>"
	"    </menu>"
	"    <menu action=\"profiler\">"
	"      <menu action=\"target\">"
	"        <menuitem action=\"target-spawn\"/>"
	"        <menuitem action=\"target-existing\"/>"
	"        <separator/>"
	"        <menuitem action=\"target-none\"/>"
	"      </menu>"
	"      <separator/>"
	"      <menuitem action=\"stop\"/>"
	"      <menuitem action=\"pause\"/>"
	"      <menuitem action=\"run\"/>"
	"      <menuitem action=\"restart\"/>"
	"      <separator/>"
	"      <menuitem action=\"settings\"/>"
	"    </menu>"
	"    <menu action=\"instrument\">"
	"      <menuitem action=\"add-instrument\"/>"
	"      <separator/>"
	"      <menu action=\"visualizers\">"
	"      </menu>"
	"      <separator/>"
	"      <menuitem action=\"configure-instrument\"/>"
	"    </menu>"
	"    <menu action=\"help\">"
	"      <menuitem action=\"about\"/>"
	"    </menu>"
	"  </menubar>"
	"  <toolbar name=\"toolbar\">"
	"    <toolitem action=\"stop\"/>"
	"    <toolitem action=\"pause\"/>"
	"    <toolitem action=\"run\"/>"
	"    <toolitem action=\"restart\"/>"
	"    <separator/>"
	"    <toolitem action=\"add-instrument\"/>"
	"  </toolbar>"
	"  <popup name=\"target-popup\">"
	"    <menuitem action=\"target-spawn\"/>"
	"    <menuitem action=\"target-existing\"/>"
	"    <separator/>"
	"    <menuitem action=\"target-none\"/>"
	"  </popup>"
	"  <popup name=\"instrument-popup\">"
	"    <menuitem action=\"add-instrument\"/>"
	"    <separator/>"
	"    <menu action=\"visualizers\">"
	"    </menu>"
	"    <separator/>"
	"    <menuitem action=\"configure-instrument\"/>"
	"  </popup>"
	"</ui>";
