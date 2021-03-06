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

#include "overlay-tracker-mir.h"
#include <ubuntu-app-launch.h>

static const char * HELPER_TYPE = "url-overlay";

OverlayTrackerMir::OverlayTrackerMir () 
	: thread([this] {
		/* Setup Helper Observer */
		ubuntu_app_launch_observer_add_helper_stop(untrustedHelperStoppedStatic, HELPER_TYPE, this);
		},
		[this] {
		/* Remove Helper Observer */
		ubuntu_app_launch_observer_delete_helper_stop(untrustedHelperStoppedStatic, HELPER_TYPE, this);
		})
{
	mir = std::shared_ptr<MirConnection>([] {
			gchar * path = g_build_filename(g_get_user_runtime_dir(), "mir_socket_trusted", NULL);
			MirConnection * con = mir_connect_sync(path, "url-dispatcher");
			g_free(path);
			return con;
		}(),
		[] (MirConnection * connection) {
			if (connection != nullptr)
				mir_connection_release(connection);
		});

	if (!mir) {
		throw std::runtime_error("Unable to connect to Mir");
	}
}

/* Enforce a shutdown order, sessions before connection */
OverlayTrackerMir::~OverlayTrackerMir () 
{
	thread.executeOnThread<bool>([this] {
		while (!ongoingSessions.empty()) {
			removeSession(std::get<2>(*ongoingSessions.begin()).get());
		}

		return true;
	});

	mir.reset();
}

bool
OverlayTrackerMir::addOverlay (const char * appid, unsigned long pid, const char * url)
{
	std::string sappid(appid);
	std::string surl(url);

	return thread.executeOnThread<bool>([this, sappid, pid, surl] {
		g_debug("Setting up over lay for PID %d with '%s'", pid, sappid.c_str());

		auto session = std::shared_ptr<MirPromptSession>(
			mir_connection_create_prompt_session_sync(mir.get(), pid, sessionStateChangedStatic, this),
			[] (MirPromptSession * session) { if (session) mir_prompt_session_release_sync(session); });
		if (!session) {
			g_critical("Unable to create trusted prompt session for %d with appid '%s'", pid, sappid.c_str());
			return false;
		}
		
		std::array<const char *, 2> urls { surl.c_str(), nullptr };
		auto instance = ubuntu_app_launch_start_session_helper(HELPER_TYPE, session.get(), sappid.c_str(), urls.data());
		if (instance == nullptr) {
			g_critical("Unable to start helper for %d with appid '%s'", pid, sappid.c_str());
			return false;
		}

		ongoingSessions.emplace(std::make_tuple(sappid, std::string(instance), session));
		g_free(instance);
		return true;
	});
}

void
OverlayTrackerMir::sessionStateChangedStatic (MirPromptSession * session, MirPromptSessionState state, void * user_data)
{
	reinterpret_cast<OverlayTrackerMir *>(user_data)->sessionStateChanged(session, state);
}

void
OverlayTrackerMir::removeSession (MirPromptSession * session)
{
	for (auto it = ongoingSessions.begin(); it != ongoingSessions.end(); it++) {
		if (std::get<2>(*it).get() == session) {
			ubuntu_app_launch_stop_multiple_helper(HELPER_TYPE, std::get<0>(*it).c_str(), std::get<1>(*it).c_str());
			ongoingSessions.erase(it);
			break;
		}
	}
}

void
OverlayTrackerMir::sessionStateChanged (MirPromptSession * session, MirPromptSessionState state)
{
	if (state != mir_prompt_session_state_stopped) {
		/* We only care about the stopped state */
		return;
	}

	/* Executing on the Mir thread, which is nice and all, but we
	   want to get back on our thread */
	thread.executeOnThread([this, session]() {
		removeSession(session);
	});
}

void
OverlayTrackerMir::untrustedHelperStoppedStatic (const gchar * appid, const gchar * instanceid, const gchar * helpertype, gpointer user_data)
{
	reinterpret_cast<OverlayTrackerMir *>(user_data)->untrustedHelperStopped(appid, instanceid, helpertype);
}

void 
OverlayTrackerMir::untrustedHelperStopped(const gchar * appid, const gchar * instanceid, const gchar * helpertype)
{
	/* This callback will happen on our thread already, we don't need
	   to proxy it on */
	if (g_strcmp0(helpertype, HELPER_TYPE) != 0) {
		return;
	}

	/* Making the code in the loop easier to read by using std::string outside */
	std::string sappid(appid);
	std::string sinstanceid(instanceid);

	for (auto it = ongoingSessions.begin(); it != ongoingSessions.end(); it++) {
		if (std::get<0>(*it) == sappid && std::get<1>(*it) == sinstanceid) {
			ongoingSessions.erase(it);
			break;
		}
	}
}
