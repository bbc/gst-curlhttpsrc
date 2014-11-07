/*
 * GStreamer
 * Copyright (C) 2005 Thomas Vander Stichele <thomas@apestaart.org>
 * Copyright (C) 2005 Ronald S. Bultje <rbultje@ronald.bitfreak.net>
 * Copyright (C) 2014 Sam Hurst <samuelh@rd.bbc.co.uk>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * Alternatively, the contents of this file may be used under the
 * GNU Lesser General Public License Version 2.1 (the "LGPL"), in
 * which case the following provisions apply instead of the ones
 * mentioned above:
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

/**
 * SECTION:element-curlhttpsrc
 *
 * FIXME:Describe curlhttpsrc here.
 *
 * <refsect2>
 * <title>Example launch line</title>
 * |[
 * gst-launch-1.0 playbin uri=http://rdmedia.bbc.co.uk/dash/testmpds/multiperiod/bbb.php
 * ]|
 * </refsect2>
 */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include "gstcurlhttpsrc.h"

GST_DEBUG_CATEGORY_STATIC (gst_curl_http_src_debug);
#define GST_CAT_DEFAULT gst_curl_http_src_debug

/*
 * Make a source pad template to be able to kick out recv'd data
 */
static GstStaticPadTemplate srcpadtemplate = GST_STATIC_PAD_TEMPLATE (
	"src",
	GST_PAD_SRC,
	GST_PAD_ALWAYS,
	GST_STATIC_CAPS_ANY
);

#define gst_curl_http_src_parent_class parent_class
G_DEFINE_TYPE_WITH_CODE (GstCurlHttpSrc, gst_curl_http_src, GST_TYPE_PUSH_SRC,
		G_IMPLEMENT_INTERFACE (GST_TYPE_URI_HANDLER,
				gst_curl_http_src_uri_handler_init));

static void
gst_curl_http_src_class_init (GstCurlHttpSrcClass * klass)
{
	GObjectClass *gobject_class;
	GstElementClass *gstelement_class;
	GstPushSrcClass *gstpushsrc_class;

	gobject_class = (GObjectClass *) klass;
	gstelement_class = (GstElementClass *) klass;
	gstpushsrc_class = (GstPushSrcClass *) klass;

	GST_INFO_OBJECT(klass, "class_init started!");

	gstelement_class->change_state =
			GST_DEBUG_FUNCPTR(gst_curl_http_src_change_state);
	gstpushsrc_class->create = GST_DEBUG_FUNCPTR(gst_curl_http_src_create);

	gst_element_class_add_pad_template(gstelement_class,
			gst_static_pad_template_get(&srcpadtemplate));

	gobject_class->set_property = gst_curl_http_src_set_property;
	gobject_class->get_property = gst_curl_http_src_get_property;

	g_object_class_install_property (gobject_class, PROP_URI,
		g_param_spec_string ("uri", "URI", "URI of resource requested",
			"", G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	g_object_class_install_property (gobject_class, PROP_PROXYURI,
		g_param_spec_string ("proxy", "Proxy", "URI of HTTP/S proxy server",
			"", G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	g_object_class_install_property (gobject_class, PROP_COOKIES,
		g_param_spec_boxed ("cookies", "Cookies", "List of HTTP Cookies",
			G_TYPE_STRV, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	g_object_class_install_property (gobject_class, PROP_USERAGENT,
		g_param_spec_string ("user-agent", "User-Agent", "URI of resource requested",
			"", G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	g_object_class_install_property (gobject_class, PROP_REDIRECT,
		g_param_spec_boolean ("automatic-redirect", "automatic-redirect",
			"Allow HTTP Redirections (HTTP Status Code 300 series)",
			FALSE, G_PARAM_READWRITE));

	g_object_class_install_property (gobject_class, PROP_MAXREDIRECT,
		g_param_spec_int ("max-redirect", "Max-Redirect",
			"Maximum number of permitted redirections. -1 is unlimited.",
			GSTCURL_MIN_REDIRECTIONS, GSTCURL_MAX_REDIRECTIONS, -1,
			G_PARAM_READWRITE));

	g_object_class_install_property (gobject_class, PROP_KEEPALIVE,
		g_param_spec_boolean ("keep-alive", "Keep-Alive",
			"Toggle keep-alive for connection reuse.",
			TRUE, G_PARAM_READWRITE));

	g_object_class_install_property (gobject_class, PROP_CONNECTIONMAXTIME,
		g_param_spec_uint ("max-connection-time", "Max-Connection-Time",
			"Maximum amount of time to keep-alive HTTP connections",
			GSTCURL_MIN_CONNECTION_TIME, GSTCURL_MAX_CONNECTION_TIME, 30,
			G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE));

	g_object_class_install_property (gobject_class, PROP_MAXCONCURRENT_SERVER,
		g_param_spec_uint ("max-connections-per-server", "Max-Connections-Per-Server",
			"Maximum number of connections allowed per server for HTTP/1.x",
			GSTCURL_MIN_CONNECTIONS_SERVER, GSTCURL_MAX_CONNECTIONS_SERVER,
			GSTCURL_DEFAULT_CONNECTIONS_SERVER,
			G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE));

	g_object_class_install_property (gobject_class, PROP_MAXCONCURRENT_PROXY,
		g_param_spec_uint ("max-connections-per-proxy", "Max-Connections-Per-Proxy",
			"Maximum number of concurrent connections allowed per proxy for HTTP/1.x",
			GSTCURL_MIN_CONNECTIONS_PROXY, GSTCURL_MAX_CONNECTIONS_PROXY,
			GSTCURL_DEFAULT_CONNECTIONS_PROXY,
			G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE));

	g_object_class_install_property (gobject_class, PROP_MAXCONCURRENT_GLOBAL,
		g_param_spec_uint ("max-connections", "Max-Connections",
			"Maximum number of concurrent connections allowed for HTTP/1.x",
			GSTCURL_MIN_CONNECTIONS_GLOBAL, GSTCURL_MAX_CONNECTIONS_GLOBAL,
			GSTCURL_DEFAULT_CONNECTIONS_GLOBAL,
			G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE));

	/* Add a debugging task so it's easier to debug in the Multi worker thread */
	GST_DEBUG_CATEGORY_INIT (gst_curl_loop_debug, "curl_multi_loop", 0,
			"libcURL loop thread debugging");
	gst_debug_log (gst_curl_loop_debug, GST_LEVEL_INFO, __FILE__, __func__,
			__LINE__, NULL, "Testing the curl_multi_loop debugging prints");

	g_mutex_init(&GstCurlHttpSrcLoopReadyMutex);
	g_mutex_lock(&GstCurlHttpSrcLoopReadyMutex);
	g_cond_init(&GstCurlHttpSrcLoopReadyCond);
	g_rec_mutex_init(&GstCurlHttpSrcLoopRecMutex);
	GstCurlHttpSrcLoopTask = gst_task_new(
			(GstTaskFunction)gst_curl_http_src_curl_multi_loop, NULL, NULL);
	gst_task_set_lock(GstCurlHttpSrcLoopTask, &GstCurlHttpSrcLoopRecMutex);
	if(gst_task_start(GstCurlHttpSrcLoopTask) == FALSE) {
		GSTCURL_ERROR_PRINT("Couldn't start Curl Multi Loop task!");
	}
	g_cond_wait(&GstCurlHttpSrcLoopReadyCond, &GstCurlHttpSrcLoopReadyMutex);
	GSTCURL_INFO_PRINT("Curl Multi loop has been correctly initialised!");
	g_mutex_unlock(&GstCurlHttpSrcLoopReadyMutex);


	gst_element_class_set_details_simple(gstelement_class,
			"HTTP Client Source using libcURL",
			"Source/Network",
			"Receiver data as a client over a network via HTTP using cURL",
			"Sam Hurst <samuelh@rd.bbc.co.uk>");
}

static void
gst_curl_http_src_set_property (GObject * object, guint prop_id,
		const GValue * value, GParamSpec * pspec)
{
	GstCurlHttpSrc *source = GST_CURLHTTPSRC (object);
	  GSTCURL_FUNCTION_ENTRY(source);

	  switch (prop_id) {
	  case PROP_URI:
		  if (source->uri != NULL) {
			  g_free (source->uri);
		  }
		  source->uri = g_value_dup_string (value);
		  break;
	  case PROP_PROXYURI:
		  if (source->proxy_uri != NULL) {
			  g_free (source->uri);
		  }
		  source->proxy_uri = g_value_dup_string (value);
		  break;
	  case PROP_COOKIES:
		  g_strfreev (source->cookies);
		  source->cookies = g_strdupv (g_value_get_boxed (value));
		  source->number_cookies = g_strv_length(source->cookies);
		  break;
	  case PROP_USERAGENT:
		  if(source->user_agent != NULL) {
			 g_free (source->user_agent);
		  }
		  source->user_agent = g_value_dup_string (value);
		  break;
	  case PROP_REDIRECT:
		  source->allow_3xx_redirect = g_value_get_boolean(value);
		  break;
	  case PROP_MAXREDIRECT:
		  source->max_3xx_redirects = g_value_get_int (value);
		  break;
	  case PROP_KEEPALIVE:
		  source->keep_alive = g_value_get_boolean(value);
		  break;
	  case PROP_CONNECTIONMAXTIME:
		  source->max_connection_time = g_value_get_uint (value);
		  break;
	  case PROP_MAXCONCURRENT_SERVER:
		  source->max_conns_per_server = g_value_get_uint(value);
		  break;
	  case PROP_MAXCONCURRENT_PROXY:
		  source->max_conns_per_proxy = g_value_get_uint(value);
		  break;
	  case PROP_MAXCONCURRENT_GLOBAL:
		  source->max_conns_global = g_value_get_uint(value);
		  break;
	  default:
	      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
	      break;
	  }
	  GSTCURL_FUNCTION_EXIT(source);
}

static void
gst_curl_http_src_get_property (GObject * object, guint prop_id,
		GValue * value, GParamSpec * pspec)
{
	GstCurlHttpSrc *source = GST_CURLHTTPSRC (object);
	GSTCURL_FUNCTION_ENTRY(source);

	switch (prop_id) {
	case PROP_URI:
		g_value_set_string(value, source->uri);
		break;
	case PROP_PROXYURI:
		g_value_set_string(value, source->proxy_uri);
		break;
	case PROP_COOKIES:
		g_value_set_boxed (value, source->cookies);
		break;
	case PROP_USERAGENT:
	  	g_value_set_string(value, source->user_agent);
	  	break;
	case PROP_REDIRECT:
	    g_value_set_boolean (value, source->allow_3xx_redirect);
	  	break;
	case PROP_MAXREDIRECT:
	  	g_value_set_int(value, source->max_3xx_redirects);
	  	break;
	case PROP_KEEPALIVE:
		g_value_set_boolean(value, source->keep_alive);
		break;
	case PROP_CONNECTIONMAXTIME:
		g_value_set_uint(value, source->max_connection_time);
		break;
	case PROP_MAXCONCURRENT_SERVER:
		g_value_set_uint(value, source->max_conns_per_server);
		break;
	case PROP_MAXCONCURRENT_PROXY:
		g_value_set_uint(value, source->max_conns_per_proxy);
		break;
	case PROP_MAXCONCURRENT_GLOBAL:
		g_value_set_uint(value, source->max_conns_global);
		break;
	default:
	    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
	    break;
	}
	GSTCURL_FUNCTION_EXIT(source);
}

static void
gst_curl_http_src_init (GstCurlHttpSrc * source)
{
	const gchar *proxy_uri = NULL;
	GSTCURL_FUNCTION_ENTRY(source);

	/* Assume everything is already free'd */
	source->uri = NULL;
	source->proxy_uri = NULL;
	source->proxy_user = NULL;
	source->proxy_pass = NULL;
	source->cookies = NULL;
	source->number_cookies = 0;
	source->end_of_message = FALSE;

	source->caps = gst_caps_new_empty();
	gst_pad_use_fixed_caps(GST_BASE_SRC_PAD(source));
	gst_base_src_set_automatic_eos (GST_BASE_SRC (source), FALSE);

	proxy_uri = g_getenv("http_proxy");
	if(proxy_uri != NULL) {
		source->proxy_uri = g_malloc(strlen(proxy_uri) * (sizeof(gchar) + 1));
		memcpy(source->proxy_uri, proxy_uri, strlen(proxy_uri));
	}

	source->mutex = g_malloc(sizeof(GMutex));
	g_mutex_init(source->mutex);
	source->finished = g_malloc(sizeof(GCond));
	g_cond_init(source->finished);

	GSTCURL_FUNCTION_EXIT(source);
}

static GstFlowReturn
gst_curl_http_src_create (GstPushSrc * psrc, GstBuffer ** outbuf)
{
	GstFlowReturn ret;
	GstMapInfo info;
	GstCurlHttpSrc *src = GST_CURLHTTPSRC(psrc);
	GSTCURL_FUNCTION_ENTRY(src);
	ret = GST_FLOW_OK;

	if(src->end_of_message == TRUE) {
		GST_DEBUG_OBJECT(src, "Full body received, signalling EOS for URI %s.",
				src->uri);
		src->end_of_message = FALSE;
		return GST_FLOW_EOS;
	}

	src->curl_handle = gst_curl_http_src_create_easy_handle(src);

	if(gst_curl_http_src_make_request(src) == FALSE) {
		return GST_FLOW_ERROR;
	}

	*outbuf = gst_buffer_new_allocate(NULL, src->len, NULL);
	gst_buffer_map(*outbuf, &info, GST_MAP_READWRITE);
	memcpy(info.data, src->msg, (size_t) src->len);

	gst_curl_http_src_negotiate_caps(src);

	gst_curl_http_src_destroy_easy_handle(src->curl_handle);

	/* Reset the return types as our instance will be reused with a new URI */
	g_free(src->msg);
	src->msg = NULL;
	g_free(src->headers.content_type);
	src->headers.content_type = NULL;
	src->len = 0;

	GSTCURL_FUNCTION_EXIT(src);
	return ret;
}

/*
 * "Negotiate" capabilities between us and the sink.
 * I.e. tell the sink device what data to expect. We can't be told what to send
 * unless we implement "only return to me if this type" property. Potential TODO
 */
static gboolean
gst_curl_http_src_negotiate_caps (GstCurlHttpSrc *src)
{
	if (src->headers.content_type != NULL) {
		if (src->caps != NULL) {
			GST_INFO_OBJECT (src, "Setting cap on Content-Type of %s",
					src->headers.content_type);
			src->caps = gst_caps_make_writable (src->caps);
			gst_caps_set_simple (src->caps, "content-type", G_TYPE_STRING,
					src->headers.content_type, NULL);
			if (!(GST_BASE_SRC(src), src->caps)) {
				GST_ERROR_OBJECT(src, "Setting caps failed!");
				return FALSE;
			}
		}
		else {
			GST_WARNING_OBJECT(src, "source->caps is not initialised!");
			return FALSE;
		}
	}
	else {
		GST_INFO_OBJECT (src, "No caps have been set, continue.");
	}
	return TRUE;
}

static GstStateChangeReturn
gst_curl_http_src_change_state (GstElement * element,
		GstStateChange transition)
{
	GstStateChangeReturn ret;
	GstCurlHttpSrc *source = GST_CURLHTTPSRC(element);
	GSTCURL_FUNCTION_ENTRY(source);

	switch(transition) {
	case GST_STATE_CHANGE_READY_TO_NULL:
		/* The pipeline has ended, so signal any running request to end. */
		gst_curl_http_src_request_remove(source);
		break;
	default:
		break;
	}

	ret = GST_ELEMENT_CLASS (parent_class)->change_state (element, transition);

	GSTCURL_FUNCTION_EXIT(source);
	return ret;
}

static void
gst_curl_http_src_uri_handler_init (gpointer g_iface, gpointer iface_data)
{
	GstURIHandlerInterface *uri_iface = (GstURIHandlerInterface *) g_iface;

	uri_iface->get_type = gst_curl_http_src_urihandler_get_type;
	uri_iface->get_protocols = gst_curl_http_src_urihandler_get_protocols;
	uri_iface->get_uri = gst_curl_http_src_urihandler_get_uri;
	uri_iface->set_uri = gst_curl_http_src_urihandler_set_uri;
}

static guint
gst_curl_http_src_urihandler_get_type (GType type)
{
  return GST_URI_SRC;
}

static const gchar *const *
gst_curl_http_src_urihandler_get_protocols (GType type)
{
  static const gchar *protocols[] = { "http", "https", NULL };

  return protocols;
}

static gchar *
gst_curl_http_src_urihandler_get_uri (GstURIHandler * handler)
{
  GstCurlHttpSrc *source = GST_CURLHTTPSRC (handler);
  GSTCURL_FUNCTION_ENTRY(source);

  /* FIXME: make thread-safe */
  return g_strdup (source->uri);
}

static gboolean
gst_curl_http_src_urihandler_set_uri (GstURIHandler * handler, const gchar * uri,
    GError ** error)
{
  GstCurlHttpSrc *source = GST_CURLHTTPSRC (handler);
  GSTCURL_FUNCTION_ENTRY(source);

  if (source->uri != NULL) {
	  GST_DEBUG_OBJECT(source, "URI already present as %s, updating to new URI %s",
			  source->uri, uri);
	  g_free(source->uri);
	  source->end_of_message = FALSE;
  }

  source->uri = g_strdup(uri);
  if(source->uri == NULL) {
	  return FALSE;
  }

  GSTCURL_FUNCTION_EXIT(source);
  return TRUE;
}

/*****************************************************************************
 * Curl loop task functions begin
 *****************************************************************************/

static void
gst_curl_http_src_curl_multi_loop(gpointer thread_data)
{
	CURLM *multi_handle;
	CURLMsg *curl_message;
	gboolean run, exit_cond, finished_processing;
	gint still_running, i, reason;
	GstCurlHttpSrcQueueElement *queue_element;

	GSTCURL_INFO_PRINT("cURL multi handle loop task has started!");

	g_mutex_lock(&GstCurlHttpSrcLoopReadyMutex);

	multi_handle = curl_multi_init();

	request_queue_mutex = g_malloc(sizeof(GMutex));
	if(request_queue_mutex == NULL) {
		GSTCURL_ERROR_PRINT("Couldn't malloc request_queue_mutex!");
		return;
	}
	g_mutex_init(request_queue_mutex);
	request_queue = NULL;

	curl_multi_loop_signal_mutex = g_malloc(sizeof(GMutex));
	if(curl_multi_loop_signal_mutex == NULL) {
		GSTCURL_ERROR_PRINT("Couldn't malloc curl_multi_loop_signal_mutex!");
		return;
	}
	g_mutex_init(curl_multi_loop_signal_mutex);
	curl_multi_loop_signaller = g_malloc(sizeof(GCond));
	if(curl_multi_loop_signaller == NULL) {
		GSTCURL_ERROR_PRINT("Couldn't malloc curl_multi_loop_signaller!");
		return;
	}
	g_cond_init(curl_multi_loop_signaller);
	curl_multi_loop_signal_state = GSTCURL_MULTI_LOOP_STATE_WAIT;

	request_removal_mutex = g_malloc(sizeof(GMutex));
	if(request_removal_mutex == NULL) {
		GSTCURL_ERROR_PRINT("Couldn't malloc request_removal_mutex!");
		return;
	}
	g_mutex_init(request_removal_mutex);
	request_removal_element = NULL;

	run = TRUE;
	still_running = 0;

	g_cond_signal(&GstCurlHttpSrcLoopReadyCond);
	g_mutex_unlock(&GstCurlHttpSrcLoopReadyMutex);

	while(run == TRUE) {
		g_mutex_lock(curl_multi_loop_signal_mutex);
		while(curl_multi_loop_signal_state == GSTCURL_MULTI_LOOP_STATE_WAIT) {
			GSTCURL_DEBUG_PRINT("Entering wait state...");
			g_cond_wait(curl_multi_loop_signaller, curl_multi_loop_signal_mutex);
			GSTCURL_DEBUG_PRINT("Received wake up call!");
		}

		if(curl_multi_loop_signal_state == GSTCURL_MULTI_LOOP_STATE_QUEUE_EVENT)
		{
			g_mutex_unlock(curl_multi_loop_signal_mutex);
			g_mutex_lock(request_queue_mutex);
			GSTCURL_DEBUG_PRINT("Received a new item on the queue!");
			if(request_queue == NULL) {
				GSTCURL_ERROR_PRINT("Request Queue was empty on a Queue Event!");
				break;
			}
			i = 1;
			queue_element = request_queue;
			exit_cond = FALSE;

			while(queue_element != NULL) {
				/*if(g_mutex_trylock(queue_element->running) == TRUE) {*/
				if(gst_curl_try_mutex(queue_element->running) == TRUE) {
					GSTCURL_DEBUG_PRINT("Adding easy handle for URI %s",
							queue_element->p->uri);
					curl_multi_add_handle(multi_handle,
							queue_element->p->curl_handle);
					GSTCURL_DEBUG_PRINT("Curl easy handle for URI %s added",
							queue_element->p->uri);
				}
				queue_element = queue_element->next;
			}

			g_mutex_unlock(request_queue_mutex);
			GSTCURL_DEBUG_PRINT("Finished adding all handles, continuing.");
			g_mutex_lock(curl_multi_loop_signal_mutex);
			curl_multi_loop_signal_state = GSTCURL_MULTI_LOOP_STATE_RUNNING;
			g_mutex_unlock(curl_multi_loop_signal_mutex);
		}
		else if(curl_multi_loop_signal_state == GSTCURL_MULTI_LOOP_STATE_RUNNING)
		{
			g_mutex_unlock(curl_multi_loop_signal_mutex);
			/*GSTCURL_DEBUG_PRINT("Performing curl operation...");*/
			finished_processing = FALSE;
			/*while(finished_processing != TRUE) {*/
			/* We have queue item(s), so poke curl with the do summat stick */
			struct timeval timeout;
			gint rc;

			fd_set fdread;
			fd_set fdwrite;
			fd_set fdexcep;
			int maxfd = -1;

			long curl_timeo = -1;

			FD_ZERO(&fdread);
			FD_ZERO(&fdwrite);
			FD_ZERO(&fdexcep);

			/* set a suitable timeout to play around with */
			timeout.tv_sec = 1;
			timeout.tv_usec = 0;

			curl_multi_timeout(multi_handle, &curl_timeo);
			if(curl_timeo >= 0) {
				timeout.tv_sec = curl_timeo / 1000;
				if(timeout.tv_sec > 1)
					timeout.tv_sec = 1;
				else
					timeout.tv_usec = (curl_timeo % 1000) * 1000;
			}

			/* get file descriptors from the transfers */
			curl_multi_fdset(multi_handle, &fdread, &fdwrite, &fdexcep, &maxfd);

			rc = select(maxfd+1, &fdread, &fdwrite, &fdexcep, &timeout);

			switch(rc) {
			case -1:
				/* select error */
				break;
			case 0:
			default:
				/* timeout or readable/writable sockets */
				curl_multi_perform(multi_handle, &still_running);
				break;
			}

			/*
			 * Check the CURL message buffer to find out if any transfers have
			 * completed. If they have, call the signal_finished function which
			 * will signal the g_cond_wait call in that calling instance.
			 */
			exit_cond = FALSE;
			i = 0;
			while(exit_cond != TRUE) {
				curl_message = curl_multi_info_read(multi_handle, &i);
				if(curl_message == NULL) {
					exit_cond = TRUE;
				}
				else if(curl_message->msg == CURLMSG_DONE) {
					if(gst_curl_http_src_signal_finished(curl_message->easy_handle,
							GSTCURL_RETURN_DONE) == FALSE) {
						GSTCURL_WARNING_PRINT("Couldn't signal to calling thread!");
					}
					curl_multi_remove_handle(multi_handle,
							curl_message->easy_handle);
				}
			}

			if(still_running == 0) {
				/* We've finished processing, so set the state to wait.
				 *
				 * This is a little more complex, as we need to catch the edge
				 * case of another thread adding a queue item while we've been
				 * working.
				 */
				g_mutex_lock(curl_multi_loop_signal_mutex);
				if((curl_multi_loop_signal_state ==
						GSTCURL_MULTI_LOOP_STATE_QUEUE_EVENT) ||
						(curl_multi_loop_signal_state ==
								GSTCURL_MULTI_LOOP_STATE_REQUEST_REMOVAL)) {
					g_mutex_unlock(curl_multi_loop_signal_mutex);
					continue;
				}
				else {
					curl_multi_loop_signal_state = GSTCURL_MULTI_LOOP_STATE_WAIT;
				}
				finished_processing = TRUE;
				g_mutex_unlock(curl_multi_loop_signal_mutex);
			}
			/*}*/
		}
		else if (curl_multi_loop_signal_state == GSTCURL_MULTI_LOOP_STATE_STOP)
		{
			g_mutex_unlock(curl_multi_loop_signal_mutex);
			/* Something wants us to shut down, so set the run condition */
			GSTCURL_INFO_PRINT("Got instruction to shut down");
			run = FALSE;
			reason = GSTCURL_RETURN_PIPELINE_NULL;
		}
		else if(curl_multi_loop_signal_state == GSTCURL_MULTI_LOOP_STATE_REQUEST_REMOVAL)
		{
			g_mutex_unlock(curl_multi_loop_signal_mutex);
			exit_cond = FALSE;
			queue_element = request_queue;
			while((exit_cond != TRUE)) {
				if(queue_element == NULL) {
					break;
				}
				if(queue_element->p == request_removal_element) {
					curl_multi_remove_handle(multi_handle,
							request_removal_element->curl_handle);
					gst_curl_http_src_signal_finished(
							request_removal_element->curl_handle,
							GSTCURL_RETURN_PIPELINE_NULL);
					exit_cond = TRUE;
				}
				queue_element = queue_element->next;
			}
			request_removal_element = NULL;
			g_mutex_unlock(request_removal_mutex);
		}
		else
		{
			GSTCURL_WARNING_PRINT("Curl Loop State was invalid or unsupported");
			GSTCURL_WARNING_PRINT("Signal State is %d, resetting to RUNNING.",
					curl_multi_loop_signal_state);
			/* Reset to running, so if there isn't anything to do it'll be
			 * changed the WAIT once curl_multi_perform says it has no active
			 * handles. */
			curl_multi_loop_signal_state = GSTCURL_MULTI_LOOP_STATE_RUNNING;
			g_mutex_unlock(curl_multi_loop_signal_mutex);
		}
	}
	/*
	 * We must deal with a possibility of the above bombing out when curl still
	 * has handles running. This can be if a new queue element arrived and was
	 * NULL, which simply cannot happen. Use the signal functionality to call
	 * back to connected clients to tell them that there was a failure so they
	 * can return GST_FLOW_ERROR to signal the pipeline that something horrible
	 * has happened.
	 *
	 * Alternatively, the thread could've been given the STATE_STOP signal in
	 * which case we've been asked to shut everything down.
	 */
	if(request_queue != NULL) {
		gst_curl_http_src_recurse_queue_cleanup(request_queue, reason);
	}
}

static gboolean
gst_curl_try_mutex(GMutex* gmutex) {
	pthread_mutex_t *pmutex;
	int pthread_ret;
	gboolean ret = FALSE;

	pmutex = g_atomic_pointer_get(&gmutex->p);
	if(pmutex == NULL) {
		GSTCURL_ERROR_PRINT("Couldn't get posix mutex handle!");
	}
	else {
		pthread_ret = pthread_mutex_trylock(pmutex);
		switch (pthread_ret) {
		case 0:
			GSTCURL_DEBUG_PRINT("pthread mutex has been locked!");
			ret = TRUE;
			break;
		case EINVAL:
			GSTCURL_ERROR_PRINT("pthread mutex couldn't be locked, err EINVAL");
			break;
		case EBUSY:
			GSTCURL_DEBUG_PRINT("pthread mutex is already locked");
			break;
		case EAGAIN:
			GSTCURL_ERROR_PRINT("pthread mutex couldn't be locked, exceed max recursive locks");
			break;
		default:
			GSTCURL_WARNING_PRINT("Unexpected posix return!");
			break;
		}
	}
	return ret;
}

/*
 * From the data in the queue element s, create a CURL easy handle and populate
 * options with the URL, proxy data, login options, cookies,
 */
static CURL*
gst_curl_http_src_create_easy_handle(GstCurlHttpSrc *s)
{
	CURL* handle;
	gint i;
	GSTCURL_FUNCTION_ENTRY(s);

	handle = curl_easy_init();
	if(handle == NULL) {
		GST_ERROR_OBJECT(s, "Couldn't init a curl easy handle!");
		return NULL;
	}
	GST_INFO_OBJECT(s, "Creating a new handle for URI %s", s->uri);

	/* This is mandatory and yet not default option, so if this is NULL
	 * then something very bad is going on. */
	curl_easy_setopt(handle, CURLOPT_URL, s->uri);

	gst_curl_setopt_str(handle, CURLOPT_PROXY, s->proxy_uri);
	gst_curl_setopt_str(handle, CURLOPT_PROXYUSERNAME, s->proxy_user);
	gst_curl_setopt_str(handle, CURLOPT_PROXYPASSWORD, s->proxy_pass);

	for(i = 0; i < s->number_cookies; i++) {
		gst_curl_setopt_str(handle, CURLOPT_COOKIELIST, s->cookies[i]);
	}
	gst_curl_setopt_int(handle, CURLOPT_FOLLOWLOCATION, s->allow_3xx_redirect);
	gst_curl_setopt_int_default(handle, CURLOPT_MAXREDIRS, s->max_3xx_redirects);
	gst_curl_setopt_int(handle, CURLOPT_TCP_KEEPALIVE, s->keep_alive);

	switch (s->preferred_http_version) {
	case GSTCURL_HTTP_VERSION_1_0:
		gst_curl_setopt_int(handle, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_1_0);
		break;
	case GSTCURL_HTTP_VERSION_1_1:
		gst_curl_setopt_int(handle, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_1_1);
		break;
	case GSTCURL_HTTP_VERSION_2_0:
		gst_curl_setopt_int(handle, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_2_0);
		break;
	default:
		/* TODO Print a dodgy or as yet unimplemented option! */
		GST_WARNING_OBJECT(s, "Supplied a bogus HTTP version, using curl default!");
	}

	curl_easy_setopt(handle, CURLOPT_HEADERFUNCTION, gst_curl_http_src_get_header);
	curl_easy_setopt(handle, CURLOPT_HEADERDATA, s);
	curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, gst_curl_http_src_get_chunks);
	curl_easy_setopt(handle, CURLOPT_WRITEDATA, s);

	GSTCURL_FUNCTION_EXIT(s);
	return handle;
}

/*
 * Add the GstCurlHttpSrc item to the queue and then wait until the curl thread
 * signals us to say that our request has completed.
 */
static gboolean
gst_curl_http_src_make_request(GstCurlHttpSrc *s)
{
	GstCurlHttpSrcQueueElement* element;
	gboolean ret = FALSE;
	GSTCURL_FUNCTION_ENTRY(s);

	s->result = GSTCURL_RETURN_NONE;
	if(s->curl_handle == NULL) {
		return ret;
	}
	g_mutex_lock(s->mutex);
	g_mutex_lock(request_queue_mutex);
	if (request_queue == NULL) {
		/* Queue is currently empty, so create a new item on the head */
		request_queue = g_malloc(sizeof(GstCurlHttpSrcQueueElement));
		if(request_queue == NULL) {
			GST_ERROR_OBJECT(s, "Couldn't allocate space for request queue!");
			return ret;
		}
		request_queue->p = s;
		request_queue->running = g_malloc(sizeof(GMutex));
		g_mutex_init(request_queue->running);
		request_queue->next = NULL;
	}
	else {
		element = request_queue;
		while(element->next != NULL) {
			element = element->next;
		}
		element->next = g_malloc(sizeof(GstCurlHttpSrcQueueElement));
		if(element->next == NULL) {
			GST_ERROR_OBJECT(s, "Couldn't allocate space for new queue element!");
			return ret;
		}
		element->next->p = s;
		element->next->running = g_malloc(sizeof(GMutex));
		g_mutex_init(element->next->running);
		element->next->next = NULL;
	}

	GST_DEBUG_OBJECT(s, "Submitting request for URI %s to curl", s->uri);

	/* Signal the worker thread */
	g_mutex_lock(curl_multi_loop_signal_mutex);
	curl_multi_loop_signal_state = GSTCURL_MULTI_LOOP_STATE_QUEUE_EVENT;
	g_cond_signal(curl_multi_loop_signaller);
	g_mutex_unlock(request_queue_mutex);
	g_mutex_unlock(curl_multi_loop_signal_mutex);

	g_cond_wait(s->finished, s->mutex);
	g_mutex_unlock(s->mutex);

	switch (s->result) {
	case GSTCURL_RETURN_NONE:
		GST_WARNING_OBJECT(s, "Nothing ever happened to our request for URI %s!",
				s->uri);
		break;
	case GSTCURL_RETURN_DONE:
		GST_DEBUG_OBJECT(s, "cURL call finished and returned for URI %s", s->uri);
		s->end_of_message = TRUE;
		ret = TRUE;
		break;
	case GSTCURL_RETURN_BAD_QUEUE_REQUEST:
		GST_WARNING_OBJECT(s, "cURL call for URI %s returned as a bad queue",
				s->uri);
		break;
	case GSTCURL_RETURN_TOTAL_ERROR:
		GST_ERROR_OBJECT(s, "cURL call for URI %s returned as a total failure",
				s->uri);
		break;
	case GSTCURL_RETURN_PIPELINE_NULL:
		GST_INFO_OBJECT(s,
			"Pipeline is cleaning up before request for URI %s could complete",
				s->uri);
		break;
	default:
		/* Why are we here? */
		GST_WARNING_OBJECT(s, "Shouldn't have reached here?");
	}

	GSTCURL_FUNCTION_EXIT(s);
	return ret;
}

/*
 * Destroy the easy handle created in the above function
 */
static inline void
gst_curl_http_src_destroy_easy_handle(CURL* handle)
{
	/* Thank you Handles, and well done. Well done, mate. */
	curl_easy_cleanup(handle);
}

/*
 * Function to get individual headers from curl response.
 */
static size_t
gst_curl_http_src_get_header(void *header, size_t size, size_t nmemb,
		GstCurlHttpSrc *s)
{
	char* substr;
	int i, len;
	/*
	 * All HTTP headers follow the same format.
	 * 	<<Identifier>>: <<Value>>
	 *
	 * So just parse for those!
	 */
	substr = strstr(header, "Content-Type: ");
	if(substr != NULL) {
		/*Length of stuff we don't need is 14 bytes*/
		substr += 14;
		len = (size*nmemb) - 14;
		if (s->headers.content_type != NULL) {
			GST_DEBUG_OBJECT(s, "Content Type header already present.");
			free(s->headers.content_type);
		}
		s->headers.content_type = malloc(sizeof(char) * (len + 1));
		if (s->headers.content_type == NULL) {
			GST_ERROR_OBJECT(s, "s->headers.content_type malloc failed!");
		}
		else {
			for(i = 0; i < len; i++) {
				/* For some reason, we get garbage characters at the end, so
				 * quick and dirty bit of stripping. We only want printing
				 * characters here. Also neatly null terminates! */
				if ((substr[i] >= 0x20) && (substr[i] < 0x7f))
					s->headers.content_type[i] = substr[i];
				else
					s->headers.content_type[i] = '\0';
			}
			GST_INFO_OBJECT(s, "Got Content-Type of %s",
					s->headers.content_type);
		}
	}
	return size*nmemb;
}

/*
 * Get chunks for currently running curl process.
 */
static size_t
gst_curl_http_src_get_chunks(void *chunk, size_t size, size_t nmemb,
		GstCurlHttpSrc *s)
{
    size_t new_len = s->len + size*nmemb;
    GST_TRACE_OBJECT(s, "Received curl chunk for URI %s of size %d, new total size %d",
            s->uri, (int) (size*nmemb), (int) new_len);
    s->msg = realloc(s->msg, (new_len + 1) * sizeof(char));
    if(s->msg == NULL) {
        GST_ERROR_OBJECT(s, "Realloc for cURL response message failed!\n");
        return 0;
    }
    memcpy(s->msg+s->len, chunk, size*nmemb);
    s->len = new_len;
    return size*nmemb;
}

static gboolean
gst_curl_http_src_signal_finished(CURL* handle, gint reason)
{
	gboolean ret, exit_cond = FALSE;
	GstCurlHttpSrcQueueElement *prev, *curr;
	/*
	 * Find the particular cURL instance that has just finished, signal the
	 * calling thread and then remove it from the list.
	 */
	prev = NULL;
	curr = request_queue;
	while(exit_cond != TRUE) {
		if(curr->p->curl_handle == handle) {
			curr->p->result = reason;
			g_mutex_unlock(curr->running);
			g_free(curr->running);
			g_cond_signal(curr->p->finished);
			/*g_mutex_unlock(curr->p->mutex);*/
			if(prev == NULL) {
				request_queue = curr->next;
			}
			else  {
				prev->next = curr->next;
				g_free(curr);
				curr = NULL;
			}
			exit_cond = TRUE;
			ret = TRUE;
		}
		else if (curr->next == NULL) {
			/*
			 * Reached the end of the queue without finding the element, return
			 * a failure.
			 */
			exit_cond = TRUE;
			ret = FALSE;
		}
		else {
			prev = curr;
			curr = curr->next;
		}
	}
	return ret;
}

static void
gst_curl_http_src_recurse_queue_cleanup(GstCurlHttpSrcQueueElement* element,
		gint reason)
{
	if(element->next != NULL) {
		gst_curl_http_src_recurse_queue_cleanup(element->next, reason);
	}
	/* Signal the calling thread, which should clean up the GstCurlHttpSrc */
	element->p->result = reason;
	g_cond_signal(element->p->finished);
	g_mutex_unlock(element->p->mutex);
	g_free(element);
	element = NULL;
}

static void
gst_curl_http_src_request_remove(GstCurlHttpSrc* src)
{
	g_mutex_lock(request_removal_mutex);
	g_mutex_lock(curl_multi_loop_signal_mutex);

	curl_multi_loop_signal_state = GSTCURL_MULTI_LOOP_STATE_REQUEST_REMOVAL;
	request_removal_element = src;
	g_cond_signal(curl_multi_loop_signaller);
	g_mutex_unlock(curl_multi_loop_signal_mutex);
	/* The following should be unlocked by the thread... */
	/*g_mutex_unlock(request_removal_mutex);*/
}

/*****************************************************************************
 * Curl loop task functions end
 *****************************************************************************/

/* entry point to initialize the plug-in
 * initialize the plug-in itself
 * register the element factories and other features
 */
static gboolean
curlhttpsrc_init (GstPlugin * curlhttpsrc)
{
  /* debug category for fltering log messages
   *
   * exchange the string 'Template curlhttpsrc' with your description
   */
  GST_DEBUG_CATEGORY_INIT (gst_curl_http_src_debug, "curlhttpsrc",
      0, "UriHandler for libcURL");

  /* Set to 500 so we take precedence over soup for dev purposes. */
  return gst_element_register (curlhttpsrc, "curlhttpsrc", 500,
      GST_TYPE_CURLHTTPSRC);
}

/* PACKAGE: this is usually set by autotools depending on some _INIT macro
 * in configure.ac and then written into and defined in config.h, but we can
 * just set it ourselves here in case someone doesn't use autotools to
 * compile this code. GST_PLUGIN_DEFINE needs PACKAGE to be defined.
 */
#ifndef PACKAGE
#define PACKAGE "CurlHttpSrc"
#endif

GST_PLUGIN_DEFINE (
    GST_VERSION_MAJOR,
    GST_VERSION_MINOR,
    curlhttpsrc,
    "UriHandler for libcURL",
    curlhttpsrc_init,
    VERSION,
    "LGPL",
    "BBC Research & Development",
    "http://www.bbc.co.uk/rd"
)
