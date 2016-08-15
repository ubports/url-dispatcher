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

#pragma once

#include <map>
#include <set>
#include <vector>

#include <mir_toolkit/mir_connection.h>
#include <mir_toolkit/mir_prompt_session.h>

#include "glib-thread.h"
#include "overlay-tracker-iface.h"

class OverlayTrackerMir : public OverlayTrackerIface {
private:
	struct OverlayData {
		std::string appid;
		std::string instanceid;
		std::shared_ptr<MirPromptSession> session;
	};

	GLib::ContextThread thread;
	std::shared_ptr<MirConnection> mir;
	std::map<std::string, std::vector<OverlayData>> ongoingSessions;
	std::set<std::pair<std::string, std::shared_ptr<MirPromptSession>>> badUrlSessions;

public:
	OverlayTrackerMir (); 
	~OverlayTrackerMir (); 
	bool addOverlay (const char * appid, unsigned long pid, const char * url) override;
	bool badUrl (unsigned long pid, const char * url) override;

private:
	/* Overlay Functions */
	void removeSession (const std::string &type, MirPromptSession * session);

	static void badUrlSessionStateChangedStatic (MirPromptSession * session, MirPromptSessionState state, void * user_data);
	static void overlaySessionStateChangedStatic (MirPromptSession * session, MirPromptSessionState state, void * user_data);
	void sessionStateChanged (MirPromptSession * session, MirPromptSessionState state, const std::string &type);

	static void overlayHelperStoppedStatic (const gchar * appid, const gchar * instanceid, const gchar * helpertype, gpointer user_data);
	void overlayHelperStopped(const gchar * appid, const gchar * instanceid, const gchar * helpertype);

};
