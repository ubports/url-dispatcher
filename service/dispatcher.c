#include <gio/gio.h>

/* Globals */
GMainLoop * mainloop = NULL;
GCancellable * cancellable = NULL;

/* Callback when we're connected to dbus */
void
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

	return;
}

/* Where it all begins */
int
main (int argc, char * argv[])
{
	mainloop = g_main_loop_new(NULL, FALSE);
	cancellable = g_cancellable_new();

	g_bus_get(G_BUS_TYPE_SESSION, cancellable, bus_got, NULL);

	/* Run Main */
	g_main_loop_run(mainloop);

	/* Clean up globals */
	g_main_loop_unref(mainloop);

	return 0;
}
