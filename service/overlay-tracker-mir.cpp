#include "overlay-tracker-mir.h"

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
	});
}

