#include "overlay-tracker-mir.h"
#include <ubuntu-app-launch.h>

static const char * HELPER_TYPE = "url-overlay";

OverlayTrackerMir::OverlayTrackerMir (void) 
	: thread([this] {
		/* Setup Helper Observer */
		},
		[this] {
		/* Remove Helper Observer */
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

void
OverlayTrackerMir::addOverlay (const char * appid, unsigned long pid)
{
	std::string sappid(appid);

	thread.executeOnThread([this, sappid, pid] {
		g_debug("Setting up over lay for PID %d with '%s'", pid, sappid.c_str());

		auto session = std::shared_ptr<MirPromptSession>(
			mir_connection_create_prompt_session_sync(mir.get(), pid, sessionStateChangedStatic, this),
			[] (MirPromptSession * session) { if (session) mir_prompt_session_release_sync(session); });
		
		auto instance = ubuntu_app_launch_start_session_helper(HELPER_TYPE, session.get(), sappid.c_str(), nullptr /* TODO */);

		ongoingSessions.emplace(std::make_pair(std::string(instance), session));
	});
}

void
OverlayTrackerMir::sessionStateChangedStatic (MirPromptSession * session, MirPromptSessionState state, void * user_data)
{
	reinterpret_cast<OverlayTrackerMir *>(user_data)->sessionStateChanged(session, state);
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
		for (auto it = ongoingSessions.begin(); it != ongoingSessions.end(); it++) {
			if (it->second.get() == session) {
				ubuntu_app_launch_stop_multiple_helper(HELPER_TYPE, "foo" /* TODO */, it->first.c_str());
				ongoingSessions.erase(it);
				break;
			}
		}
	});
}
