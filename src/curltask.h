/*
 * curltask.h
 *
 *  Created on: 29 Oct 2014
 *      Author: sam
 */

#ifndef CURLTASK_H_
#define CURLTASK_H_

#include <curl/curl.h>
#include "gstcurldefaults.h"

GST_DEBUG_CATEGORY_STATIC (gst_curl_loop_debug);

#define GSTCURL_ERROR_PRINT(...) gst_debug_log (gst_curl_loop_debug, GST_LEVEL_ERROR, __FILE__, __func__, __LINE__, NULL, __VA_ARGS__)
#define GSTCURL_WARNING_PRINT(...) gst_debug_log (gst_curl_loop_debug, GST_LEVEL_WARNING, __FILE__, __func__, __LINE__, NULL, __VA_ARGS__)
#define GSTCURL_INFO_PRINT(...) gst_debug_log (gst_curl_loop_debug, GST_LEVEL_INFO, __FILE__, __func__, __LINE__, NULL, __VA_ARGS__)
#define GSTCURL_DEBUG_PRINT(...) gst_debug_log (gst_curl_loop_debug, GST_LEVEL_DEBUG, __FILE__, __func__, __LINE__, NULL, __VA_ARGS__)
#define GSTCURL_TRACE_PRINT(...) gst_debug_log (gst_curl_loop_debug, GST_LEVEL_TRACE, __FILE__, __func__, __LINE__, NULL, __VA_ARGS__)

#define gst_curl_setopt_str(handle,type,option) \
	if(option != NULL) { \
		curl_easy_setopt(handle,type,option); \
	} \

#define gst_curl_setopt_int(handle, type, option) \
	if((option > GSTCURL_HANDLE_MIN_##type) && (option < GSTCURL_HANDLE_MAX_##type)) { \
		curl_easy_setopt(handle,type,option); \
	} \

#define gst_curl_setopt_str_default(handle,type,option) \
	if(option == NULL) { \
		curl_easy_setopt(handle,type,GSTCURL_HANDLE_DEFAULT_##type); \
	} \
	else { \
		curl_easy_setopt(handle,type,option); \
	} \

#define gst_curl_setopt_int_default(handle,type,option) \
	if((option < GSTCURL_HANDLE_MIN_##type) || (option > GSTCURL_HANDLE_MAX_##type)) { \
		curl_easy_setopt(handle,type,GSTCURL_HANDLE_DEFAULT_##type); \
	} \
	else { \
		curl_easy_setopt(handle,type,option); \
	} \

#define GSTCURL_ASSERT_MUTEX(x) if(g_atomic_pointer_get(&x->p) == NULL) GSTCURL_DEBUG_PRINT("ASSERTION: No valid mutex handle in GMutex %p", x);

struct {

} GstCurlThreadData;

/*
 * Function definitions
 */

#endif /* CURLTASK_H_ */
