
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
url_dispatched (GObject * obj, GAsyncResult * res, gpointer user_data)
{
	GError * error = NULL;
	dispatch_data_t * dispatch_data = (dispatch_data_t *)user_data;

	service_iface_com_canonical_urldispatcher_call_dispatch_url_finish(
		SERVICE_IFACE_COM_CANONICAL_URLDISPATCHER(obj),
		res,
		&error);

	if (error != NULL) {
		g_warning("Unable to dispatch url '%s':%s", dispatch_data->url, error->message);
		g_error_free(error);

		if (dispatch_data->cb != NULL) {
			dispatch_data->cb(dispatch_data->url, FALSE, dispatch_data->user_data);
		}
	} else {
		if (dispatch_data->cb != NULL) {
			dispatch_data->cb(dispatch_data->url, TRUE, dispatch_data->user_data);
		}
	}

	g_free(dispatch_data->url);
	g_free(dispatch_data);

	return;
}

static void
got_proxy (GObject * obj, GAsyncResult * res, gpointer user_data)
{
	GError * error = NULL;
	dispatch_data_t * dispatch_data = (dispatch_data_t *)user_data;

	dispatch_data->proxy = service_iface_com_canonical_urldispatcher_proxy_new_for_bus_finish(res, &error);

	if (error != NULL) {
		g_warning("Unable to get proxy for URL Dispatcher: %s", error->message);
		g_error_free(error);

		if (dispatch_data->cb != NULL) {
			dispatch_data->cb(dispatch_data->url, FALSE, dispatch_data->user_data);
		}

		g_free(dispatch_data->url);
		g_free(dispatch_data);
		return;
	}

	service_iface_com_canonical_urldispatcher_call_dispatch_url(
		dispatch_data->proxy,
		dispatch_data->url,
		NULL, /* cancelable */
		url_dispatched,
		dispatch_data);

	return;
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
