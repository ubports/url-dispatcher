
#include "test-config.h"

#include <gio/gio.h>
#include <gtest/gtest.h>
#include <libdbustest/dbus-test.h>

#include "overlay-tracker-mir.h"

class OverlayTrackerTest : public ::testing::Test
{
	protected:
		DbusTestService * service = nullptr;
		DbusTestDbusMock * mock = nullptr;
		DbusTestDbusMockObject * obj = nullptr;
		DbusTestDbusMockObject * jobobj = nullptr;
		GDBusConnection * bus = nullptr;

		virtual void SetUp() {
			g_setenv("UBUNTU_APP_LAUNCH_USE_SESSION", "1", TRUE);
			g_setenv("URL_DISPATCHER_DISABLE_RECOVERABLE_ERROR", "1", TRUE);

			service = dbus_test_service_new(nullptr);

			/* Upstart Mock */
			mock = dbus_test_dbus_mock_new("com.ubuntu.Upstart");
			obj = dbus_test_dbus_mock_get_object(mock, "/com/ubuntu/Upstart", "com.ubuntu.Upstart0_6", nullptr);

			dbus_test_dbus_mock_object_add_method(mock, obj,
				"GetJobByName",
				G_VARIANT_TYPE_STRING,
				G_VARIANT_TYPE_OBJECT_PATH, /* out */
				"ret = dbus.ObjectPath('/job')", /* python */
				nullptr); /* error */

			jobobj = dbus_test_dbus_mock_get_object(mock, "/job", "com.ubuntu.Upstart0_6.Job", nullptr);

			dbus_test_dbus_mock_object_add_method(mock, jobobj,
				"Start",
				G_VARIANT_TYPE("(asb)"),
				G_VARIANT_TYPE_OBJECT_PATH, /* out */
				"ret = dbus.ObjectPath('/instance')", /* python */
				nullptr); /* error */

			dbus_test_task_set_name(DBUS_TEST_TASK(mock), "Upstart");
			dbus_test_service_add_task(service, DBUS_TEST_TASK(mock));

			/* Start your engines! */
			dbus_test_service_start_tasks(service);

			bus = g_bus_get_sync(G_BUS_TYPE_SESSION, nullptr, nullptr);
			g_dbus_connection_set_exit_on_close(bus, FALSE);
			g_object_add_weak_pointer(G_OBJECT(bus), (gpointer *)&bus);
			return;
		}

		virtual void TearDown() {
			g_clear_object(&mock);
			g_clear_object(&service);

			g_object_unref(bus);

			unsigned int cleartry = 0;
			while (bus != nullptr && cleartry < 100) {
				pause(100);
				while (g_main_pending())
					g_main_iteration(TRUE);
				cleartry++;
			}

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

	guint callslen = 0;
	const DbusTestDbusMockCall * calls = dbus_test_dbus_mock_object_get_method_calls(mock, jobobj, "Start", &callslen, nullptr);

	ASSERT_EQ(callslen, 1);

	/* Making sure the APP_ID is here.  We're not testing more to
	   make it so the tests break less, that should be tested in
	   Upstart App Launch, we don't need to retest */
	GVariant * env = g_variant_get_child_value(calls->params, 0);
	GVariantIter iter;
	bool found_appid = false;
	g_variant_iter_init(&iter, env);
	gchar * var = nullptr;

	while (g_variant_iter_loop(&iter, "s", &var)) {
		if (g_strcmp0(var, "APP_ID=app-id") == 0) {
			ASSERT_FALSE(found_appid);
			found_appid = true;
		}
	}

	g_variant_unref(env);

	ASSERT_TRUE(found_appid);

	delete tracker;
}
