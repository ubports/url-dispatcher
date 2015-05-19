
#include <random>

#include "test-config.h"

#include <gio/gio.h>
#include <gtest/gtest.h>
#include <libdbustest/dbus-test.h>

#include "overlay-tracker-mir.h"
#include "ubuntu-app-launch-mock.h"
#include "mir-mock.h"

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

	auto mirconn = mir_mock_connect_last_connect();
	EXPECT_EQ("mir_socket_trusted", mirconn.first.substr(mirconn.first.size() - 18));
	EXPECT_EQ("url-dispatcher", mirconn.second);

	EXPECT_TRUE(tracker->addOverlay("app-id", 5, "http://no-name-yet.com"));

	EXPECT_EQ(5, mir_mock_last_trust_pid);

	EXPECT_STREQ("url-overlay", ubuntu_app_launch_mock_last_start_session_helper);
	EXPECT_STREQ("app-id", ubuntu_app_launch_mock_last_start_session_appid);
	EXPECT_STREQ("http://no-name-yet.com", ubuntu_app_launch_mock_last_start_session_uris[0]);

	delete tracker;

	EXPECT_STREQ("url-overlay", ubuntu_app_launch_mock_last_stop_helper);
	EXPECT_STREQ("app-id", ubuntu_app_launch_mock_last_stop_appid);
	EXPECT_STREQ("instance", ubuntu_app_launch_mock_last_stop_instance);
}

TEST_F(OverlayTrackerTest, OverlayABunch) {
	OverlayTrackerMir tracker;
	std::uniform_int_distribution<> randpid(1, 32000);
	std::mt19937 rand;

	/* Testing adding a bunch of overlays, we're using pretty standard
	   data structures, but let's make sure we didn't break 'em */
	for (auto name : std::vector<std::string>{"warty", "hoary", "breezy", "dapper", "edgy", "feisty", "gutsy", "hardy", "intrepid", "jaunty", "karmic", "lucid", "maverick", "natty", "oneiric", "precise", "quantal", "raring", "saucy", "trusty", "utopic", "vivid", "wily"}) {
		int pid = randpid(rand);
		tracker.addOverlay(name.c_str(), pid, "http://ubuntu.com/releases");

		EXPECT_EQ(pid, mir_mock_last_trust_pid);
		EXPECT_EQ(name, ubuntu_app_launch_mock_last_start_session_appid);
	}
}

TEST_F(OverlayTrackerTest, UALSignalStop) {
	OverlayTrackerMir tracker;

	/* Call with the overlay before it is set */
	ubuntu_app_launch_mock_observer_helper_stop_func("app-id", "instance", "url-overlay", ubuntu_app_launch_mock_observer_helper_stop_user_data);

	EXPECT_TRUE(tracker.addOverlay("app-id", 5, "http://no-name-yet.com"));

	mir_mock_last_released_session = nullptr;
	ubuntu_app_launch_mock_observer_helper_stop_func("app-id", "instance", "url-overlay", ubuntu_app_launch_mock_observer_helper_stop_user_data);
	EXPECT_NE(nullptr, mir_mock_last_released_session);
}

TEST_F(OverlayTrackerTest, MirSignalStop) {
	OverlayTrackerMir tracker;

	EXPECT_TRUE(tracker.addOverlay("app-id", 5, "http://no-name-yet.com"));

	EXPECT_NE(nullptr, mir_mock_last_trust_func);
	mir_mock_last_trust_func(mir_mock_valid_trust_session, mir_prompt_session_state_stopped, mir_mock_last_trust_data);

	pause(100);

	EXPECT_STREQ("url-overlay", ubuntu_app_launch_mock_last_stop_helper);
	EXPECT_STREQ("app-id", ubuntu_app_launch_mock_last_stop_appid);
	EXPECT_STREQ("instance", ubuntu_app_launch_mock_last_stop_instance);
}
