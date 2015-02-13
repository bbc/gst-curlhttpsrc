plugin_LTLIBRARIES = libgstcurlhttpsrc.la

# sources used to compile this plug-in
libgstcurlhttpsrc_la_SOURCES = gstcurlhttpsrc.c gstcurlqueue.c gstcurlhttpsrc.h \
                            curltask.h gstcurldefaults.h gstcurlqueue.h

# compiler and linker flags used to compile this plugin, set in configure.ac
libgstcurlhttpsrc_la_CFLAGS = $(GST_CFLAGS)
libgstcurlhttpsrc_la_LIBADD = $(GST_LIBS) -lcurl
libgstcurlhttpsrc_la_LDFLAGS = $(GST_PLUGIN_LDFLAGS)
libgstcurlhttpsrc_la_LIBTOOLFLAGS = --tag=disable-static
