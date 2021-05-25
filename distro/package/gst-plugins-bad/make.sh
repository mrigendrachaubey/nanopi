#!/bin/bash

set -e
DEPENDENCIES=gst-plugins-base
PKG=gst-plugins-bad
VERSION=1.14.4
source $OUTPUT_DIR/.config
if [ ! -e $DOWNLOAD_DIR/$PKG-$VERSION.tar.xz ];then
	wget -P $DOWNLOAD_DIR https://gstreamer.freedesktop.org/src/gst-plugins-bad/$PKG-$VERSION.tar.xz
fi

if [ ! -d $BUILD_DIR/$PKG/$PKG-$VERSION ];then
	tar -xf $DOWNLOAD_DIR/$PKG-$VERSION.tar.xz -C $BUILD_DIR/$PKG
	mv $BUILD_DIR/$PKG/$PKG-$VERSION/* $BUILD_DIR/$PKG/
fi

cd $BUILD_DIR/$PKG
OPTS="--target=aarch64-linux-gnu --host=aarch64-linux-gnu --prefix=/usr --libdir=/usr/lib/$TOOLCHAIN --disable-gtk-doc --disable-gtk-doc-html --disable-dependency-tracking --disable-nls --disable-static --enable-shared  --disable-examples --disable-valgrind"

if [ x$BR2_PACKAGE_GST_PLUGINS_BAD_IPA = xy ];then
	OPTS="$OPTS --enable-iqa"
else
	OPTS="$OPTS --disable-iqa"
fi

if [ x$BR2_PACKAGE_GST_PLUGINS_BAD_ORC = xy ];then
	OPTS="$OPTS --enable-orc"
else
	OPTS="$OPTS --disable-orc"
fi

if [ x$BR2_PACKAGE_GST_PLUGINS_BAD_ACCURIP = xy ];then
	OPTS="$OPTS --enable-accurip"
else
	OPTS="$OPTS --disable-accurip"
fi

if [ x$BR2_PACKAGE_GST_PLUGINS_BAD_ADPCMDEC = xy ];then
	OPTS="$OPTS --enable-adpcmdec"
else
	OPTS="$OPTS --disable-adpcmdec"
fi

if [ x$BR2_PACKAGE_GST_PLUGINS_BAD_ADPCMENC = xy ];then
	OPTS="$OPTS --enable-adpcmenc"
else
	OPTS="$OPTS --disable-adpcmenc"
fi

if [ x$BR2_PACKAGE_GST_PLUGINS_BAD_AIFF = xy ];then
	OPTS="$OPTS --enable-aiff"
else
	OPTS="$OPTS --disable-aiff"
fi

if [ x$BR2_PACKAGE_GST_PLUGINS_BAD_VIDEOFRAME_AUDIOLEVEL = xy ];then
	OPTS="$OPTS --enable-videoframe_audiolevel"
else
	OPTS="$OPTS --disable-videoframe_audiolevel"
fi

if [ x$BR2_PACKAGE_GST_PLUGINS_BAD_ASFMUX = xy ];then
	OPTS="$OPTS --enable-asfmux"
else
	OPTS="$OPTS --disable-asfmux"
fi

if [ x$BR2_PACKAGE_GST_PLUGINS_BAD_AUDIOBUFFERSPLIT = xy ];then
	OPTS="$OPTS --enable-audiobuffersplit"
else
	OPTS="$OPTS --disable-audiobuffersplit"
fi

if [ x$BR2_PACKAGE_GST_PLUGINS_BAD_AUDIOFXBAD = xy ];then
	OPTS="$OPTS --enable-audiofxbad"
else
	OPTS="$OPTS --disable-audiofxbad"
fi

if [ x$BR2_PACKAGE_GST_PLUGINS_BAD_AUDIOLATENCY = xy ];then
	OPTS="$OPTS --enable-audiolatency"
else
	OPTS="$OPTS --disable-audiolatency"
fi

if [ x$BR2_PACKAGE_GST_PLUGINS_BAD_AUDIOMIXMATRIX = xy ];then
	OPTS="$OPTS --enable-audiomixmatrix"
else
	OPTS="$OPTS --disable-audiomixmatrix"
fi

if [ x$BR2_PACKAGE_GST_PLUGINS_BAD_COMPOSITOR = xy ];then
	OPTS="$OPTS --enable-compositor"
else
	OPTS="$OPTS --disable-compositor"
fi

if [ x$BR2_PACKAGE_GST_PLUGINS_BAD_AUDIOVISUALIZERS = xy ];then
	OPTS="$OPTS --enable-audiovisualizers"
else
	OPTS="$OPTS --disable-audiovisualizers"
fi

if [ x$BR2_PACKAGE_GST_PLUGINS_BAD_AUTOCONVERT = xy ];then
	OPTS="$OPTS --enable-autoconvert"
else
	OPTS="$OPTS --disable-autoconvert"
fi

if [ x$BR2_PACKAGE_GST_PLUGINS_BAD_BAYER = xy ];then
	OPTS="$OPTS --enable-bayer"
else
	OPTS="$OPTS --disable-bayer"
fi

if [ x$BR2_PACKAGE_GST_PLUGINS_BAD_CAMERABIN2 = xy ];then
	OPTS="$OPTS --enable-camerabin2"
else
	OPTS="$OPTS --disable-camerabin2"
fi

if [ x$BR2_PACKAGE_GST_PLUGINS_BAD_COLOREFFECTS = xy ];then
	OPTS="$OPTS --enable-coloreffects"
else
	OPTS="$OPTS --disable-coloreffects"
fi

if [ x$BR2_PACKAGE_GST_PLUGINS_BAD_DEBUGUTILS = xy ];then
	OPTS="$OPTS --enable-debugutils"
else
	OPTS="$OPTS --disable-debugutils"
fi

if [ x$BR2_PACKAGE_GST_PLUGINS_BAD_DVBSUBOVERLAY = xy ];then
	OPTS="$OPTS --enable-dvbsuboverlay"
else
	OPTS="$OPTS --disable-dvbsuboverlay"
fi

if [ x$BR2_PACKAGE_GST_PLUGINS_BAD_DVDSPU = xy ];then
	OPTS="$OPTS --enable-dvdspu"
else
	OPTS="$OPTS --disable-dvdspu"
fi

if [ x$BR2_PACKAGE_GST_PLUGINS_BAD_FACEOVERLAY = xy ];then
	OPTS="$OPTS --enable-faceoverlay"
else
	OPTS="$OPTS --disable-faceoverlay"
fi

if [ x$BR2_PACKAGE_GST_PLUGINS_BAD_FESTIVAL = xy ];then
	OPTS="$OPTS --enable-festival"
else
	OPTS="$OPTS --disable-festival"
fi

if [ x$BR2_PACKAGE_GST_PLUGINS_BAD_FIELDANALYSIS = xy ];then
	OPTS="$OPTS --enable-fieldanalysis"
else
	OPTS="$OPTS --disable-fieldanalysis"
fi

if [ x$BR2_PACKAGE_GST_PLUGINS_BAD_FREEVERB = xy ];then
	OPTS="$OPTS --enable-freeverb"
else
	OPTS="$OPTS --disable-freeverb"
fi

if [ x$BR2_PACKAGE_GST_PLUGINS_BAD_FREI0R = xy ];then
	OPTS="$OPTS --enable-frei0r"
else
	OPTS="$OPTS --disable-frei0r"
fi

if [ x$BR2_PACKAGE_GST_PLUGINS_BAD_GAUDIEFFECTS = xy ];then
	OPTS="$OPTS --enable-gaudieffects"
else
	OPTS="$OPTS --disable-gaudieffects"
fi

if [ x$BR2_PACKAGE_GST_PLUGINS_BAD_GEOMETRICTRANSFORM = xy ];then
	OPTS="$OPTS --enable-geometrictransform"
else
	OPTS="$OPTS --disable-geometrictransform"
fi

if [ x$BR2_PACKAGE_GST_PLUGINS_BAD_GDP = xy ];then
	OPTS="$OPTS --enable-gdp"
else
	OPTS="$OPTS --disable-gdp"
fi

if [ x$BR2_PACKAGE_GST_PLUGINS_BAD_ID3TAG = xy ];then
	OPTS="$OPTS --enable-id3tag"
else
	OPTS="$OPTS --disable-id3tag"
fi

if [ x$BR2_PACKAGE_GST_PLUGINS_BAD_INTER = xy ];then
	OPTS="$OPTS --enable-inter"
else
	OPTS="$OPTS --disable-inter"
fi

if [ x$BR2_PACKAGE_GST_PLUGINS_BAD_INTERLACE = xy ];then
	OPTS="$OPTS --enable-interlace"
else
	OPTS="$OPTS --disable-interlace"
fi

if [ x$BR2_PACKAGE_GST_PLUGINS_BAD_IVFPARSE = xy ];then
	OPTS="$OPTS --enable-ivfparse"
else
	OPTS="$OPTS --disable-ivfparse"
fi

if [ x$BR2_PACKAGE_GST_PLUGINS_BAD_IVTC = xy ];then
	OPTS="$OPTS --enable-ivtc"
else
	OPTS="$OPTS --disable-ivtc"
fi

if [ x$BR2_PACKAGE_GST_PLUGINS_BAD_JP2KDECIMATOR = xy ];then
	OPTS="$OPTS --enable-jp2kdecimator"
else
	OPTS="$OPTS --disable-jp2kdecimator"
fi

if [ x$BR2_PACKAGE_GST_PLUGINS_BAD_JPEGFORMAT = xy ];then
	OPTS="$OPTS --enable-jpegformat"
else
	OPTS="$OPTS --disable-jpegformat"
fi

if [ x$BR2_PACKAGE_GST_PLUGINS_BAD_LIBRFB = xy ];then
	OPTS="$OPTS --enable-librfb"
else
	OPTS="$OPTS --disable-librfb"
fi

if [ x$BR2_PACKAGE_GST_PLUGINS_BAD_MIDI = xy ];then
	OPTS="$OPTS --enable-midi"
else
	OPTS="$OPTS --disable-midi"
fi

if [ x$BR2_PACKAGE_GST_PLUGINS_BAD_MPEGDEMUX = xy ];then
	OPTS="$OPTS --enable-mpegdemux"
else
	OPTS="$OPTS --disable-mpegdemux"
fi

if [ x$BR2_PACKAGE_GST_PLUGINS_BAD_MPEGTSDEMUX = xy ];then
	OPTS="$OPTS --enable-mpegtsdemux"
else
	OPTS="$OPTS --disable-mpegtsdemux"
fi

if [ x$BR2_PACKAGE_GST_PLUGINS_BAD_MPEGTSMUX = xy ];then
	OPTS="$OPTS --enable-mpegtsmux"
else
	OPTS="$OPTS --disable-mpegtsmux"
fi

if [ x$BR2_PACKAGE_GST_PLUGINS_BAD_MPEGPSMUX = xy ];then
	OPTS="$OPTS --enable-mpegpsmux"
else
	OPTS="$OPTS --disable-mpegpsmux"
fi

if [ x$BR2_PACKAGE_GST_PLUGINS_BAD_MXF = xy ];then
	OPTS="$OPTS --enable-mxf"
else
	OPTS="$OPTS --disable-mxf"
fi

if [ x$BR2_PACKAGE_GST_PLUGINS_BAD_NETSIM = xy ];then
	OPTS="$OPTS --enable-netsim"
else
	OPTS="$OPTS --disable-netsim"
fi

if [ x$BR2_PACKAGE_GST_PLUGINS_BAD_ONVIF = xy ];then
	OPTS="$OPTS --enable-onvif"
else
	OPTS="$OPTS --disable-onvif"
fi

if [ x$BR2_PACKAGE_GST_PLUGINS_BAD_PCAPPARSE = xy ];then
	OPTS="$OPTS --enable-pcapparse"
else
	OPTS="$OPTS --disable-pcapparse"
fi

if [ x$BR2_PACKAGE_GST_PLUGINS_BAD_PNM = xy ];then
	OPTS="$OPTS --enable-pnm"
else
	OPTS="$OPTS --disable-pnm"
fi

if [ x$BR2_PACKAGE_GST_PLUGINS_BAD_PROXY = xy ];then
	OPTS="$OPTS --disable-proxy"
else
	OPTS="$OPTS --disable-proxy"
fi

if [ x$BR2_PACKAGE_GST_PLUGINS_BAD_RAWPARSE = xy ];then
	OPTS="$OPTS --enable-rawparse"
else
	OPTS="$OPTS --disable-rawparse"
fi

if [ x$BR2_PACKAGE_GST_PLUGINS_BAD_REMOVESILENCE = xy ];then
	OPTS="$OPTS --enable-removesilence"
else
	OPTS="$OPTS --disable-removesilence"
fi

if [ x$BR2_PACKAGE_GST_PLUGINS_BAD_SDP = xy ];then
	OPTS="$OPTS --enable-sdp"
else
	OPTS="$OPTS --disable-sdp"
fi

if [ x$BR2_PACKAGE_GST_PLUGINS_BAD_SEGMENTCLIP = xy ];then
	OPTS="$OPTS --enable-segmentclip"
else
	OPTS="$OPTS --disable-segmentclip"
fi

if [ x$BR2_PACKAGE_GST_PLUGINS_BAD_SIREN = xy ];then
	OPTS="$OPTS --enable-siren"
else
	OPTS="$OPTS --disable-siren"
fi

if [ x$BR2_PACKAGE_GST_PLUGINS_BAD_SMOOTH = xy ];then
	OPTS="$OPTS --enable-smooth"
else
	OPTS="$OPTS --disable-smooth"
fi

if [ x$BR2_PACKAGE_GST_PLUGINS_BAD_SPEED = xy ];then
	OPTS="$OPTS --enable-speed"
else
	OPTS="$OPTS --disable-speed"
fi

if [ x$BR2_PACKAGE_GST_PLUGINS_BAD_SUBENC = xy ];then
	OPTS="$OPTS --enable-subenc"
else
	OPTS="$OPTS --disable-subenc"
fi

if [ x$BR2_PACKAGE_GST_PLUGINS_BAD_STEREO = xy ];then
	OPTS="$OPTS --enable-stereo"
else
	OPTS="$OPTS --disable-stereo"
fi

if [ x$BR2_PACKAGE_GST_PLUGINS_BAD_TIMECODE = xy ];then
	OPTS="$OPTS --enable-timecode"
else
	OPTS="$OPTS --disable-timecode"
fi

if [ x$BR2_PACKAGE_GST_PLUGINS_BAD_VIDEOFILTERS = xy ];then
	OPTS="$OPTS --enable-videofilters"
else
	OPTS="$OPTS --disable-videofilters"
fi

if [ x$BR2_PACKAGE_GST_PLUGINS_BAD_VIDEOPARSERS = xy ];then
	OPTS="$OPTS --enable-videoparsers"
else
	OPTS="$OPTS --disable-videoparsers"
fi

if [ x$BR2_PACKAGE_GST_PLUGINS_BAD_VIDEOSIGNAL = xy ];then
	OPTS="$OPTS --enable-videosignal"
else
	OPTS="$OPTS --disable-videosignal"
fi

if [ x$BR2_PACKAGE_GST_PLUGINS_BAD_VMNC = xy ];then
	OPTS="$OPTS --enable-vmnc"
else
	OPTS="$OPTS --disable-vmnc"
fi

if [ x$BR2_PACKAGE_GST_PLUGINS_BAD_Y4M = xy ];then
	OPTS="$OPTS --enable-y4m"
else
	OPTS="$OPTS --disable-y4m"
fi

if [ x$BR2_PACKAGE_GST_PLUGINS_BAD_YADIF = xy ];then
	OPTS="$OPTS --enable-yadif"
else
	OPTS="$OPTS --disable-yadif"
fi

if [ x$BR2_PACKAGE_GST_PLUGINS_BAD_DIRECTSOUND = xy ];then
	OPTS="$OPTS --enable-directsound"
else
	OPTS="$OPTS --disable-directsound"
fi

if [ x$BR2_PACKAGE_GST_PLUGINS_BAD_WASAPI = xy ];then
	OPTS="$OPTS --enable-wasapi"
else
	OPTS="$OPTS --disable-wasapi"
fi

if [ x$BR2_PACKAGE_GST_PLUGINS_BAD_DIRECT3D = xy ];then
	OPTS="$OPTS --enable-direct3d"
else
	OPTS="$OPTS --disable-direct3d"
fi

if [ x$BR2_PACKAGE_GST_PLUGINS_BAD_WINSCREENCAP = xy ];then
	OPTS="$OPTS --enable-winscreencap"
else
	OPTS="$OPTS --disable-winscreencap"
fi

if [ x$BR2_PACKAGE_GST_PLUGINS_BAD_WINKS = xy ];then
	OPTS="$OPTS --enable-winks"
else
	OPTS="$OPTS --disable-winks"
fi

if [ x$BR2_PACKAGE_GST_PLUGINS_BAD_ANDROID_MEDIA = xy ];then
	OPTS="$OPTS --enable-android_media"
else
	OPTS="$OPTS --disable-android_media"
fi

if [ x$BR2_PACKAGE_GST_PLUGINS_BAD_APPLE_MEDIA = xy ];then
	OPTS="$OPTS --enable-apple_media"
else
	OPTS="$OPTS --disable-apple_media"
fi

if [ x$BR2_PACKAGE_GST_PLUGINS_BAD_BLUEZ = xy ];then
	OPTS="$OPTS --enable-bluez"
else
	OPTS="$OPTS --disable-bluez"
fi

if [ x$BR2_PACKAGE_GST_PLUGINS_BAD_AVC = xy ];then
	OPTS="$OPTS --enable-avc"
else
	OPTS="$OPTS --disable-avc"
fi

if [ x$BR2_PACKAGE_GST_PLUGINS_BAD_SHM = xy ];then
	OPTS="$OPTS --enable-shm"
else
	OPTS="$OPTS --disable-shm"
fi

if [ x$BR2_PACKAGE_GST_PLUGINS_BAD_IPCPIPELINE = xy ];then
	OPTS="$OPTS --enable-ipcpipeline"
else
	OPTS="$OPTS --disable-ipcpipeline"
fi

if [ x$BR2_PACKAGE_GST_PLUGINS_BAD_VCD = xy ];then
	OPTS="$OPTS --enable-vcd"
else
	OPTS="$OPTS --disable-vcd"
fi

if [ x$BR2_PACKAGE_GST_PLUGINS_BAD_OPENSLES = xy ];then
	OPTS="$OPTS --enable-opensles"
else
	OPTS="$OPTS --disable-opensles"
fi

if [ x$BR2_PACKAGE_GST_PLUGINS_BAD_UVCH264 = xy ];then
	OPTS="$OPTS --enable-uvch264"
else
	OPTS="$OPTS --disable-uvch264"
fi

if [ x$BR2_PACKAGE_GST_PLUGINS_BAD_CUDA = xy ];then
	OPTS="$OPTS --enable-cuda"
else
	OPTS="$OPTS --disable-cuda"
fi

if [ x$BR2_PACKAGE_GST_PLUGINS_BAD_NVDEC = xy ];then
	OPTS="$OPTS --enable-nvdec"
else
	OPTS="$OPTS --disable-nvdec"
fi

if [ x$BR2_PACKAGE_GST_PLUGINS_BAD_NVENC = xy ];then
	OPTS="$OPTS --enable-nvenc"
else
	OPTS="$OPTS --disable-nvenc"
fi

if [ x$BR2_PACKAGE_GST_PLUGINS_BAD_TINYALSA = xy ];then
	OPTS="$OPTS --enable-tinyalsa"
else
	OPTS="$OPTS --disable-tinyalsa"
fi

if [ x$BR2_PACKAGE_GST_PLUGINS_BAD_MSDK = xy ];then
	OPTS="$OPTS --enable-msdk"
else
	OPTS="$OPTS --disable-msdk"
fi

if [ x$BR2_PACKAGE_GST_PLUGINS_BAD_ASSRENDER = xy ];then
	OPTS="$OPTS --enable-assrender"
else
	OPTS="$OPTS --disable-assrender"
fi

if [ x$BR2_PACKAGE_GST_PLUGINS_BAD_AOM = xy ];then
	OPTS="$OPTS --enable-aom"
else
	OPTS="$OPTS --disable-aom"
fi

if [ x$BR2_PACKAGE_GST_PLUGINS_BAD_VOAMRWBENC = xy ];then
	OPTS="$OPTS --enable-voamrwbenc"
else
	OPTS="$OPTS --disable-voamrwbenc"
fi

if [ x$BR2_PACKAGE_GST_PLUGINS_BAD_VOAACENC = xy ];then
	OPTS="$OPTS --enable-voaacenc"
else
	OPTS="$OPTS --disable-voaacenc"
fi

if [ x$BR2_PACKAGE_GST_PLUGINS_BAD_BS2B = xy ];then
	OPTS="$OPTS --enable-bs2b"
else
	OPTS="$OPTS --disable-bs2b"
fi

if [ x$BR2_PACKAGE_GST_PLUGINS_BAD_BZ2 = xy ];then
	OPTS="$OPTS --enable-bz2"
else
	OPTS="$OPTS --disable-bz2"
fi

if [ x$BR2_PACKAGE_GST_PLUGINS_BAD_CHROMAPRINT = xy ];then
	OPTS="$OPTS --enable-chromaprint"
else
	OPTS="$OPTS --disable-chromaprint"
fi

if [ x$BR2_PACKAGE_GST_PLUGINS_BAD_CURL = xy ];then
	OPTS="$OPTS --enable-curl"
else
	OPTS="$OPTS --disable-curl"
fi

if [ x$BR2_PACKAGE_GST_PLUGINS_BAD_DASH = xy ];then
	OPTS="$OPTS --enable-dash"
else
	OPTS="$OPTS --disable-dash"
fi

if [ x$BR2_PACKAGE_GST_PLUGINS_BAD_DC1394 = xy ];then
	OPTS="$OPTS --enable-dc1394"
else
	OPTS="$OPTS --disable-dc1394"
fi

if [ x$BR2_PACKAGE_GST_PLUGINS_BAD_DECKLINK = xy ];then
	OPTS="$OPTS --enable-decklink"
else
	OPTS="$OPTS --disable-decklink"
fi

if [ x$BR2_PACKAGE_GST_PLUGINS_BAD_DIRECTFB = xy ];then
	OPTS="$OPTS --enable-directfb"
else
	OPTS="$OPTS --disable-directfb"
fi

if [ x$BR2_PACKAGE_GST_PLUGINS_BAD_WAYLAND = xy ];then
	OPTS="$OPTS --enable-wayland"
else
	OPTS="$OPTS --disable-wayland"
fi

if [ x$BR2_PACKAGE_GST_PLUGINS_BAD_WEBP = xy ];then
	OPTS="$OPTS --enable-webp"
else
	OPTS="$OPTS --disable-webp"
fi

if [ x$BR2_PACKAGE_GST_PLUGINS_BAD_DAALA = xy ];then
	OPTS="$OPTS --enable-daala"
else
	OPTS="$OPTS --disable-daala"
fi

if [ x$BR2_PACKAGE_GST_PLUGINS_BAD_DTS = xy ];then
	OPTS="$OPTS --enable-dts"
else
	OPTS="$OPTS --disable-dts"
fi

if [ x$BR2_PACKAGE_GST_PLUGINS_BAD_RESINDVD = xy ];then
	OPTS="$OPTS --enable-resindvd"
else
	OPTS="$OPTS --disable-resindvd"
fi

if [ x$BR2_PACKAGE_GST_PLUGINS_BAD_FAAC = xy ];then
	OPTS="$OPTS --enable-faac"
else
	OPTS="$OPTS --disable-faac"
fi

if [ x$BR2_PACKAGE_GST_PLUGINS_BAD_FAAD = xy ];then
	OPTS="$OPTS --enable-faad"
	DEPENDENCIES="$DEPENDENCIES libfaad-dev"
else
	OPTS="$OPTS --disable-faad"
fi

if [ x$BR2_PACKAGE_GST_PLUGINS_BAD_FBDEV = xy ];then
	OPTS="$OPTS --enable-fbdev"
else
	OPTS="$OPTS --disable-fbdev"
fi

if [ x$BR2_PACKAGE_GST_PLUGINS_BAD_FDK_AAC = xy ];then
	OPTS="$OPTS --enable-fdk_aac"
else
	OPTS="$OPTS --disable-fdk_aac"
fi

if [ x$BR2_PACKAGE_GST_PLUGINS_BAD_FLITE = xy ];then
	OPTS="$OPTS --enable-flite"
else
	OPTS="$OPTS --disable-flite"
fi

if [ x$BR2_PACKAGE_GST_PLUGINS_BAD_GSM = xy ];then
	OPTS="$OPTS --enable-gsm"
else
	OPTS="$OPTS --disable-gsm"
fi

if [ x$BR2_PACKAGE_GST_PLUGINS_BAD_FLUIDSYNTH = xy ];then
	OPTS="$OPTS --enable-fluidsynth"
else
	OPTS="$OPTS --disable-fluidsynth"
fi

if [ x$BR2_PACKAGE_GST_PLUGINS_BAD_KATE = xy ];then
	OPTS="$OPTS --enable-kate"
else
	OPTS="$OPTS --disable-kate"
fi

if [ x$BR2_PACKAGE_GST_PLUGINS_BAD_KMS = xy ];then
	OPTS="$OPTS --enable-kms"
else
	OPTS="$OPTS --disable-kms"
fi

if [ x$BR2_PACKAGE_GST_PLUGINS_BAD_LADSPA = xy ];then
	OPTS="$OPTS --enable-ladspa"
else
	OPTS="$OPTS --disable-ladspa"
fi

if [ x$BR2_PACKAGE_GST_PLUGINS_BAD_LCMS2 = xy ];then
	OPTS="$OPTS --enable-lcms2"
else
	OPTS="$OPTS --disable-lcms2"
fi

if [ x$BR2_PACKAGE_GST_PLUGINS_BAD_LV2 = xy ];then
	OPTS="$OPTS --enable-lv2"
else
	OPTS="$OPTS --disable-lv2"
fi

if [ x$BR2_PACKAGE_GST_PLUGINS_BAD_LIBDE265 = xy ];then
	OPTS="$OPTS --enable-libde265"
else
	OPTS="$OPTS --disable-libde265"
fi

if [ x$BR2_PACKAGE_GST_PLUGINS_BAD_LIBMMS = xy ];then
	OPTS="$OPTS --enable-libmms"
else
	OPTS="$OPTS --disable-libmms"
fi

if [ x$BR2_PACKAGE_GST_PLUGINS_BAD_SRT = xy ];then
	OPTS="$OPTS --enable-srt"
else
	OPTS="$OPTS --disable-srt"
fi

if [ x$BR2_PACKAGE_GST_PLUGINS_BAD_SRTP = xy ];then
	OPTS="$OPTS --enable-srtp"
else
	OPTS="$OPTS --disable-srtp"
fi

if [ x$BR2_PACKAGE_GST_PLUGINS_BAD_DTLS = xy ];then
	OPTS="$OPTS --enable-dtls"
else
	OPTS="$OPTS --disable-dtls"
fi

if [ x$BR2_PACKAGE_GST_PLUGINS_BAD_TTML = xy ];then
	OPTS="$OPTS --enable-ttml"
else
	OPTS="$OPTS --disable-ttml"
fi

if [ x$BR2_PACKAGE_GST_PLUGINS_BAD_MODPLUG = xy ];then
	OPTS="$OPTS --enable-modplug"
else
	OPTS="$OPTS --disable-modplug"
fi

if [ x$BR2_PACKAGE_GST_PLUGINS_BAD_MPEG2ENC = xy ];then
	OPTS="$OPTS --enable-mpeg2enc"
else
	OPTS="$OPTS --disable-mpeg2enc"
fi

if [ x$BR2_PACKAGE_GST_PLUGINS_BAD_MPLEX = xy ];then
	OPTS="$OPTS --enable-mplex"
else
	OPTS="$OPTS --disable-mplex"
fi

if [ x$BR2_PACKAGE_GST_PLUGINS_BAD_MUSEPACK = xy ];then
	OPTS="$OPTS --enable-musepack"
else
	OPTS="$OPTS --disable-musepack"
fi

if [ x$BR2_PACKAGE_GST_PLUGINS_BAD_NEON = xy ];then
	OPTS="$OPTS --enable-neon"
else
	OPTS="$OPTS --disable-neon"
fi

if [ x$BR2_PACKAGE_GST_PLUGINS_BAD_OFA = xy ];then
	OPTS="$OPTS --enable-ofa"
else
	OPTS="$OPTS --disable-ofa"
fi

if [ x$BR2_PACKAGE_GST_PLUGINS_BAD_OPENAL = xy ];then
	OPTS="$OPTS --enable-openal"
else
	OPTS="$OPTS --disable-openal"
fi

if [ x$BR2_PACKAGE_GST_PLUGINS_BAD_OPENCV = xy ];then
	OPTS="$OPTS --enable-opencv"
else
	OPTS="$OPTS --disable-opencv"
fi

if [ x$BR2_PACKAGE_GST_PLUGINS_BAD_OPENEXR = xy ];then
	OPTS="$OPTS --enable-openexr"
else
	OPTS="$OPTS --disable-openexr"
fi

if [ x$BR2_PACKAGE_GST_PLUGINS_BAD_OPENH264 = xy ];then
	OPTS="$OPTS --enable-openh264"
else
	OPTS="$OPTS --disable-openh264"
fi

if [ x$BR2_PACKAGE_GST_PLUGINS_BAD_OPENJPEG = xy ];then
	OPTS="$OPTS --enable-openjpeg"
else
	OPTS="$OPTS --disable-openjpeg"
fi

if [ x$BR2_PACKAGE_GST_PLUGINS_BAD_OPENMPT = xy ];then
	OPTS="$OPTS --enable-openmpt"
else
	OPTS="$OPTS --disable-openmpt"
fi

if [ x$BR2_PACKAGE_GST_PLUGINS_BAD_OPENNI2 = xy ];then
	OPTS="$OPTS --enable-openni2"
else
	OPTS="$OPTS --disable-openni2"
fi

if [ x$BR2_PACKAGE_GST_PLUGINS_BAD_OPUS = xy ];then
	OPTS="$OPTS --enable-opus"
else
	OPTS="$OPTS --disable-opus"
fi

if [ x$BR2_PACKAGE_GST_PLUGINS_BAD_RSVG = xy ];then
	OPTS="$OPTS --disable-rsvg"
else
	OPTS="$OPTS --disable-rsvg"
fi

if [ x$BR2_PACKAGE_GST_PLUGINS_BAD_GL = xy ];then
	OPTS="$OPTS --enable-gl"
else
	OPTS="$OPTS --disable-gl"
fi

if [ x$BR2_PACKAGE_GST_PLUGINS_BAD_VULKAN = xy ];then
	OPTS="$OPTS --enable-vulkan"
else
	OPTS="$OPTS --disable-vulkan"
fi

if [ x$BR2_PACKAGE_GST_PLUGINS_BAD_TELETEXTDEC = xy ];then
	OPTS="$OPTS --enable-teletextdec"
else
	OPTS="$OPTS --disable-teletextdec"
fi

if [ x$BR2_PACKAGE_GST_PLUGINS_BAD_WILDMIDI = xy ];then
	OPTS="$OPTS --enable-wildmidi"
else
	OPTS="$OPTS --disable-wildmidi"
fi

if [ x$BR2_PACKAGE_GST_PLUGINS_BAD_SMOOTHSTREAMING = xy ];then
	OPTS="$OPTS --enable-smoothstreaming"
else
	OPTS="$OPTS --disable-smoothstreaming"
fi

if [ x$BR2_PACKAGE_GST_PLUGINS_BAD_SNDFILE = xy ];then
	OPTS="$OPTS --enable-sndfile"
else
	OPTS="$OPTS --disable-sndfile"
fi

if [ x$BR2_PACKAGE_GST_PLUGINS_BAD_SOUNDTOUCH = xy ];then
	OPTS="$OPTS --enable-soundtouch"
else
	OPTS="$OPTS --disable-soundtouch"
fi

if [ x$BR2_PACKAGE_GST_PLUGINS_BAD_SPC = xy ];then
	OPTS="$OPTS --enable-spc"
else
	OPTS="$OPTS --disable-spc"
fi

if [ x$BR2_PACKAGE_GST_PLUGINS_BAD_GME = xy ];then
	OPTS="$OPTS --enable-gme"
else
	OPTS="$OPTS --disable-gme"
fi

if [ x$BR2_PACKAGE_GST_PLUGINS_BAD_DVB = xy ];then
	OPTS="$OPTS --enable-dvb"
else
	OPTS="$OPTS --disable-dvb"
fi

if [ x$BR2_PACKAGE_GST_PLUGINS_BAD_ACM = xy ];then
	OPTS="$OPTS --enable-acm"
else
	OPTS="$OPTS --disable-acm"
fi

if [ x$BR2_PACKAGE_GST_PLUGINS_BAD_VDPAU = xy ];then
	OPTS="$OPTS --enable-vdpau"
else
	OPTS="$OPTS --disable-vdpau"
fi

if [ x$BR2_PACKAGE_GST_PLUGINS_BAD_SBC = xy ];then
	OPTS="$OPTS --enable-sbc"
else
	OPTS="$OPTS --disable-sbc"
fi

if [ x$BR2_PACKAGE_GST_PLUGINS_BAD_ZBAR = xy ];then
	OPTS="$OPTS --enable-zbar"
else
	OPTS="$OPTS --disable-zbar"
fi

if [ x$BR2_PACKAGE_GST_PLUGINS_BAD_RTMP = xy ];then
	OPTS="$OPTS --enable-rtmp"
else
	OPTS="$OPTS --disable-rtmp"
fi

if [ x$BR2_PACKAGE_GST_PLUGINS_BAD_SPANDSP = xy ];then
	OPTS="$OPTS --enable-spandsp"
else
	OPTS="$OPTS --disable-spandsp"
fi

if [ x$BR2_PACKAGE_GST_PLUGINS_BAD_HLS = xy ];then
	OPTS="$OPTS --enable-hls"
else
	OPTS="$OPTS --disable-hls"
fi

if [ x$BR2_PACKAGE_GST_PLUGINS_BAD_X265 = xy ];then
	OPTS="$OPTS --enable-x265"
else
	OPTS="$OPTS --disable-x265"
fi

if [ x$BR2_PACKAGE_GST_PLUGINS_BAD_WEBRTCDSP = xy ];then
	OPTS="$OPTS --enable-webrtcdsp"
else
	OPTS="$OPTS --disable-webrtcdsp"
fi

if [ x$BR2_PACKAGE_GST_PLUGINS_BAD_WEBRTC = xy ];then
	OPTS="$OPTS --enable-webrtc"
else
	OPTS="$OPTS --disable-webrtc"
fi
$SCRIPTS_DIR/build_pkgs.sh $ARCH $SUITE "$DEPENDENCIES"
echo "opts: $OPTS"
./configure $OPTS
make
make install
$SCRIPTS_DIR/fixlibtool.sh $TARGET_DIR $TARGET_DIR
cd -
