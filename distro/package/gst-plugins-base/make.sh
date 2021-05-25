#!/bin/bash

set -e
DEPENDENCIES="gstreamer"
PKG=gst-plugins-base
VERSION=1.14.4
source $OUTPUT_DIR/.config
if [ ! -e $DOWNLOAD_DIR/$PKG-$VERSION.tar.xz ];then
	wget -P $DOWNLOAD_DIR https://gstreamer.freedesktop.org/src/gst-plugins-base/$PKG-$VERSION.tar.xz
fi

if [ ! -d $BUILD_DIR/$PKG/$PKG-$VERSION ];then
	tar -xf $DOWNLOAD_DIR/$PKG-$VERSION.tar.xz -C $BUILD_DIR/$PKG
	mv $BUILD_DIR/$PKG/$PKG-$VERSION/* $BUILD_DIR/$PKG/
fi

cd $BUILD_DIR/$PKG
OPTS="--target=aarch64-linux-gnu --host=aarch64-linux-gnu --prefix=/usr --libdir=/usr/lib/$TOOLCHAIN --disable-gtk-doc --disable-gtk-doc-html --disable-dependency-tracking --disable-nls --disable-static --enable-shared  --disable-examples --disable-valgrind --disable-introspection --disable-cdparanoia --disable-libvisual --disable-iso-codes"

if [ x$BR2_PACKAGE_GST_PLUGINS_BASE_OPENGL = xy ];then
	OPTS="$OPTS --enable-opengl"
else
	OPTS="$OPTS --disable-opengl"
fi

if [ x$BR2_PACKAGE_GST_PLUGINS_BASE_GLES2 = xy ];then
	OPTS="$OPTS --enable-gles2"
else
	OPTS="$OPTS --disable-gles2"
fi

if [ x$BR2_PACKAGE_GST_PLUGINS_BASE_EGL = xy ];then
	OPTS="$OPTS --enable-egl"
else
	OPTS="$OPTS --disable-egl"
fi

if [ x$BR2_PACKAGE_GST_PLUGINS_BASE_WGL = xy ];then
	OPTS="$OPTS --enable-wgl"
else
	OPTS="$OPTS --disable-wgl"
fi

if [ x$BR2_PACKAGE_GST_PLUGINS_BASE_GLX = xy ];then
	OPTS="$OPTS --enable-glx"
else
	OPTS="$OPTS --disable-glx"
fi

if [ x$BR2_PACKAGE_GST_PLUGINS_BASE_COCOA = xy ];then
	OPTS="$OPTS --enable-cocoa"
else
	OPTS="$OPTS --disable-cocoa"
fi

if [ x$BR2_PACKAGE_GST_PLUGINS_BASE_X11 = xy ];then
	OPTS="$OPTS --enable-x11"
else
	OPTS="$OPTS --disable-x11"
fi

if [ x$BR2_PACKAGE_GST_PLUGINS_BASE_WAYLAND = xy ];then
	OPTS="$OPTS --enable-wayland"
else
	OPTS="$OPTS --disable-wayland"
fi

if [ x$BR2_PACKAGE_GST_PLUGINS_BASE_DISPMANX = xy ];then
	OPTS="$OPTS --enable-dispmanx"
else
	OPTS="$OPTS --disable-dispmanx"
fi

if [ x$BR2_PACKAGE_GST_PLUGINS_BASE_BSYMBOLIC = xy ];then
	OPTS="$OPTS --enable-Bsymbolic"
else
	OPTS="$OPTS --disable-Bsymbolic"
fi

if [ x$BR2_PACKAGE_GST_PLUGINS_BASE_ADDER = xy ];then
	OPTS="$OPTS --enable-adder"
else
	OPTS="$OPTS --disable-adder"
fi

if [ x$BR2_PACKAGE_GST_PLUGINS_BASE_APP = xy ];then
	OPTS="$OPTS --enable-app"
else
	OPTS="$OPTS --disable-app"
fi

if [ x$BR2_PACKAGE_GST_PLUGINS_BASE_AUDIOCONVERT = xy ];then
	OPTS="$OPTS --enable-audioconvert"
else
	OPTS="$OPTS --disable-audioconvert"
fi

if [ x$BR2_PACKAGE_GST_PLUGINS_BASE_AUDIOMIXER = xy ];then
	OPTS="$OPTS --enable-audiomixer"
else
	OPTS="$OPTS --disable-audiomixer"
fi

if [ x$BR2_PACKAGE_GST_PLUGINS_BASE_AUDIORATE = xy ];then
	OPTS="$OPTS --enable-audiorate"
else
	OPTS="$OPTS --disable-audiorate"
fi

if [ x$BR2_PACKAGE_GST_PLUGINS_BASE_AUDIOTESTSRC = xy ];then
	OPTS="$OPTS --enable-audiotestsrc"
else
	OPTS="$OPTS --disable-audiotestsrc"
fi

if [ x$BR2_PACKAGE_GST_PLUGINS_BASE_ENCODING = xy ];then
	OPTS="$OPTS --enable-encoding"
else
	OPTS="$OPTS --disable-encoding"
fi

if [ x$BR2_PACKAGE_GST_PLUGINS_BASE_VIDEOCONVERT = xy ];then
	OPTS="$OPTS --enable-videoconvert"
else
	OPTS="$OPTS --disable-videoconvert"
fi

if [ x$BR2_PACKAGE_GST_PLUGINS_BASE_GIO = xy ];then
	OPTS="$OPTS --enable-gio"
else
	OPTS="$OPTS --disable-gio"
fi

if [ x$BR2_PACKAGE_GST_PLUGINS_BASE_PLAYBACK = xy ];then
	OPTS="$OPTS --enable-playback"
else
	OPTS="$OPTS --disable-playback"
fi

if [ x$BR2_PACKAGE_GST_PLUGINS_BASE_RESAMPLE = xy ];then
	OPTS="$OPTS --enable-audioresample"
else
	OPTS="$OPTS --disable-audioresample"
fi

if [ x$BR2_PACKAGE_GST_PLUGINS_BASE_RAWPARSE = xy ];then
	OPTS="$OPTS --enable-rawparse"
else
	OPTS="$OPTS --disable-rawparse"
fi

if [ x$BR2_PACKAGE_GST_PLUGINS_BASE_SUBPARSE = xy ];then
	OPTS="$OPTS --enable-subparse"
else
	OPTS="$OPTS --disable-subparse"
fi

if [ x$BR2_PACKAGE_GST_PLUGINS_BASE_TCP = xy ];then
	OPTS="$OPTS --enable-tcp"
else
	OPTS="$OPTS --disable-tcp"
fi

if [ x$BR2_PACKAGE_GST_PLUGINS_BASE_TYPEFIND = xy ];then
	OPTS="$OPTS --enable-typefind"
else
	OPTS="$OPTS --disable-typefind"
fi

if [ x$BR2_PACKAGE_GST_PLUGINS_BASE_VIDEOTESTSRC = xy ];then
	OPTS="$OPTS --enable-videotestsrc"
else
	OPTS="$OPTS --disable-videotestsrc"
fi

if [ x$BR2_PACKAGE_GST_PLUGINS_BASE_VIDEORATE = xy ];then
	OPTS="$OPTS --enable-videorate"
else
	OPTS="$OPTS --disable-videorate"
fi

if [ x$BR2_PACKAGE_GST_PLUGINS_BASE_VIDEOSCALE = xy ];then
	OPTS="$OPTS --enable-videoscale"
else
	OPTS="$OPTS --disable-videoscale"
fi

if [ x$BR2_PACKAGE_GST_PLUGINS_BASE_VOLUME = xy ];then
	OPTS="$OPTS --enable-volume"
else
	OPTS="$OPTS --disable-volume"
fi

if [ x$BR2_PACKAGE_GST_PLUGINS_BASE_ISO_CODES = xy ];then
	OPTS="$OPTS --enable-iso-codes"
else
	OPTS="$OPTS --disable-iso-codes"
fi

if [ x$BR2_PACKAGE_GST_PLUGINS_BASE_ZLIB = xy ];then
	OPTS="$OPTS --enable-zlib"
else
	OPTS="$OPTS --disable-zlib"
fi

if [ x$BR2_PACKAGE_GST_PLUGINS_BASE_X = xy ];then
	OPTS="$OPTS --enable-x"
else
	OPTS="$OPTS --disable-x"
fi

if [ x$BR2_PACKAGE_GST_PLUGINS_BASE_XVIDEO = xy ];then
	OPTS="$OPTS --enable-xvideo"
else
	OPTS="$OPTS --disable-xvideo"
fi

if [ x$BR2_PACKAGE_GST_PLUGINS_BASE_XSHM = xy ];then
	OPTS="$OPTS --enable-xshm"
else
	OPTS="$OPTS --disable-xshm"
fi

if [ x$BR2_PACKAGE_GST_PLUGINS_BASE_ALSA = xy ];then
	OPTS="$OPTS --enable-alsa"
	DEPENDENCIES="$DEPENDENCIES libasound2-dev"
else
	OPTS="$OPTS --disable-alsa"
fi

if [ x$BR2_PACKAGE_GST_PLUGINS_BASE_CDPARANOIA = xy ];then
	OPTS="$OPTS --enable-cdparanoia"
else
	OPTS="$OPTS --disable-cdparanoia"
fi

if [ x$BR2_PACKAGE_GST_PLUGINS_BASE_GL = xy ];then
	OPTS="$OPTS --enable-gl"
else
	OPTS="$OPTS --disable-gl"
fi

if [ x$BR2_PACKAGE_GST_PLUGINS_BASE_IVORBIS = xy ];then
	OPTS="$OPTS --enable-ivorbis"
else
	OPTS="$OPTS --disable-ivorbis"
fi

if [ x$BR2_PACKAGE_GST_PLUGINS_BASE_LIBVISUAL = xy ];then
	OPTS="$OPTS --enable-libvisual"
else
	OPTS="$OPTS --disable-libvisual"
fi

if [ x$BR2_PACKAGE_GST_PLUGINS_BASE_OGG = xy ];then
	OPTS="$OPTS --enable-ogg"
else
	OPTS="$OPTS --disable-ogg"
fi
if [ x$BR2_PACKAGE_GST_PLUGINS_BASE_OPUS = xy ];then
	OPTS="$OPTS --enable-opus"
else
	OPTS="$OPTS --disable-opus"
fi

if [ x$BR2_PACKAGE_GST_PLUGINS_BASE_PANGO = xy ];then
	OPTS="$OPTS --enable-pango"
else
	OPTS="$OPTS --disable-pango"
fi

if [ x$BR2_PACKAGE_GST_PLUGINS_BASE_THEORA = xy ];then
	OPTS="$OPTS --enable-theora"
else
	OPTS="$OPTS --disable-theora"
fi

if [ x$BR2_PACKAGE_GST_PLUGINS_BASE_VORBIS = xy ];then
	OPTS="$OPTS --enable-vorbis"
else
	OPTS="$OPTS --disable-vorbis"
fi
$SCRIPTS_DIR/build_pkgs.sh $ARCH $SUITE "$DEPENDENCIES"
echo "opts: $OPTS"
./configure $OPTS
make
make install
$SCRIPTS_DIR/fixlibtool.sh $TARGET_DIR $TARGET_DIR
cd -
