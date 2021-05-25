plugin_LTLIBRARIES = libgstrkv4l2.la

include $(top_srcdir)/common/gst-glib-gen.mak

libgstrkv4l2_la_SOURCES = gstrkv4l2.c \
				gstv4l2allocator.c \
				gstv4l2object.c \
				gstv4l2bufferpool.c \
				gstrkv4l2src.c \
				gstv4l2transform.c \
				ext/mediactl.c \
				common.c \
				media-controller.c \
				v4l2_calls.c \
				v4l2-utils.c

libgstrkv4l2_la_CFLAGS =   $(GST_PLUGINS_BASE_CFLAGS) \
				 $(GST_BASE_CFLAGS) \
				 $(GST_CFLAGS) \
				 $(X_CFLAGS) \
				 $(LIBV4L2_CFLAGS) \
				 $(GUDEV_CFLAGS)

libgstrkv4l2_la_LIBADD =   $(GST_PLUGINS_BASE_LIBS) \
				 -lgstallocators-$(GST_API_VERSION) \
				 -lgstvideo-$(GST_API_VERSION) \
				 $(GST_BASE_LIBS) \
				 $(GST_LIBS) \
				 $(LIBV4L2_LIBS) \
				 $(GUDEV_LIBS) \
				 $(LIBRT) \
				 -lrkisp

noinst_HEADERS = \
	ext/types-compat.h \
	ext/v4l2-common.h \
	ext/v4l2-controls.h \
	ext/videodev2.h \
	ext/mediactl.h \
	ext/mediactl-priv.h \
	ext/tools.h \
	ext/v4l2subdev.h \
	gstv4l2allocator.h \
	gstv4l2bufferpool.h \
	gstv4l2object.h \
	gstrkv4l2src.h \
	gstv4l2transform.h \
	common.h \
	media-controller.h \
	v4l2-utils.h
