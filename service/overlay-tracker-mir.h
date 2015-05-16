
#pragma once

#include <set>

#include <mir_toolkit/mir_connection.h>
#include <mir_toolkit/mir_prompt_session.h>

#include "glib-thread.h"
#include "overlay-tracker-iface.h"

class OverlayTrackerMir : public OverlayTrackerIface {
private:
	GLib::ContextThread thread;
	std::shared_ptr<MirConnection> mir;
	std::set<std::pair<std::string, std::shared_ptr<MirPromptSession>>> ongoingSessions;

public:
	OverlayTrackerMir (void); 
	void addOverlay (const char * appid, unsigned long pid) override;

	static void sessionStateChangedStatic (MirPromptSession * session, MirPromptSessionState state, void * user_data);
	void sessionStateChanged (MirPromptSession * session, MirPromptSessionState state);
};
