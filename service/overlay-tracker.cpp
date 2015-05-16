
extern "C" {
#include "overlay-tracker.h"
}

#include "overlay-tracker-iface.h"
#include "overlay-tracker-mir.h"

OverlayTracker *
overlay_tracker_new (void) {
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
