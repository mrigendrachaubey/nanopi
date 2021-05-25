#!/bin/bash

set -e
DEPENDENCIES="gst-plugins-base"
PKG=gst-plugins-good
VERSION=1.14.4
source $OUTPUT_DIR/.config
if [ ! -e $DOWNLOAD_DIR/$PKG-$VERSION.tar.xz ];then
	wget -P $DOWNLOAD_DIR https://gstreamer.freedesktop.org/src/gst-plugins-good/$PKG-$VERSION.tar.xz
fi

if [ ! -d $BUILD_DIR/$PKG/$PKG-$VERSION ];then
	tar -xf $DOWNLOAD_DIR/$PKG-$VERSION.tar.xz -C $BUILD_DIR/$PKG
	mv $BUILD_DIR/$PKG/$PKG-$VERSION/* $BUILD_DIR/$PKG/
fi

cd $BUILD_DIR/$PKG
OPTS="--target=aarch64-linux-gnu --host=aarch64-linux-gnu --prefix=/usr --libdir=/usr/lib/$TOOLCHAIN --disable-gtk-doc --disable-gtk-doc-html --disable-dependency-tracking --disable-nls --disable-static --enable-shared  --disable-valgrind --disable-examples"


if [ x$BR2_PACKAGE_GST_PLUGINS_GOOD_ALPHA = xy ];then
	OPTS="$OPTS --ebable-alpha"
else
	OPTS="$OPTS --disable-alpha"
fi

if [ x$BR2_PACKAGE_GST_PLUGINS_GOOD_APETAG = xy ];then
	OPTS="$OPTS --enable-apetag"
else
	OPTS="$OPTS --disable-apetag"
fi

if [ x$BR2_PACKAGE_GST_PLUGINS_GOOD_AUDIOFX = xy ];then
	OPTS="$OPTS --enable-audiofx"
else
	OPTS="$OPTS --disable-audiofx"
fi

if [ x$BR2_PACKAGE_GST_PLUGINS_GOOD_AUDIOPARSERS = xy ];then
	OPTS="$OPTS --enable-audioparsers"
else
	OPTS="$OPTS --disable-audioparsers"
fi

if [ x$BR2_PACKAGE_GST_PLUGINS_GOOD_AUPARSE = xy ];then
	OPTS="$OPTS --enable-auparse"
else
	OPTS="$OPTS --disable-auparse"
fi

if [ x$BR2_PACKAGE_GST_PLUGINS_GOOD_AUTODETECT = xy ];then
	OPTS="$OPTS --enable-autodetect"
else
	OPTS="$OPTS --disable-autodetect"
fi

if [ x$BR2_PACKAGE_GST_PLUGINS_GOOD_AVI = xy ];then
	OPTS="$OPTS --enable-avi"
else
	OPTS="$OPTS --disable-avi"
fi

if [ x$BR2_PACKAGE_GST_PLUGINS_GOOD_CUTTER = xy ];then
	OPTS="$OPTS --enable-cutter"
else
	OPTS="$OPTS --disable-cutter"
fi

if [ x$BR2_PACKAGE_GST_PLUGINS_GOOD_DEBUGUTILS = xy ];then
	OPTS="$OPTS --enable-debugutils"
else
	OPTS="$OPTS --disable-debugutils"
fi

if [ x$BR2_PACKAGE_GST_PLUGINS_GOOD_DEINTERLACE = xy ];then
	OPTS="$OPTS --enable-deinterlace"
else
	OPTS="$OPTS --disable-deinterlace"
fi

if [ x$BR2_PACKAGE_GST_PLUGINS_GOOD_DTMF = xy ];then
	OPTS="$OPTS --enable-dtmf"
else
	OPTS="$OPTS --disable-dtmf"
fi

if [ x$BR2_PACKAGE_GST_PLUGINS_GOOD_EFFECTV = xy ];then
	OPTS="$OPTS --enable-effectv"
else
	OPTS="$OPTS --disable-effectv"
fi

if [ x$BR2_PACKAGE_GST_PLUGINS_GOOD_EQUALIZER = xy ];then
	OPTS="$OPTS --enable-equalizer"
else
	OPTS="$OPTS --disable-equalizer"
fi

if [ x$BR2_PACKAGE_GST_PLUGINS_GOOD_FLV = xy ];then
	OPTS="$OPTS --enable-flv"
else
	OPTS="$OPTS --disable-flv"
fi

if [ x$BR2_PACKAGE_GST_PLUGINS_GOOD_FLX = xy ];then
	OPTS="$OPTS --enable-flx"
else
	OPTS="$OPTS --disable-flx"
fi

if [ x$BR2_PACKAGE_GST_PLUGINS_GOOD_GOOM = xy ];then
	OPTS="$OPTS --enable-goom"
else
	OPTS="$OPTS --disable-goom"
fi

if [ x$BR2_PACKAGE_GST_PLUGINS_GOOD_GOOM2k1 = xy ];then
	OPTS="$OPTS --enable-goom2k1"
else
	OPTS="$OPTS --disable-goom2k1"
fi

if [ x$BR2_PACKAGE_GST_PLUGINS_GOOD_ICYDEMUX = xy ];then
	OPTS="$OPTS --enable-icydemux"
else
	OPTS="$OPTS --disable-icydemux"
fi

if [ x$BR2_PACKAGE_GST_PLUGINS_GOOD_ID3DEMUX = xy ];then
	OPTS="$OPTS --enable-id3demux"
else
	OPTS="$OPTS --disable-id3demux"
fi

if [ x$BR2_PACKAGE_GST_PLUGINS_GOOD_IMAGEFREEZE = xy ];then
	OPTS="$OPTS --enable-imagefreeze"
else
	OPTS="$OPTS --disable-imagefreeze"
fi

if [ x$BR2_PACKAGE_GST_PLUGINS_GOOD_INTERLEAVE = xy ];then
	OPTS="$OPTS --enable-interleave"
else
	OPTS="$OPTS --disable-interleave"
fi

if [ x$BR2_PACKAGE_GST_PLUGINS_GOOD_ISOMP4 = xy ];then
	OPTS="$OPTS --enable-isomp4"
else
	OPTS="$OPTS --disable-isomp4"
fi

if [ x$BR2_PACKAGE_GST_PLUGINS_GOOD_LAW = xy ];then
	OPTS="$OPTS --enable-law"
else
	OPTS="$OPTS --disable-law"
fi

if [ x$BR2_PACKAGE_GST_PLUGINS_GOOD_LEVEL = xy ];then
	OPTS="$OPTS --enable-level"
else
	OPTS="$OPTS --disable-level"
fi

if [ x$BR2_PACKAGE_GST_PLUGINS_GOOD_MATROSKA = xy ];then
	OPTS="$OPTS --enable-matroska"
else
	OPTS="$OPTS --disable-matroska"
fi

if [ x$BR2_PACKAGE_GST_PLUGINS_GOOD_MONOSCOPE = xy ];then
	OPTS="$OPTS --enable-monoscope"
else
	OPTS="$OPTS --disable-monoscope"
fi

if [ x$BR2_PACKAGE_GST_PLUGINS_GOOD_MULTIFILE = xy ];then
	OPTS="$OPTS --enable-multifile"
else
	OPTS="$OPTS --disable-multifile"
fi

if [ x$BR2_PACKAGE_GST_PLUGINS_GOOD_MULTIPART = xy ];then
	OPTS="$OPTS --enable-multipart"
else
	OPTS="$OPTS --disable-multipart"
fi

if [ x$BR2_PACKAGE_GST_PLUGINS_GOOD_REPLAYGAIN = xy ];then
	OPTS="$OPTS --enable-replaygain"
else
	OPTS="$OPTS --disable-replaygain"
fi

if [ x$BR2_PACKAGE_GST_PLUGINS_GOOD_RTP = xy ];then
	OPTS="$OPTS --enable-rtp"
else
	OPTS="$OPTS --disable-rtp"
fi

if [ x$BR2_PACKAGE_GST_PLUGINS_GOOD_RTPMANAGER = xy ];then
	OPTS="$OPTS --enable-rtpmanager"
else
	OPTS="$OPTS --disable-rtpmanager"
fi

if [ x$BR2_PACKAGE_GST_PLUGINS_GOOD_RTSP = xy ];then
	OPTS="$OPTS --enable-rtsp"
else
	OPTS="$OPTS --disable-rtsp"
fi

if [ x$BR2_PACKAGE_GST_PLUGINS_GOOD_SHAPEWIPE = xy ];then
	OPTS="$OPTS --enable-shapewipe"
else
	OPTS="$OPTS --disable-shapewipe"
fi

if [ x$BR2_PACKAGE_GST_PLUGINS_GOOD_SMPTE = xy ];then
	OPTS="$OPTS --enable-smpte"
else
	OPTS="$OPTS --disable-smpte"
fi

if [ x$BR2_PACKAGE_GST_PLUGINS_GOOD_SPECTRUM = xy ];then
	OPTS="$OPTS --enable-spectrum"
else
	OPTS="$OPTS --disable-spectrum"
fi

if [ x$BR2_PACKAGE_GST_PLUGINS_GOOD_UDP = xy ];then
	OPTS="$OPTS --enable-udp"
else
	OPTS="$OPTS --disable-udp"
fi

if [ x$BR2_PACKAGE_GST_PLUGINS_GOOD_VIDEOBOX = xy ];then
	OPTS="$OPTS --enable-videobox"
else
	OPTS="$OPTS --disable-videobox"
fi

if [ x$BR2_PACKAGE_GST_PLUGINS_GOOD_VIDEOCROP = xy ];then
	OPTS="$OPTS --enable-videocrop"
else
	OPTS="$OPTS --disable-videocrop"
fi

if [ x$BR2_PACKAGE_GST_PLUGINS_GOOD_VIDEOFILTER = xy ];then
	OPTS="$OPTS --enable-videofilter"
else
	OPTS="$OPTS --disable-videofilter"
fi

if [ x$BR2_PACKAGE_GST_PLUGINS_GOOD_VIDEOMIXER = xy ];then
	OPTS="$OPTS --enable-videomixer"
else
	OPTS="$OPTS --disable-videomixer"
fi

if [ x$BR2_PACKAGE_GST_PLUGINS_GOOD_WAVENC = xy ];then
	OPTS="$OPTS --enable-wavenc"
else
	OPTS="$OPTS --disable-wavenc"
fi

if [ x$BR2_PACKAGE_GST_PLUGINS_GOOD_WAVPARSE = xy ];then
	OPTS="$OPTS --enable-wavparse"
else
	OPTS="$OPTS --disable-wavparse"
fi

if [ x$BR2_PACKAGE_GST_PLUGINS_GOOD_Y4M = xy ];then
	OPTS="$OPTS --enable-y4m"
else
	OPTS="$OPTS --disable-y4m"
fi

if [ x$BR2_PACKAGE_GST_PLUGINS_GOOD_DIRECTSOUND = xy ];then
	OPTS="$OPTS --enable-directsound"
else
	OPTS="$OPTS --disable-directsound"
fi

if [ x$BR2_PACKAGE_GST_PLUGINS_GOOD_WAVEFORM = xy ];then
	OPTS="$OPTS --enable-waveform"
else
	OPTS="$OPTS --disable-waveform"
fi

if [ x$BR2_PACKAGE_GST_PLUGINS_GOOD_OSS = xy ];then
	OPTS="$OPTS --enable-oss"
else
	OPTS="$OPTS --disable-oss"
fi

if [ x$BR2_PACKAGE_GST_PLUGINS_GOOD_OSS4 = xy ];then
	OPTS="$OPTS --enable-oss4"
else
	OPTS="$OPTS --disable-oss4"
fi

if [ x$BR2_PACKAGE_GST_PLUGINS_GOOD_OSX_AUDIO = xy ];then
	OPTS="$OPTS --enable-osx_audio"
else
	OPTS="$OPTS --disable-osx_audio"
fi

if [ x$BR2_PACKAGE_GST_PLUGINS_GOOD_OSX_VIDEO = xy ];then
	OPTS="$OPTS --enable-osx_video"
else
	OPTS="$OPTS --disable-osx_video"
fi

if [ x$BR2_PACKAGE_GST_PLUGINS_GOOD_GST_V4L2 = xy ];then
	OPTS="$OPTS --enable-gst_v4l2"
else
	OPTS="$OPTS --disable-gst_v4l2"
fi

if [ x$BR2_PACKAGE_GST_PLUGINS_GOOD_V4L2_PROBE = xy ];then
	OPTS="$OPTS --enable-v4l2-probe"
else
	OPTS="$OPTS --disable-v4l2-probe"
fi

if [ x$BR2_PACKAGE_GST_PLUGINS_GOOD_X = xy ];then
	OPTS="$OPTS --enable-x"
else
	OPTS="$OPTS --disable-x"
fi

if [ x$BR2_PACKAGE_GST_PLUGINS_GOOD_aalib = xy ];then
	OPTS="$OPTS --enable-aalib"
else
	OPTS="$OPTS --disable-aalib"
fi

if [ x$BR2_PACKAGE_GST_PLUGINS_GOOD_AALIBTEST = xy ];then
	OPTS="$OPTS --enable-aalibtest"
else
	OPTS="$OPTS --disable-aalibtest"
fi

if [ x$BR2_PACKAGE_GST_PLUGINS_GOOD_CAIRO = xy ];then
	OPTS="$OPTS --enable-cairo"
else
	OPTS="$OPTS --disable-cairo"
fi

if [ x$BR2_PACKAGE_GST_PLUGINS_GOOD_FLAC = xy ];then
	OPTS="$OPTS --enable-flac"
else
	OPTS="$OPTS --disable-flac"
fi

if [ x$BR2_PACKAGE_GST_PLUGINS_GOOD_GDK_PIXBUF = xy ];then
	OPTS="$OPTS --enable-gdk_pixbuf"
else
	OPTS="$OPTS --disable-gdk_pixbuf"
fi

if [ x$BR2_PACKAGE_GST_PLUGINS_GOOD_GTK3 = xy ];then
	OPTS="$OPTS --enable-gtk3"
else
	OPTS="$OPTS --disable-gtk3"
fi

if [ x$BR2_PACKAGE_GST_PLUGINS_GOOD_JACK = xy ];then
	OPTS="$OPTS --enable-jack"
else
	OPTS="$OPTS --disable-jack"
fi

if [ x$BR2_PACKAGE_GST_PLUGINS_GOOD_JPEG = xy ];then
	OPTS="$OPTS --enable-jpeg"
else
	OPTS="$OPTS --disable-jpeg"
fi

if [ x$BR2_PACKAGE_GST_PLUGINS_GOOD_LAME = xy ];then
	OPTS="$OPTS --enable-lame"
else
	OPTS="$OPTS --disable-lame"
fi

if [ x$BR2_PACKAGE_GST_PLUGINS_GOOD_LIBCACA = xy ];then
	OPTS="$OPTS --enable-libcaca"
else
	OPTS="$OPTS --disable-libcaca"
fi

if [ x$BR2_PACKAGE_GST_PLUGINS_GOOD_LIBDV = xy ];then
	OPTS="$OPTS --enable-libdv"
else
	OPTS="$OPTS --disable-libdv"
fi

if [ x$BR2_PACKAGE_GST_PLUGINS_GOOD_LIBPNG = xy ];then
	OPTS="$OPTS --enable-libpng"
else
	OPTS="$OPTS --disable-libpng"
fi

if [ x$BR2_PACKAGE_GST_PLUGINS_GOOD_MPG123 = xy ];then
	OPTS="$OPTS --enable-mpg123"
	DEPENDENCIES="$DEPENDENCIES libmpg123"
else
	OPTS="$OPTS --disable-mpg123"
fi

if [ x$BR2_PACKAGE_GST_PLUGINS_GOOD_PULSE = xy ];then
	OPTS="$OPTS --enable-pulse"
else
	OPTS="$OPTS --disable-pulse"
fi

if [ x$BR2_PACKAGE_GST_PLUGINS_GOOD_DV1394 = xy ];then
	OPTS="$OPTS --enable-dv1394"
else
	OPTS="$OPTS --disable-dv1394"
fi

if [ x$BR2_PACKAGE_GST_PLUGINS_GOOD_QT = xy ];then
	OPTS="$OPTS --enable-qt"
else
	OPTS="$OPTS --disable-qt"
fi

if [ x$BR2_PACKAGE_GST_PLUGINS_GOOD_SHOUT2 = xy ];then
	OPTS="$OPTS --enable-shout2"
else
	OPTS="$OPTS --disable-shout2"
fi

if [ x$BR2_PACKAGE_GST_PLUGINS_GOOD_SOUP = xy ];then
	OPTS="$OPTS --enable-soup"
else
	OPTS="$OPTS --disable-soup"
fi

if [ x$BR2_PACKAGE_GST_PLUGINS_GOOD_SPEEX = xy ];then
	OPTS="$OPTS --enable-speex"
else
	OPTS="$OPTS --disable-speex"
fi

if [ x$BR2_PACKAGE_GST_PLUGINS_GOOD_TAGLIB = xy ];then
	OPTS="$OPTS --enable-taglib"
else
	OPTS="$OPTS --disable-taglib"
fi

if [ x$BR2_PACKAGE_GST_PLUGINS_GOOD_TWOLAME = xy ];then
	OPTS="$OPTS --enable-twolame"
else
	OPTS="$OPTS --disable-twolame"
fi

if [ x$BR2_PACKAGE_GST_PLUGINS_GOOD_VPX = xy ];then
	OPTS="$OPTS --enable-vpx"
else
	OPTS="$OPTS --disable-vpx"
fi

if [ x$BR2_PACKAGE_GST_PLUGINS_GOOD_WAVPACK = xy ];then
	OPTS="$OPTS --enable-wavpack"
else
	OPTS="$OPTS --disable-wavpack"
fi

if [ x$BR2_PACKAGE_GST_PLUGINS_GOOD_ZLIB = xy ];then
	OPTS="$OPTS --enable-zlib"
else
	OPTS="$OPTS --disable-zlib"
fi

if [ x$BR2_PACKAGE_GST_PLUGINS_GOOD_BZ2 = xy ];then
	OPTS="$OPTS --enable-bz2"
else
	OPTS="$OPTS --disable-bz2"
fi
$SCRIPTS_DIR/build_pkgs.sh $ARCH $SUITE "$DEPENDENCIES"
echo "opts: $OPTS"
./configure $OPTS
make
make install
$SCRIPTS_DIR/fixlibtool.sh $TARGET_DIR $TARGET_DIR
cd -
