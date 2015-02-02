plugin_LTLIBRARIES = libcurlhttpsrc.la

# sources used to compile this plug-in
libcurlhttpsrc_la_SOURCES = gstcurlhttpsrc.c gstcurlqueue.c gstcurlhttpsrc.h \
                            curltask.h gstcurldefaults.h gstcurlqueue.h

# compiler and linker flags used to compile this plugin, set in configure.ac
libcurlhttpsrc_la_CFLAGS = $(GST_CFLAGS)
libcurlhttpsrc_la_LIBADD = $(GST_LIBS) -lcurl
libcurlhttpsrc_la_LDFLAGS = $(GST_PLUGIN_LDFLAGS)
libcurlhttpsrc_la_LIBTOOLFLAGS = --tag=disable-static
