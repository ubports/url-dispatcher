#pragma once

class OverlayTrackerIface {
public:
	virtual ~OverlayTrackerIface() = default;
	virtual void addOverlay (const char * appid, unsigned long pid) = 0;
};
