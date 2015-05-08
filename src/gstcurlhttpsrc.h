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
typedef struct _GstCurlHttpSrcMultiTaskContext GstCurlHttpSrcMultiTaskContext;
typedef struct _GstCurlHttpSrcQueueElement GstCurlHttpSrcQueueElement;

struct _GstCurlHttpSrcMultiTaskContext
{
  GstTask     *task;
  GRecMutex   task_rec_mutex;
  GMutex      mutex;
  guint       refcount;
  GCond       signal;

  GstCurlHttpSrc  *request_removal_element;

  GstCurlHttpSrcQueueElement  *queue;

  enum
  {
    GSTCURL_MULTI_LOOP_STATE_WAIT = 0,
    GSTCURL_MULTI_LOOP_STATE_QUEUE_EVENT,
    GSTCURL_MULTI_LOOP_STATE_RUNNING,
    GSTCURL_MULTI_LOOP_STATE_REQUEST_REMOVAL,
    GSTCURL_MULTI_LOOP_STATE_STOP,
    GSTCURL_MULTI_LOOP_STATE_MAX
  } state;

  /* < private > */
  CURLM *multi_handle;
};

struct _GstCurlHttpSrcClass
{
  GstPushSrcClass parent_class;

  GstCurlHttpSrcMultiTaskContext multi_task_context;
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
  gchar *redirect_uri;		/* CURLINFO_REDIRECT_URL */
  gchar *username;              /* CURLOPT_USERNAME */
  gchar *password;              /* CURLOPT_PASSWORD */
  gchar *proxy_uri;             /* CURLOPT_PROXY */
  gchar *no_proxy_list;         /* CURLOPT_NOPROXY */
  gchar *proxy_user;            /* CURLOPT_PROXYUSERNAME */
  gchar *proxy_pass;            /* CURLOPT_PROXYPASSWORD */

  /* Header options */
  gchar **cookies;              /* CURLOPT_COOKIELIST */
  gint number_cookies;
  gchar *user_agent;            /* CURLOPT_USERAGENT */
  gchar **extra_headers;        /* CURLOPT_HTTPHEADER */
  gint number_headers;
  struct curl_slist *slist;
  gboolean accept_compressed_encodings; /* CURLOPT_ACCEPT_ENCODING */

  /* Connection options */
  glong allow_3xx_redirect;     /* CURLOPT_FOLLOWLOCATION */
  glong max_3xx_redirects;      /* CURLOPT_MAXREDIRS */
  gboolean keep_alive;          /* CURLOPT_TCP_KEEPALIVE */
  gint timeout_secs;            /* CURLOPT_TIMEOUT */
  gboolean strict_ssl;		/* CURLOPT_SSL_VERIFYPEER */
  gchar* custom_ca_file;	/* CURLOPT_CAINFO */

  gint total_retries;
  gint retries_remaining;

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
#ifdef CURL_VERSION_HTTP2
    GSTCURL_HTTP_VERSION_2_0,
#endif
    GSTCURL_HTTP_NOT,           /* For future use, incase not HTTP protocol! */
    GSTCURL_HTTP_VERSION_MAX
  } preferred_http_version;     /* CURLOPT_HTTP_VERSION */

  GCond *finished;
  enum
  {
    GSTCURL_RETURN_NONE,
    GSTCURL_RETURN_DONE,
    GSTCURL_RETURN_REMOVED,
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
  guint64 content_length;
  struct
  {
    gchar *content_type;
  } headers;

  GstCaps *caps;
};

enum
{
  PROP_0,
  PROP_URI,
  PROP_USERNAME,
  PROP_PASSWORD,
  PROP_PROXYURI,
  PROP_PROXYUSERNAME,
  PROP_PROXYPASSWORD,
  PROP_COOKIES,
  PROP_USERAGENT,
  PROP_HEADERS,
  PROP_COMPRESS,
  PROP_REDIRECT,
  PROP_MAXREDIRECT,
  PROP_KEEPALIVE,
  PROP_TIMEOUT,
  PROP_STRICT_SSL,
  PROP_SSL_CA_FILE,
  PROP_RETRIES,
  PROP_CONNECTIONMAXTIME,
  PROP_MAXCONCURRENT_SERVER,
  PROP_MAXCONCURRENT_PROXY,
  PROP_MAXCONCURRENT_GLOBAL,
  PROP_HTTPVERSION,
  PROP_MAX
};

curl_version_info_data *gst_curl_http_src_curl_capabilities;
gfloat pref_http_ver;
gchar *gst_curl_http_src_default_useragent;

G_END_DECLS
#endif /* GSTCURLHTTPSRC_H_ */
