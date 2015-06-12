/*
 * Copyright © 2015 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3, as published
 * by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranties of
 * MERCHANTABILITY, SATISFACTORY QUALITY, or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authors:
 *   Ted Gould <ted.gould@canonical.com>
 */

extern "C" {
#include "overlay-tracker.h"
}

#include "overlay-tracker-iface.h"
#include "overlay-tracker-mir.h"

OverlayTracker *
overlay_tracker_new () {
	try {
		OverlayTrackerMir * cpptracker = new OverlayTrackerMir();
		return reinterpret_cast<OverlayTracker *>(cpptracker);
	} catch (...) {
		return nullptr;
	}
}

void
overlay_tracker_delete (OverlayTracker * tracker) {
	g_return_if_fail(tracker != nullptr);

	auto cpptracker = reinterpret_cast<OverlayTrackerMir *>(tracker);
	delete cpptracker;
	return;
}

gboolean
overlay_tracker_add (OverlayTracker * tracker, const char * appid, unsigned long pid, const gchar * url) {
	g_return_val_if_fail(tracker != nullptr, FALSE);
	g_return_val_if_fail(appid != nullptr, FALSE);
	g_return_val_if_fail(pid != 0, FALSE);
	g_return_val_if_fail(url != nullptr, FALSE);

	return reinterpret_cast<OverlayTrackerIface *>(tracker)->addOverlay(appid, pid, url) ? TRUE : FALSE;
}
