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
	bool addOverlay (const char * appid, unsigned long pid, const char * url) override;

	void removeSession (MirPromptSession * session);

	static void sessionStateChangedStatic (MirPromptSession * session, MirPromptSessionState state, void * user_data);
	void sessionStateChanged (MirPromptSession * session, MirPromptSessionState state);

	static void untrustedHelperStoppedStatic (const gchar * appid, const gchar * instanceid, const gchar * helpertype, gpointer user_data);
	void untrustedHelperStopped(const gchar * appid, const gchar * instanceid, const gchar * helpertype);
};
