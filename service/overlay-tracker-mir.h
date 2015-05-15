
#include <mir_toolkit/mir_connection.h>
#include <mir_toolkit/mir_prompt_session.h>

#include "glib-thread.h"
#include "overlay-tracker-iface.h"

class OverlayTrackerMir : public OverlayTrackerIface {
private:
	GLib::ContextThread thread;
	std::shared_ptr<MirConnection> mir;

public:
	OverlayTrackerMir (void); 
	void addOverlay (const char * appid, unsigned long pid) override;
};
