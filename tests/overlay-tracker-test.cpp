
#include "test-config.h"

#include <gio/gio.h>
#include <gtest/gtest.h>
#include <libdbustest/dbus-test.h>

#include "overlay-tracker-mir.h"
#include "ubuntu-app-launch-mock.h"

class OverlayTrackerTest : public ::testing::Test
{
	protected:
		virtual void SetUp() {
			g_setenv("URL_DISPATCHER_DISABLE_RECOVERABLE_ERROR", "1", TRUE);

			return;
		}

		virtual void TearDown() {
			return;
		}

		static gboolean quit_loop (gpointer ploop) {
			g_main_loop_quit((GMainLoop *)ploop);
			return FALSE;
		}

		void pause (int time) {
			GMainLoop * loop = g_main_loop_new(nullptr, FALSE);
			g_timeout_add(time, quit_loop, loop);
			g_main_loop_run(loop);
			g_main_loop_unref(loop);
		}
};

TEST_F(OverlayTrackerTest, BasicCreation) {
	auto tracker = new OverlayTrackerMir();
	delete tracker;
}

TEST_F(OverlayTrackerTest, AddOverlay) {
	auto tracker = new OverlayTrackerMir();

	EXPECT_TRUE(tracker->addOverlay("app-id", 5, "http://no-name-yet.com"));

	EXPECT_STREQ("url-overlay", ubuntu_app_launch_mock_last_start_session_helper);
	EXPECT_STREQ("app-id", ubuntu_app_launch_mock_last_start_session_appid);
	EXPECT_STREQ("http://no-name-yet.com", ubuntu_app_launch_mock_last_start_session_uris[0]);

	delete tracker;

	EXPECT_STREQ("url-overlay", ubuntu_app_launch_mock_last_stop_helper);
	EXPECT_STREQ("app-id", ubuntu_app_launch_mock_last_stop_appid);
	EXPECT_STREQ("instance", ubuntu_app_launch_mock_last_stop_instance);
}
