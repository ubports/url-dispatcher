
#include "url-dispatcher.h"
#include "service-iface.h"

typedef struct _dispatch_data_t dispatch_data_t;
struct _dispatch_data_t {
	ServiceIfaceComCanonicalURLDispatcher * proxy;
	URLDispatchCallback cb;
	gpointer user_data;
	gchar * url;
};

static void
got_proxy (GObject * obj, GAsyncResult * res, gpointer user_data)
{


}

void
url_dispatch_send (const gchar * url, URLDispatchCallback cb, gpointer user_data)
{
	dispatch_data_t * dispatch_data = g_new0(dispatch_data_t, 1);

	dispatch_data->cb = cb;
	dispatch_data->user_data = user_data;
	dispatch_data->url = g_strdup(url);

	service_iface_com_canonical_urldispatcher_proxy_new_for_bus(
		G_BUS_TYPE_SESSION,
		G_DBUS_PROXY_FLAGS_DO_NOT_LOAD_PROPERTIES | G_DBUS_PROXY_FLAGS_DO_NOT_CONNECT_SIGNALS,
		"com.canonical.URLDispatcher",
		"/com/canonical/URLDispatcher",
		NULL, /* cancellable */
		got_proxy,
		dispatch_data
	);

	return;
}
