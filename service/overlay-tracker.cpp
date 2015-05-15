
extern "C" {
#include "overlay-tracker.h"
}

#include "overlay-tracker-iface.h"
#include "overlay-tracker-mir.h"

OverlayTracker * overlay_tracker_new (void) {
	try {
		OverlayTrackerMir * cpptracker = new OverlayTrackerMir();
		return reinterpret_cast<OverlayTracker *>(cpptracker);
	} catch (...) {
		return nullptr;
	}
}

void overlay_tracker_delete (OverlayTracker * tracker) {
	g_return_if_fail(tracker != nullptr);

	auto cpptracker = reinterpret_cast<OverlayTrackerMir *>(tracker);
	delete cpptracker;
	return;
}

void overlay_tracker_add (OverlayTracker * tracker, const char * appid, unsigned long pid) {
	g_return_if_fail(tracker != nullptr);
	g_return_if_fail(appid != nullptr);
	g_return_if_fail(pid != 0);

	reinterpret_cast<OverlayTrackerIface *>(tracker)->addOverlay(appid, pid);
	return;
}
