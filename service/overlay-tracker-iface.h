#pragma once

class OverlayTrackerIface {
public:
	virtual ~OverlayTrackerIface() = default;
	virtual bool addOverlay (const char * appid, unsigned long pid, const char * url) = 0;
};
