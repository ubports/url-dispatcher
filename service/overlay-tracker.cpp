#include <mir_toolkit/mir_connection.h>
#include <mir_toolkit/mir_prompt_session.h>

extern "C" {
#include "overlay-tracker.h"
}

#include "glib-thread.h"

class OverlayTrackerMir {

private:
	GLib::ContextThread thread;
	std::shared_ptr<MirConnection> mir;

public:
	OverlayTrackerMir (void) 
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
	addOverlay (const char * appid, unsigned long pid)
	{
		std::string sappid(appid);

		thread.executeOnThread([this, sappid, pid] {
			g_debug("Setting up over lay for PID %d with '%s'", pid, sappid.c_str());
		});
	}




};

OverlayTracker * overlay_tracker_new (void) {
	try {
		OverlayTrackerMir * cpptracker = new OverlayTrackerMir();
		return reinterpret_cast<OverlayTracker *>(cpptracker);
	} catch (...) {
		return nullptr;
	}
}

void overlay_tracker_delete (OverlayTracker * tracker) {
	g_return_if_fail(tracker != nullptr);

	auto cpptracker = reinterpret_cast<OverlayTrackerMir *>(tracker);
	delete cpptracker;
	return;
}

void overlay_tracker_add (OverlayTracker * tracker, const char * appid, unsigned long pid) {
	g_return_if_fail(tracker != nullptr);
	g_return_if_fail(appid != nullptr);
	g_return_if_fail(pid != 0);

	reinterpret_cast<OverlayTrackerMir *>(tracker)->addOverlay(appid, pid);
	return;
}
