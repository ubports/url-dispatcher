
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
	std::set<std::tuple<std::string, std::string, std::shared_ptr<MirPromptSession>>> ongoingSessions;

public:
	OverlayTrackerMir (void); 
	~OverlayTrackerMir (void); 
	void addOverlay (const char * appid, unsigned long pid) override;

	static void sessionStateChangedStatic (MirPromptSession * session, MirPromptSessionState state, void * user_data);
	void sessionStateChanged (MirPromptSession * session, MirPromptSessionState state);

	static void untrustedHelperStoppedStatic (const gchar * appid, const gchar * instanceid, const gchar * helpertype, gpointer user_data);
	void untrustedHelperStopped(const gchar * appid, const gchar * instanceid, const gchar * helpertype, gpointer user_data);
};
