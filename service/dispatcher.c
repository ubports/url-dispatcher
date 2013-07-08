#include <gio/gio.h>
#include "service-iface.h"

/* Globals */
GMainLoop * mainloop = NULL;
GCancellable * cancellable = NULL;
ServiceIfaceComCanonicalURLDispatcher * skel = NULL;

/* Get a URL off of the bus */
static gboolean
dispatch_url (GObject * skel, GDBusMethodInvocation * invocation, const gchar * url, gpointer user_data)
{


	return TRUE;
}

/* Callback when we're connected to dbus */
static void
bus_got (GObject * obj, GAsyncResult * res, gpointer user_data)
{
	GDBusConnection * bus = NULL;
	GError * error = NULL;

	bus = g_bus_get_finish(res, &error);

	if (error != NULL) {
		g_error("Unable to connect to D-Bus: %s", error->message);
		g_main_loop_quit(mainloop);
		return;
	}

	g_dbus_interface_skeleton_export(G_DBUS_INTERFACE_SKELETON(skel), bus, "/com/canonical/URLDispatcher", &error);
	if (error != NULL) {
		g_error("Unable to export interface skeleton: %s", error->message);
		g_main_loop_quit(mainloop);
		return;
	}

	/* TODO: Own name */

	return;
}

/* Where it all begins */
int
main (int argc, char * argv[])
{
	mainloop = g_main_loop_new(NULL, FALSE);
	cancellable = g_cancellable_new();

	g_bus_get(G_BUS_TYPE_SESSION, cancellable, bus_got, NULL);

	skel = service_iface_com_canonical_urldispatcher_skeleton_new();
	g_signal_connect(skel, "handle-dispatch-url", G_CALLBACK(dispatch_url), NULL);

	/* Run Main */
	g_main_loop_run(mainloop);

	/* Clean up globals */
	g_main_loop_unref(mainloop);
	g_object_unref(cancellable);
	g_object_unref(skel);

	return 0;
}
