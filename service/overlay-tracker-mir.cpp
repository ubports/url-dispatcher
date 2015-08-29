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

static const char * OVERLAY_HELPER_TYPE = "url-overlay";
static const char * BAD_URL_HELPER_TYPE = "bad-url";

OverlayTrackerMir::OverlayTrackerMir () 
	: thread([this] {
		/* Setup Helper Observer */
		ubuntu_app_launch_observer_add_helper_stop(overlayHelperStoppedStatic, OVERLAY_HELPER_TYPE, this);
		ubuntu_app_launch_observer_add_helper_stop(badUrlHelperStoppedStatic, BAD_URL_HELPER_TYPE, this);
		},
		[this] {
		/* Remove Helper Observer */
		ubuntu_app_launch_observer_delete_helper_stop(overlayHelperStoppedStatic, OVERLAY_HELPER_TYPE, this);
		ubuntu_app_launch_observer_delete_helper_stop(badUrlHelperStoppedStatic, BAD_URL_HELPER_TYPE, this);
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
			removeOverlaySession(ongoingSessions.begin()->session.get());
		}

		while (!badUrlSessions.empty()) {
			removeBadUrlSession(badUrlSessions.begin()->second.get());
		}

		return true;
	});

	mir.reset();
}

bool
OverlayTrackerMir::addOverlay (const char * appid, unsigned long pid, const char * url)
{
	OverlayData data;
	data.appid = appid;
	std::string surl(url);

	return thread.executeOnThread<bool>([this, &data, pid, surl] {
		g_debug("Setting up over lay for PID %d with '%s'", pid, data.appid.c_str());

		data.session = std::shared_ptr<MirPromptSession>(
			mir_connection_create_prompt_session_sync(mir.get(), pid, overlaySessionStateChangedStatic, this),
			[] (MirPromptSession * session) { if (session) mir_prompt_session_release_sync(session); });
		if (!data.session) {
			g_critical("Unable to create trusted prompt session for %d with appid '%s'", pid, data.appid.c_str());
			return false;
		}

		std::array<const char *, 2> urls { surl.c_str(), nullptr };
		auto instance = ubuntu_app_launch_start_session_helper(OVERLAY_HELPER_TYPE, data.session.get(), data.appid.c_str(), urls.data());
		if (instance == nullptr) {
			g_critical("Unable to start helper for %d with appid '%s'", pid, data.appid.c_str());
			return false;
		}
		data.instanceid = instance;

		ongoingSessions.push_back(data);
		g_free(instance);
		return true;
	});
}

bool
OverlayTrackerMir::badUrl (unsigned long pid, const char * url)
{
	std::string surl(url);

	return thread.executeOnThread<bool>([this, pid, surl] {
		g_debug("Setting up bad URL for PID %d for '%s'", pid, surl.c_str());

		auto session = std::shared_ptr<MirPromptSession>(
			mir_connection_create_prompt_session_sync(mir.get(), pid, badUrlSessionStateChangedStatic, this),
			[] (MirPromptSession * session) { if (session) mir_prompt_session_release_sync(session); });
		if (!session) {
			g_critical("Unable to create a bad url trusted prompt session for %d on url '%s'", pid, surl.c_str());
			return false;
		}
		
		std::array<const char *, 2> urls { surl.c_str(), nullptr };
		auto instance = ubuntu_app_launch_start_session_helper(BAD_URL_HELPER_TYPE, session.get(), "appid-isnt-used", urls.data());
		if (instance == nullptr) {
			g_critical("Unable to start bad url helper for %d with url '%s'", pid, surl.c_str());
			return false;
		}

		badUrlSessions.emplace(std::make_pair(std::string(instance), session));
		g_free(instance);
		return true;
	});
}

void
OverlayTrackerMir::overlaySessionStateChangedStatic (MirPromptSession * session, MirPromptSessionState state, void * user_data)
{
	reinterpret_cast<OverlayTrackerMir *>(user_data)->overlaySessionStateChanged(session, state);
}

void
OverlayTrackerMir::removeOverlaySession (MirPromptSession * session)
{
	for (auto it = ongoingSessions.begin(); it != ongoingSessions.end(); it++) {
		if (it->session.get() == session) {
			ubuntu_app_launch_stop_multiple_helper(OVERLAY_HELPER_TYPE, it->appid.c_str(), it->instanceid.c_str());
			ongoingSessions.erase(it);
			break;
		}
	}
}

void
OverlayTrackerMir::overlaySessionStateChanged (MirPromptSession * session, MirPromptSessionState state)
{
	if (state != mir_prompt_session_state_stopped) {
		/* We only care about the stopped state */
		return;
	}

	/* Executing on the Mir thread, which is nice and all, but we
	   want to get back on our thread */
	thread.executeOnThread([this, session]() {
		removeOverlaySession(session);
	});
}

void
OverlayTrackerMir::overlayHelperStoppedStatic (const gchar * appid, const gchar * instanceid, const gchar * helpertype, gpointer user_data)
{
	reinterpret_cast<OverlayTrackerMir *>(user_data)->overlayHelperStopped(appid, instanceid, helpertype);
}

void 
OverlayTrackerMir::overlayHelperStopped(const gchar * appid, const gchar * instanceid, const gchar * helpertype)
{
	/* This callback will happen on our thread already, we don't need
	   to proxy it on */
	if (g_strcmp0(helpertype, OVERLAY_HELPER_TYPE) != 0) {
		return;
	}

	/* Making the code in the loop easier to read by using std::string outside */
	std::string sappid(appid);
	std::string sinstanceid(instanceid);

	for (auto it = ongoingSessions.begin(); it != ongoingSessions.end(); it++) {
		if (it->appid == sappid && it->instanceid == sinstanceid) {
			ongoingSessions.erase(it);
			break;
		}
	}
}

void
OverlayTrackerMir::badUrlSessionStateChangedStatic (MirPromptSession * session, MirPromptSessionState state, void * user_data)
{
	reinterpret_cast<OverlayTrackerMir *>(user_data)->badUrlSessionStateChanged(session, state);
}

void
OverlayTrackerMir::removeBadUrlSession (MirPromptSession * session)
{
	for (auto it = badUrlSessions.begin(); it != badUrlSessions.end(); it++) {
		if (it->second.get() == session) {
			ubuntu_app_launch_stop_multiple_helper(BAD_URL_HELPER_TYPE, "appid-isnt-used", it->first.c_str());
			badUrlSessions.erase(it);
			break;
		}
	}
}

void
OverlayTrackerMir::badUrlSessionStateChanged (MirPromptSession * session, MirPromptSessionState state)
{
	if (state != mir_prompt_session_state_stopped) {
		/* We only care about the stopped state */
		return;
	}

	/* Executing on the Mir thread, which is nice and all, but we
	   want to get back on our thread */
	thread.executeOnThread([this, session]() {
		removeBadUrlSession(session);
	});
}

void
OverlayTrackerMir::badUrlHelperStoppedStatic (const gchar * appid, const gchar * instanceid, const gchar * helpertype, gpointer user_data)
{
	reinterpret_cast<OverlayTrackerMir *>(user_data)->badUrlHelperStopped(appid, instanceid, helpertype);
}

void 
OverlayTrackerMir::badUrlHelperStopped(const gchar *, const gchar * instanceid, const gchar * helpertype)
{
	/* This callback will happen on our thread already, we don't need
	   to proxy it on */
	if (g_strcmp0(helpertype, BAD_URL_HELPER_TYPE) != 0) {
		return;
	}

	/* Making the code in the loop easier to read by using std::string outside */
	std::string sinstanceid(instanceid);

	for (auto it = badUrlSessions.begin(); it != badUrlSessions.end(); it++) {
		if (it->first == sinstanceid) {
			badUrlSessions.erase(it);
			break;
		}
	}
}
