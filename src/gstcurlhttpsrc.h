/*
 * GstCurlHttpSrc
 * Copyright 2014 British Broadcasting Corporation - Research and Development
 *
 * Author: Sam Hurst <samuelh@rd.bbc.co.uk>
 *
 * Based on the GstElement template, courtesy of
 * Copyright (C) 2005 Thomas Vander Stichele <thomas@apestaart.org>
 * Copyright (C) 2005 Ronald S. Bultje <rbultje@ronald.bitfreak.net>
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

/*
 * This header file contains definitions only for
 */

#ifndef GSTCURLHTTPSRC_H_
#define GSTCURLHTTPSRC_H_

#include <gst/gst.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <unistd.h>
#include <gst/base/gstpushsrc.h>

#include "curltask.h"

G_BEGIN_DECLS
/* #defines don't like whitespacey bits */
#define GST_TYPE_CURLHTTPSRC \
  (gst_curl_http_src_get_type())
#define GST_CURLHTTPSRC(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_CURLHTTPSRC,GstCurlHttpSrc))
#define GST_CURLHTTPSRC_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_CURLHTTPSRC,GstCurlHttpSrcClass))
#define GST_IS_CURLHTTPSRC(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_CURLHTTPSRC))
#define GST_IS_CURLHTTPSRC_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_CURLHTTPSRC))
/* Because g_param_spec_int requires min/max bounding... */
#define GSTCURL_MIN_REDIRECTIONS -1
#define GSTCURL_MAX_REDIRECTIONS 255
#define GSTCURL_MIN_CONNECTION_TIME 2
#define GSTCURL_MAX_CONNECTION_TIME 60
#define GSTCURL_MIN_CONNECTIONS_SERVER 1
#define GSTCURL_MAX_CONNECTIONS_SERVER 60
#define GSTCURL_MIN_CONNECTIONS_PROXY 1
#define GSTCURL_MAX_CONNECTIONS_PROXY 60
#define GSTCURL_MIN_CONNECTIONS_GLOBAL 1
#define GSTCURL_MAX_CONNECTIONS_GLOBAL 255
#define GSTCURL_DEFAULT_CONNECTION_TIME 30
#define GSTCURL_DEFAULT_CONNECTIONS_SERVER 5
#define GSTCURL_DEFAULT_CONNECTIONS_PROXY 30
#define GSTCURL_DEFAULT_CONNECTIONS_GLOBAL 255
#define GSTCURL_INFO_RESPONSE(x) ((x >= 100) && (x <= 199))
#define GSTCURL_SUCCESS_RESPONSE(x) ((x >= 200) && (x <=299))
#define GSTCURL_REDIRECT_RESPONSE(x) ((x >= 300) && (x <= 399))
#define GSTCURL_CLIENT_ERR_RESPONSE(x) ((x >= 400) && (x <= 499))
#define GSTCURL_SERVER_ERR_RESPONSE(x) ((x >= 500) && (x <= 599))
#define GSTCURL_FUNCTIONTRACE 0
#if GSTCURL_FUNCTIONTRACE
#define GSTCURL_FUNCTION_ENTRY(x) GST_DEBUG_OBJECT(x, "Entering function");
#define GSTCURL_FUNCTION_EXIT(x) GST_DEBUG_OBJECT(x, "Leaving function");
#else
#define GSTCURL_FUNCTION_ENTRY(x)
#define GSTCURL_FUNCTION_EXIT(x)
#endif
typedef struct _GstCurlHttpSrc GstCurlHttpSrc;
typedef struct _GstCurlHttpSrcClass GstCurlHttpSrcClass;
typedef struct _GstCurlHttpSrcQueueElement GstCurlHttpSrcQueueElement;

struct _GstCurlHttpSrcClass
{
  GstPushSrcClass parent_class;
};

/*
 * Our instance class.
 */
struct _GstCurlHttpSrc
{
  GstPushSrc element;

  /* < private > */
  GMutex *uri_mutex; /* Make the URIHandler get/set thread safe */
  /*
   * Things to tell libcURL about to build up the request message.
   */
  /* Type         Name                                      Curl Option */
  gchar *uri;                   /* CURLOPT_URL */
  gchar *proxy_uri;             /* CURLOPT_PROXY */
  gchar *no_proxy_list;         /* CURLOPT_NOPROXY */
  gchar *proxy_user;            /* CURLOPT_PROXYUSERNAME */
  gchar *proxy_pass;            /* CURLOPT_PROXYPASSWORD */

  gchar **cookies;              /* CURLOPT_COOKIELIST */
  gint number_cookies;
  gchar *user_agent;            /* CURLOPT_USERAGENT */
  glong allow_3xx_redirect;     /* CURLOPT_FOLLOWLOCATION */
  glong max_3xx_redirects;      /* CURLOPT_MAXREDIRS */
  gboolean keep_alive;          /* CURLOPT_TCP_KEEPALIVE */
  /*TODO As the following are all multi options, move these to curl task */
  guint max_connection_time;    /* */
  guint max_conns_per_server;   /* CURLMOPT_MAX_HOST_CONNECTIONS */
  guint max_conns_per_proxy;    /* ?!? */
  guint max_conns_global;       /* CURLMOPT_MAXCONNECTS */
  /* END multi options */

  /* Some stuff for HTTP/2 */
  enum
  {
    GSTCURL_HTTP_VERSION_1_0,
    GSTCURL_HTTP_VERSION_1_1,
    GSTCURL_HTTP_VERSION_2_0,
    GSTCURL_HTTP_NOT,           /* For future use, incase not HTTP protocol! */
    GSTCURL_HTTP_VERSION_MAX
  } preferred_http_version;     /* CURLOPT_HTTP_VERSION */

  /*
   * Mutex for the curl task to hold while it's working.
   *
   * It seems we need to add two mutexes, because the calling thread isn't
   * fast enough to execute before the worker thread attempts to do its thing
   * and so everything goes pear shaped.
   */
  GMutex *mutex;
  GCond *finished;
  enum
  {
    GSTCURL_RETURN_NONE,
    GSTCURL_RETURN_DONE,
    GSTCURL_RETURN_BAD_QUEUE_REQUEST,
    GSTCURL_RETURN_TOTAL_ERROR,
    GSTCURL_RETURN_PIPELINE_NULL,
    GSTCURL_RETURN_MAX
  } result;
  CURL *curl_handle;
  gboolean end_of_message;

  /*
   * Response message
   */
  gchar *msg;
  guint len;
  struct
  {
    gchar *content_type;
  } headers;

  GstCaps *caps;
};

struct _GstCurlHttpSrcQueueElement
{
  GstCurlHttpSrc *p;
  GstCurlHttpSrcQueueElement *next;
  GMutex *running;
};

static GstCurlHttpSrcQueueElement *request_queue;
static GMutex *request_queue_mutex;

static GCond *curl_multi_loop_signaller;
static GMutex *curl_multi_loop_signal_mutex;
static enum
{
  GSTCURL_MULTI_LOOP_STATE_WAIT = 0,
  GSTCURL_MULTI_LOOP_STATE_QUEUE_EVENT,
  GSTCURL_MULTI_LOOP_STATE_RUNNING,
  GSTCURL_MULTI_LOOP_STATE_REQUEST_REMOVAL,
  GSTCURL_MULTI_LOOP_STATE_STOP,
  GSTCURL_MULTI_LOOP_STATE_MAX
} curl_multi_loop_signal_state;

static GMutex GstCurlHttpSrcLoopRefcountMutex;
static guint GstCurlHttpSrcLoopRefcount;

static GstTask *GstCurlHttpSrcLoopTask;
static GRecMutex GstCurlHttpSrcLoopRecMutex;
static GCond GstCurlHttpSrcLoopReadyCond;
static GMutex GstCurlHttpSrcLoopReadyMutex;

static GMutex *request_removal_mutex;
static GstCurlHttpSrc *request_removal_element;

enum
{
  PROP_0,
  PROP_URI,
  PROP_PROXYURI,
  PROP_PROXYUSERNAME,
  PROP_PROXYPASSWORD,
  PROP_COOKIES,
  PROP_USERAGENT,
  PROP_REDIRECT,
  PROP_MAXREDIRECT,
  PROP_KEEPALIVE,
  PROP_CONNECTIONMAXTIME,
  PROP_MAXCONCURRENT_SERVER,
  PROP_MAXCONCURRENT_PROXY,
  PROP_MAXCONCURRENT_GLOBAL,
  PROP_HTTPVERSION,
  PROP_MAX
};

static curl_version_info_data *gst_curl_http_src_curl_capabilities;
static gfloat pref_http_ver;
static gchar *gst_curl_http_src_default_useragent;

/*
 * Function Definitions
 */
/* Gstreamer generic element functions */
static void gst_curl_http_src_class_init (GstCurlHttpSrcClass * klass);
static void gst_curl_http_src_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec);
static void gst_curl_http_src_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec);
static void gst_curl_http_src_init (GstCurlHttpSrc * source);
static GstFlowReturn gst_curl_http_src_create (GstPushSrc * psrc,
    GstBuffer ** outbuf);
static GstFlowReturn
gst_curl_http_src_handle_response (GstCurlHttpSrc * src, GstBuffer ** buf);
static gboolean gst_curl_http_src_negotiate_caps (GstCurlHttpSrc * src);
static GstStateChangeReturn gst_curl_http_src_change_state (GstElement *
    element, GstStateChange transition);
static void gst_curl_http_src_cleanup_instance(GstCurlHttpSrc *src);

/* URI Handler functions */
static void gst_curl_http_src_uri_handler_init (gpointer g_iface,
    gpointer iface_data);
static guint gst_curl_http_src_urihandler_get_type (GType type);
static const gchar *const *gst_curl_http_src_urihandler_get_protocols (GType
    type);
static gchar *gst_curl_http_src_urihandler_get_uri (GstURIHandler * handler);
static gboolean gst_curl_http_src_urihandler_set_uri (GstURIHandler * handler,
    const gchar * uri, GError ** error);

/* GstTask functions */
static void gst_curl_http_src_curl_multi_loop (gpointer thread_data);
static CURL *gst_curl_http_src_create_easy_handle (GstCurlHttpSrc * s);
static gboolean gst_curl_http_src_make_request (GstCurlHttpSrc * s);
static inline void gst_curl_http_src_destroy_easy_handle (CURL * handle);
static size_t gst_curl_http_src_get_header (void *header, size_t size,
    size_t nmemb, GstCurlHttpSrc * s);
static size_t gst_curl_http_src_get_chunks (void *chunk, size_t size,
    size_t nmemb, GstCurlHttpSrc * s);
static gboolean gst_curl_http_src_signal_finished (CURL * handle, gint reason);
static void inline
gst_curl_http_src_recurse_queue_cleanup (GstCurlHttpSrcQueueElement * element,
    gint reason);
static void gst_curl_http_src_request_remove (GstCurlHttpSrc * src);

static char *gst_curl_http_src_strcasestr (const char *haystack,
    const char *needle);

G_END_DECLS
#endif /* GSTCURLHTTPSRC_H_ */
