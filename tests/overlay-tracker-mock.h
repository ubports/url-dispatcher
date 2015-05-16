
#pragma once
#include "overlay-tracker-iface.h"
#include <utility>

class OverlayTrackerMock : public OverlayTrackerIface
{
	public:
		std::vector<std::tuple<std::string, unsigned long, std::string>> addedOverlays;

		bool addOverlay (const char * appid, unsigned long pid, const char * url) {
			addedOverlays.push_back(std::make_tuple(std::string(appid), pid, std::string(url)));
			return true;
		}
};
