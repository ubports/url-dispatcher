
#include <gio/gio.h>
#include <gtest/gtest.h>
#include "dispatcher.h"

class DispatcherTest : public ::testing::Test
{
	private:
		GTestDBus * testbus = NULL;
		GMainLoop * mainloop = NULL;

	protected:
		virtual void SetUp() {
			testbus = g_test_dbus_new(G_TEST_DBUS_NONE);
			g_test_dbus_up(testbus);

			mainloop = g_main_loop_new(NULL, FALSE);
			dispatcher_init(mainloop);

			return;
		}

		virtual void TearDown() {
			dispatcher_shutdown();

			/* Clean up queued events */
			while (g_main_pending()) {
				g_main_iteration(TRUE);
			}

			g_main_loop_unref(mainloop);

			g_test_dbus_down(testbus);
			g_object_unref(testbus);
			return;
		}
};

TEST_F(DispatcherTest, StubTest)
{
	return;
}
