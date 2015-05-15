
#pragma once
#include "overlay-tracker-iface.h"
#include <utility>

class OverlayTrackerMock : public OverlayTrackerIface
{
	public:
		std::vector<std::pair<std::string, unsigned long>> addedOverlays;

		void addOverlay (const char * appid, unsigned long pid) {
			addedOverlays.push_back(std::make_pair(std::string(appid), pid));
		}
};
