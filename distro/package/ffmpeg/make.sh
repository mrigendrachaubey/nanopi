#!/bin/bash
set -e
DEPENDENCIES=libglib2.0-dev
PKG=ffmpeg
mkdir -p $BUILD_DIR/$PKG
cd $BUILD_DIR/$PKG

OPTS="--prefix=/usr"

#  --logfile=FILE           log tests and output to FILE [ffbuild/config.log]
#  --disable-logging        do not log configure debug information
#  --fatal-warnings         fail if any configure warning is generated
#  --prefix=PREFIX          install in PREFIX [/usr/local]
#  --bindir=DIR             install binaries in DIR [PREFIX/bin]
#  --datadir=DIR            install data files in DIR [PREFIX/share/ffmpeg]
#  --docdir=DIR             install documentation in DIR [PREFIX/share/doc/ffmpeg]
#  --libdir=DIR             install libs in DIR [PREFIX/lib]
#  --shlibdir=DIR           install shared libs in DIR [LIBDIR]
#  --incdir=DIR             install includes in DIR [PREFIX/include]
#  --mandir=DIR             install man page in DIR [PREFIX/share/man]
#  --pkgconfigdir=DIR       install pkg-config files in DIR [LIBDIR/pkgconfig]
#  --enable-rpath           use rpath to allow installing libraries in paths
#  --install-name-dir=DIR   Darwin directory name for installed targets

if [ $ARCH = arm64 ];then
        OPTS="$OPTS --arch=aarch64"
elif [ $ARCH = arm ];then
	OPTS="$OPTS --arch=arm"
fi

OPTS="$OPTS \
  --cross-prefix=$CROSS_COMPILE \
  --enable-cross-compile \
  --sysroot=$SYSROOT \
  --sysinclude=$SYSROOT_DIR/usr/include:$SYSROOT_DIR/usr/include/$TOOLCHAIN \
  --target-os=linux"
#  --cpu=CPU
#  --progs-suffix=SUFFIX
#  --target-exec=CMD        command to run executables on target
#  --target-path=DIR        path to view of build directory on target
#  --target-samples=DIR     path to samples directory on target
#  --tempprefix=PATH        force fixed dir/prefix instead of mktemp for checks
#  --toolchain=NAME
#  --nm=NM                  use nm tool NM [nm -g]
#  --ar=AR                  use archive tool AR [ar]
#  --as=AS                  use assembler AS []
#  --ln_s=LN_S              use symbolic link tool LN_S [ln -s -f]
#  --strip=STRIP            use strip tool STRIP [strip]
#  --windres=WINDRES        use windows resource compiler WINDRES [windres]
#  --x86asmexe=EXE          use nasm-compatible assembler EXE [nasm]
#  --cc=CC                  use C compiler CC [gcc]
#  --cxx=CXX                use C compiler CXX [g++]
#  --objcc=OCC              use ObjC compiler OCC [gcc]
#  --dep-cc=DEPCC           use dependency generator DEPCC [gcc]
#  --nvcc=NVCC              use Nvidia CUDA compiler NVCC [nvcc]
#  --ld=LD                  use linker LD []
#  --pkg-config=PKGCONFIG   use pkg-config tool PKGCONFIG [pkg-config]
#  --pkg-config-flags=FLAGS pass additional flags to pkgconf []
#  --ranlib=RANLIB          use ranlib RANLIB [ranlib]
#  --doxygen=DOXYGEN        use DOXYGEN to generate API doc [doxygen]
#  --host-cc=HOSTCC         use host C compiler HOSTCC
#  --host-cflags=HCFLAGS    use HCFLAGS when compiling for host
#  --host-cppflags=HCPPFLAGS use HCPPFLAGS when compiling for host
#  --host-ld=HOSTLD         use host linker HOSTLD
#  --host-ldflags=HLDFLAGS  use HLDFLAGS when linking for host
#  --host-libs=HLIBS        use libs HLIBS when linking for host
#  --host-os=OS             compiler host OS []
#  --extra-cflags=ECFLAGS   add ECFLAGS to CFLAGS []
#  --extra-cxxflags=ECFLAGS add ECFLAGS to CXXFLAGS []
#  --extra-objcflags=FLAGS  add FLAGS to OBJCFLAGS []
#  --extra-ldflags=ELDFLAGS add ELDFLAGS to LDFLAGS []
#  --extra-ldexeflags=ELDFLAGS add ELDFLAGS to LDEXEFLAGS []
#  --extra-ldsoflags=ELDFLAGS add ELDFLAGS to LDSOFLAGS []
#  --extra-libs=ELIBS       add ELIBS []
#  --extra-version=STRING   version string suffix []
#  --optflags=OPTFLAGS      override optimization-related compiler flags
#  --nvccflags=NVCCFLAGS    override nvcc flags [-gencode arch=compute_30,code=sm_30 -O2]
#  --build-suffix=SUFFIX    library name suffix []
#  --enable-pic             build position-independent code
#  --enable-thumb           compile for Thumb instruction set
#  --enable-lto             use link-time optimization
#  --env="ENV=override"     override the environment variables

  OPTS="$OPTS \
  --enable-gpl \
  --enable-version3 \
  --enable-nonfree"

OPTS="$OPTS \
  --disable-static \
  --enable-shared \
  --disable-small \
  --enable-runtime-cpudetect \
  --disable-gray \
  --enable-swscale-alpha"
#  --disable-all            disable building components, libraries and programs
#  --disable-autodetect     disable automatically detected external libraries [no]

#  --disable-programs       do not build command line programs
OPTS="$OPTS \
  --enable-ffmpeg \
  --enable-ffplay \
  --enable-ffprobe"

OPTS="$OPTS \
  --disable-doc \
  --disable-htmlpages \
  --disable-manpages \
  --disable-podpages \
  --disable-txtpages"

OPTS="$OPTS \
  --enable-avdevice \
  --enable-avcodec \
  --enable-avformat \
  --enable-swresample \
  --enable-swscale \
  --disable-postproc \
  --enable-avfilter \
  --disable-avresample \
  --enable-pthreads \
  --disable-w32threads \
  --disable-os2threads \
  --enable-network \
  --enable-dct \
  --disable-dwt \
  --disable-error-resilience \
  --disable-lsp \
  --disable-lzo \
  --enable-mdct \
  --enable-rdft \
  --enable-fft \
  --disable-faan \
  --disable-pixelutils"




OPTS="$OPTS \
  --enable-asm \
  --disable-altivec \
  --disable-vsx \
  --disable-power8 \
  --disable-amd3dnow \
  --disable-amd3dnowext \
  --disable-mmx \
  --disable-mmxext \
  --disable-sse \
  --disable-sse2 \
  --disable-sse3 \
  --disable-ssse3 \
  --disable-sse4 \
  --disable-sse42 \
  --disable-avx \
  --disable-xop \
  --disable-fma3 \
  --disable-fma4 \
  --disable-avx2 \
  --disable-avx512 \
  --disable-aesni \
  --enable-armv5te \
  --enable-armv6 \
  --enable-armv6t2 \
  --enable-vfp \
  --enable-neon \
  --disable-inline-asm \
  --disable-x86asm \
  --disable-mipsdsp \
  --disable-mipsdspr2 \
  --disable-msa \
  --disable-mipsfpu \
  --disable-mmi \
  --enable-fast-unaligned"

if [ x$BR2_PACKAGE_FFMPEG_ALSA = xy ];then
        OPTS="$OPTS --enable-alsa"
else
        OPTS="$OPTS --disable-alsa"
fi

if [ x$BR2_PACKAGE_FFMPEG_APPKIT = xy ];then
        OPTS="$OPTS --enable-appkit"
else
        OPTS="$OPTS --disable-appkit"
fi

if [ x$BR2_PACKAGE_FFMPEG_AVFOUNDATION = xy ];then
        OPTS="$OPTS --enable-avfoundation"
else
        OPTS="$OPTS --disable-avfoundation"
fi

if [ x$BR2_PACKAGE_FFMPEG_AVISYNTH = xy ];then
        OPTS="$OPTS --enable-avisynth"
else
        OPTS="$OPTS --disable-avisynth"
fi

if [ x$BR2_PACKAGE_FFMPEG_BZLIB = xy ];then
        OPTS="$OPTS --enable-bzlib"
else
        OPTS="$OPTS --disable-bzlib"
fi

if [ x$BR2_PACKAGE_FFMPEG_COREIMAGE = xy ];then
        OPTS="$OPTS --enable-coreimage"
else
        OPTS="$OPTS --disable-coreimage"
fi

if [ x$BR2_PACKAGE_FFMPEG_CHROMAPRINT = xy ];then
        OPTS="$OPTS --enable-chromaprint"
else
        OPTS="$OPTS --disable-chromaprint"
fi

if [ x$BR2_PACKAGE_FFMPEG_FREI0R = xy ];then
        OPTS="$OPTS --enable-frei0r"
else
        OPTS="$OPTS --disable-frei0r"
fi

if [ x$BR2_PACKAGE_FFMPEG_GCRYPT = xy ];then
        OPTS="$OPTS --enable-gcrypt"
else
        OPTS="$OPTS --disable-gcrypt"
fi

if [ x$BR2_PACKAGE_FFMPEG_GMP = xy ];then
        OPTS="$OPTS --enable-gmp"
else
        OPTS="$OPTS --disable-gmp"
fi

if [ x$BR2_PACKAGE_FFMPEG_GNUTLS = xy ];then
        OPTS="$OPTS --enable-gnutls"
else
        OPTS="$OPTS --disable-gnutls"
fi

if [ x$BR2_PACKAGE_FFMPEG_ICONV = xy ];then
        OPTS="$OPTS --enable-iconv"
else
        OPTS="$OPTS --disable-iconv"
fi

if [ x$BR2_PACKAGE_FFMPEG_JNI = xy ];then
        OPTS="$OPTS --enable-jni"
else
        OPTS="$OPTS --disable-jni"
fi

if [ x$BR2_PACKAGE_FFMPEG_LADSPA = xy ];then
        OPTS="$OPTS --enable-ladspa"
else
        OPTS="$OPTS --disable-ladspa"
fi

if [ x$BR2_PACKAGE_FFMPEG_LIBAOM = xy ];then
        OPTS="$OPTS --enable-libaom"
else
        OPTS="$OPTS --disable-libaom"
fi

if [ x$BR2_PACKAGE_FFMPEG_LIBASS = xy ];then
        OPTS="$OPTS --enable-libass"
else
        OPTS="$OPTS --disable-libass"
fi

if [ x$BR2_PACKAGE_FFMPEG_BLURAY = xy ];then
        OPTS="$OPTS --enable-libbluray"
else
        OPTS="$OPTS --disable-libbluray"
fi

if [ x$BR2_PACKAGE_FFMPEG_LIBBS2B = xy ];then
        OPTS="$OPTS --enable-libbs2b"
else
        OPTS="$OPTS --disable-libbs2b"
fi

if [ x$BR2_PACKAGE_FFMPEG_LIBCACA = xy ];then
        OPTS="$OPTS --enable-libcaca"
else
        OPTS="$OPTS --disable-libcaca"
fi

if [ x$BR2_PACKAGE_FFMPEG_LIBCELT = xy ];then
        OPTS="$OPTS --enable-libcelt"
else
        OPTS="$OPTS --disable-libcelt"
fi

if [ x$BR2_PACKAGE_FFMPEG_LIBCDIO = xy ];then
        OPTS="$OPTS --enable-libcdio"
else
        OPTS="$OPTS --disable-libcdio"
fi

if [ x$BR2_PACKAGE_FFMPEG_LIBCODEC2 = xy ];then
        OPTS="$OPTS --enable-libcodec2"
else
        OPTS="$OPTS --disable-libcodec2"
fi

if [ x$BR2_PACKAGE_FFMPEG_LIBDC1394 = xy ];then
        OPTS="$OPTS --enable-libdc1394"
else
        OPTS="$OPTS --disable-libdc1394"
fi

if [ x$BR2_PACKAGE_FFMPEG_LIBFDK_AAC = xy ];then
        OPTS="$OPTS --enable-libfdk-aac"
else
        OPTS="$OPTS --disable-libfdk-aac"
fi

if [ x$BR2_PACKAGE_FFMPEG_LIBFLITE = xy ];then
        OPTS="$OPTS --enable-libflite"
else
        OPTS="$OPTS --disable-libflite"
fi

if [ x$BR2_PACKAGE_FFMPEG_LIBFONTCONFIG = xy ];then
        OPTS="$OPTS --enable-libfontconfig"
else
        OPTS="$OPTS --disable-libfontconfig"
fi

if [ x$BR2_PACKAGE_FFMPEG_LIBFREETYPE = xy ];then
        OPTS="$OPTS --enable-libfreetype"
else
        OPTS="$OPTS --disable-libfreetype"
fi

if [ x$BR2_PACKAGE_FFMPEG_LIBFRIBIDI = xy ];then
        OPTS="$OPTS --enable-libfribidi"
else
        OPTS="$OPTS --disable-libfribidi"
fi

if [ x$BR2_PACKAGE_FFMPEG_LIBGME = xy ];then
        OPTS="$OPTS --enable-libgme"
else
        OPTS="$OPTS --disable-libgme"
fi

if [ x$BR2_PACKAGE_FFMPEG_LIBGSM = xy ];then
        OPTS="$OPTS --enable-libgsm"
else
        OPTS="$OPTS --disable-libgsm"
fi

if [ x$BR2_PACKAGE_FFMPEG_LIBIEC61883 = xy ];then
        OPTS="$OPTS --enable-libiec61883"
else
        OPTS="$OPTS --disable-libiec61883"
fi

if [ x$BR2_PACKAGE_FFMPEG_LIBILBC = xy ];then
        OPTS="$OPTS --enable-libilbc"
else
        OPTS="$OPTS --disable-libilbc"
fi

if [ x$BR2_PACKAGE_FFMPEG_LIBJACK = xy ];then
        OPTS="$OPTS --enable-libjack"
else
        OPTS="$OPTS --disable-libjack"
fi

if [ x$BR2_PACKAGE_FFMPEG_LIBKVAZAAR = xy ];then
        OPTS="$OPTS --enable-libkvazaar"
else
        OPTS="$OPTS --disable-libkvazaar"
fi

if [ x$BR2_PACKAGE_FFMPEG_LIBMODPLUG = xy ];then
        OPTS="$OPTS --enable-libmodplug"
else
        OPTS="$OPTS --disable-libmodplug"
fi

if [ x$BR2_PACKAGE_FFMPEG_LIBMP3LAME = xy ];then
        OPTS="$OPTS --enable-libmp3lame"
else
        OPTS="$OPTS --disable-libmp3lame"
fi

if [ x$BR2_PACKAGE_FFMPEG_LIBOPENCORE_AMRNB = xy ];then
        OPTS="$OPTS --enable-libopencore-amrnb"
else
        OPTS="$OPTS --disable-libopencore-amrnb"
fi

if [ x$BR2_PACKAGE_FFMPEG_LIBOPENCORE_AMRWB = xy ];then
        OPTS="$OPTS --enable-libopencore-amrwb"
else
        OPTS="$OPTS --disable-libopencore-amrwb"
fi

if [ x$BR2_PACKAGE_FFMPEG_LIBOPENCV = xy ];then
        OPTS="$OPTS --enable-libopencv"
else
        OPTS="$OPTS --disable-libopencv"
fi

if [ x$BR2_PACKAGE_FFMPEG_LIBOPENH264 = xy ];then
        OPTS="$OPTS --enable-libopenh264"
else
        OPTS="$OPTS --disable-libopenh264"
fi

if [ x$BR2_PACKAGE_FFMPEG_LIBOPENJPEG = xy ];then
        OPTS="$OPTS --enable-libopenjpeg"
else
        OPTS="$OPTS --disable-libopenjpeg"
fi

if [ x$BR2_PACKAGE_FFMPEG_LIBOPENMPT = xy ];then
        OPTS="$OPTS --enable-libopenmpt"
else
        OPTS="$OPTS --disable-libopenmpt"
fi

if [ x$BR2_PACKAGE_FFMPEG_LIBOPUS = xy ];then
        OPTS="$OPTS --enable-libopus"
else
        OPTS="$OPTS --disable-libopus"
fi

if [ x$BR2_PACKAGE_FFMPEG_LIBPULSE = xy ];then
        OPTS="$OPTS --enable-libpulse"
else
        OPTS="$OPTS --disable-libpulse"
fi

if [ x$BR2_PACKAGE_FFMPEG_LIBRSVG = xy ];then
        OPTS="$OPTS --enable-librsvg"
else
        OPTS="$OPTS --disable-librsvg"
fi

if [ x$BR2_PACKAGE_FFMPEG_LIBRUBBERBAND = xy ];then
        OPTS="$OPTS --enable-librubberband"
else
        OPTS="$OPTS --disable-librubberband"
fi

if [ x$BR2_PACKAGE_FFMPEG_LIBRTMP = xy ];then
        OPTS="$OPTS --enable-librtmp"
else
        OPTS="$OPTS --disable-librtmp"
fi

if [ x$BR2_PACKAGE_FFMPEG_LIBSHINE = xy ];then
        OPTS="$OPTS --enable-libshine"
else
        OPTS="$OPTS --disable-libshine"
fi

if [ x$BR2_PACKAGE_FFMPEG_LIBSMBCLIENT = xy ];then
        OPTS="$OPTS --enable-libsmbclient"
else
        OPTS="$OPTS --disable-libsmbclient"
fi

if [ x$BR2_PACKAGE_FFMPEG_LIBSNAPPY = xy ];then
        OPTS="$OPTS --enable-libsnappy"
else
        OPTS="$OPTS --disable-libsnappy"
fi

if [ x$BR2_PACKAGE_FFMPEG_LIBSOXR = xy ];then
        OPTS="$OPTS --enable-libsoxr"
else
        OPTS="$OPTS --disable-libsoxr"
fi

if [ x$BR2_PACKAGE_FFMPEG_LIBSPEEX = xy ];then
        OPTS="$OPTS --enable-libspeex"
else
        OPTS="$OPTS --disable-libspeex"
fi

if [ x$BR2_PACKAGE_FFMPEG_LIBSRT = xy ];then
        OPTS="$OPTS --enable-libsrt"
else
        OPTS="$OPTS --disable-libsrt"
fi

if [ x$BR2_PACKAGE_FFMPEG_LIBSSH = xy ];then
        OPTS="$OPTS --enable-libssh"
else
        OPTS="$OPTS --disable-libssh"
fi

if [ x$BR2_PACKAGE_FFMPEG_LIBTESSERACT = xy ];then
        OPTS="$OPTS --enable-libtesseract"
else
        OPTS="$OPTS --disable-libtesseract"
fi

if [ x$BR2_PACKAGE_FFMPEG_LIBTHEORA = xy ];then
        OPTS="$OPTS --enable-libtheora"
else
        OPTS="$OPTS --disable-libtheora"
fi

if [ x$BR2_PACKAGE_FFMPEG_LIBTLS = xy ];then
        OPTS="$OPTS --enable-libtls"
else
        OPTS="$OPTS --disable-libtls"
fi

if [ x$BR2_PACKAGE_FFMPEG_LIBTWOLAME = xy ];then
        OPTS="$OPTS --enable-libtwolame"
else
        OPTS="$OPTS --disable-libtwolame"
fi

if [ x$BR2_PACKAGE_FFMPEG_LIBV4L2 = xy ];then
        OPTS="$OPTS --enable-libv4l2"
else
        OPTS="$OPTS --disable-libv4l2"
fi

if [ x$BR2_PACKAGE_FFMPEG_LIBVIDSTAB = xy ];then
        OPTS="$OPTS --enable-libvidstab"
else
        OPTS="$OPTS --disable-libvidstab"
fi

if [ x$BR2_PACKAGE_FFMPEG_LIBVMAF = xy ];then
        OPTS="$OPTS --enable-libvmaf"
else
        OPTS="$OPTS --disable-libvmaf"
fi

if [ x$BR2_PACKAGE_FFMPEG_LIBVO_AMRWBENC = xy ];then
        OPTS="$OPTS --enable-libvo-amrwbenc"
else
        OPTS="$OPTS --disable-libvo-amrwbenc"
fi

if [ x$BR2_PACKAGE_FFMPEG_LIBVORBIS = xy ];then
        OPTS="$OPTS --enable-libvorbis"
else
        OPTS="$OPTS --disable-libvorbis"
fi

if [ x$BR2_PACKAGE_FFMPEG_LIBVPX = xy ];then
        OPTS="$OPTS --enable-libvpx"
else
        OPTS="$OPTS --disable-libvpx"
fi

if [ x$BR2_PACKAGE_FFMPEG_LIBWAVPACK = xy ];then
        OPTS="$OPTS --enable-libwavpack"
else
        OPTS="$OPTS --disable-libwavpack"
fi

if [ x$BR2_PACKAGE_FFMPEG_LIBWEBP = xy ];then
        OPTS="$OPTS --enable-libwebp"
else
        OPTS="$OPTS --disable-libwebp"
fi

if [ x$BR2_PACKAGE_FFMPEG_LIBX264 = xy ];then
        OPTS="$OPTS --enable-libx264"
else
        OPTS="$OPTS --disable-libx264"
fi

if [ x$BR2_PACKAGE_FFMPEG_LIBX265 = xy ];then
        OPTS="$OPTS --enable-libx265"
else
        OPTS="$OPTS --disable-libx265"
fi

if [ x$BR2_PACKAGE_FFMPEG_LIBXAVS = xy ];then
        OPTS="$OPTS --enable-libxavs"
else
        OPTS="$OPTS --disable-libxavs"
fi

if [ x$BR2_PACKAGE_FFMPEG_LIBXCB = xy ];then
        OPTS="$OPTS --enable-libxcb"
else
        OPTS="$OPTS --disable-libxcb"
fi

if [ x$BR2_PACKAGE_FFMPEG_LIBXCB_SHM = xy ];then
        OPTS="$OPTS --enable-libxcb-shm"
else
        OPTS="$OPTS --disable-libxcb-shm"
fi

if [ x$BR2_PACKAGE_FFMPEG_LIBXCB_XFIXES = xy ];then
        OPTS="$OPTS --enable-libxcb-xfixes"
else
        OPTS="$OPTS --disable-libxcb-xfixes"
fi

if [ x$BR2_PACKAGE_FFMPEG_LIBXCB_SHAPE = xy ];then
        OPTS="$OPTS --enable-libxcb-shape"
else
        OPTS="$OPTS --disable-libxcb-shape"
fi

if [ x$BR2_PACKAGE_FFMPEG_LIBXVID = xy ];then
        OPTS="$OPTS --enable-libxvid"
else
        OPTS="$OPTS --disable-libxvid"
fi

if [ x$BR2_PACKAGE_FFMPEG_LIBXML2 = xy ];then
        OPTS="$OPTS --enable-libxml2"
else
        OPTS="$OPTS --disable-libxml2"
fi

if [ x$BR2_PACKAGE_FFMPEG_LIBZIMG = xy ];then
        OPTS="$OPTS --enable-libzimg"
else
        OPTS="$OPTS --disable-libzimg"
fi

if [ x$BR2_PACKAGE_FFMPEG_LIBZMQ = xy ];then
        OPTS="$OPTS --enable-libzmq"
else
        OPTS="$OPTS --disable-libzmq"
fi

if [ x$BR2_PACKAGE_FFMPEG_LIBZVBI = xy ];then
        OPTS="$OPTS --enable-libzvbi"
else
        OPTS="$OPTS --disable-libzvbi"
fi

if [ x$BR2_PACKAGE_FFMPEG_LV2 = xy ];then
        OPTS="$OPTS --enable-lv2"
else
        OPTS="$OPTS --disable-lv2"
fi

if [ x$BR2_PACKAGE_FFMPEG_LZMA = xy ];then
        OPTS="$OPTS --enable-lzma"
else
        OPTS="$OPTS --disable-lzma"
fi

if [ x$BR2_PACKAGE_FFMPEG_DECKLINK = xy ];then
        OPTS="$OPTS --enable-decklink"
else
        OPTS="$OPTS --disable-decklink"
fi

if [ x$BR2_PACKAGE_FFMPEG_LIBNDI_NEWTEK = xy ];then
        OPTS="$OPTS --enable-libndi_newtek"
else
        OPTS="$OPTS --disable-libndi_newtek"
fi

if [ x$BR2_PACKAGE_FFMPEG_MEDIACODEC = xy ];then
        OPTS="$OPTS --enable-mediacodec"
else
	OPTS="$OPTS --disable-mediacodec"
fi

if [ x$BR2_PACKAGE_FFMPEG_LIBMYSOFA = xy ];then
        OPTS="$OPTS --enable-libmysofa"
else
        OPTS="$OPTS --disable-libmysofa"
fi

if [ x$BR2_PACKAGE_FFMPEG_OPENAL = xy ];then
        OPTS="$OPTS --enable-openal"
else
        OPTS="$OPTS --disable-openal"
fi

if [ x$BR2_PACKAGE_FFMPEG_OPENCL = xy ];then
        OPTS="$OPTS --enable-opencl"
else
        OPTS="$OPTS --disable-opencl"
fi

if [ x$BR2_PACKAGE_FFMPEG_OPENGL = xy ];then
        OPTS="$OPTS --enable-opengl"
else
        OPTS="$OPTS --disable-opengl"
fi

if [ x$BR2_PACKAGE_FFMPEG_OPENSSL = xy ];then
        OPTS="$OPTS --enable-openssl"
else
        OPTS="$OPTS --disable-openssl"
fi

if [ x$BR2_PACKAGE_FFMPEG_SNDIO = xy ];then
        OPTS="$OPTS --enable-sndio"
else
        OPTS="$OPTS --disable-sndio"
fi

if [ x$BR2_PACKAGE_FFMPEG_SCHANNEL = xy ];then
        OPTS="$OPTS --enable-schannel"
else
        OPTS="$OPTS --disable-schannel"
fi

if [ x$BR2_PACKAGE_FFMPEG_SDL2 = xy ];then
        OPTS="$OPTS --enable-sdl2"
else
        OPTS="$OPTS --disable-sdl2"
fi

if [ x$BR2_PACKAGE_FFMPEG_SECURETRANSPORT = xy ];then
        OPTS="$OPTS --enable-securetransport"
else
        OPTS="$OPTS --disable-securetransport"
fi

if [ x$BR2_PACKAGE_FFMPEG_XLIB = xy ];then
        OPTS="$OPTS --enable-xlib"
else
        OPTS="$OPTS --disable-xlib"
fi

if [ x$BR2_PACKAGE_FFMPEG_ZLIB = xy ];then
        OPTS="$OPTS --enable-zlib"
else
        OPTS="$OPTS --disable-zlib"
fi

if [ x$BR2_PACKAGE_FFMPEG_AMF = xy ];then
        OPTS="$OPTS --enable-amf"
else
        OPTS="$OPTS --disable-amf"
fi

if [ x$BR2_PACKAGE_FFMPEG_AUDIOTOOLBOX = xy ];then
        OPTS="$OPTS --enable-audiotoolbox"
else
        OPTS="$OPTS --disable-audiotoolbox"
fi

if [ x$BR2_PACKAGE_FFMPEG_CUDA = xy ];then
        OPTS="$OPTS --enable-cuda-sdk"
else
        OPTS="$OPTS --disable-cuda-sdk"
fi

if [ x$BR2_PACKAGE_FFMPEG_CUVID = xy ];then
        OPTS="$OPTS --enable-cuvid"
else
        OPTS="$OPTS --disable-cuvid"
fi

if [ x$BR2_PACKAGE_FFMPEG_D3D11VA = xy ];then
        OPTS="$OPTS --enable-d3d11va"
else
        OPTS="$OPTS --disable-d3d11va"
fi

if [ x$BR2_PACKAGE_FFMPEG_DXVA2 = xy ];then
        OPTS="$OPTS --enable-dxva2"
else
        OPTS="$OPTS --disable-dxva2"
fi

if [ x$BR2_PACKAGE_FFMPEG_FFNVCODEC = xy ];then
        OPTS="$OPTS --enable-ffnvcodec"
else
        OPTS="$OPTS --disable-ffnvcodec"
fi

if [ x$BR2_PACKAGE_FFMPEG_LIBDRM = xy ];then
        OPTS="$OPTS --enable-libdrm"
else
        OPTS="$OPTS --disable-libdrm"
fi

if [ x$BR2_PACKAGE_FFMPEG_LIBMFX = xy ];then
        OPTS="$OPTS --enable-libmfx"
else
        OPTS="$OPTS --disable-libmfx"
fi

if [ x$BR2_PACKAGE_FFMPEG_LIBNPP = xy ];then
        OPTS="$OPTS --enable-libnpp"
else
        OPTS="$OPTS --disable-libnpp"
fi

if [ x$BR2_PACKAGE_FFMPEG_MMAL = xy ];then
        OPTS="$OPTS --enable-mmal"
else
        OPTS="$OPTS --disable-mmal"
fi

if [ x$BR2_PACKAGE_FFMPEG_NVDEC = xy ];then
        OPTS="$OPTS --enable-nvdec"
else
        OPTS="$OPTS --disable-nvdec"
fi

if [ x$BR2_PACKAGE_FFMPEG_NVENC = xy ];then
        OPTS="$OPTS --enable-nvenc"
else
        OPTS="$OPTS --disable-nvenc"
fi

if [ x$BR2_PACKAGE_FFMPEG_OMX = xy ];then
        OPTS="$OPTS --enable-omx"
else
        OPTS="$OPTS --disable-omx"
fi

if [ x$BR2_PACKAGE_FFMPEG_OMX_RPI = xy ];then
        OPTS="$OPTS --enable-omx-rpi"
else
        OPTS="$OPTS --disable-omx-rpi"
fi

if [ x$BR2_PACKAGE_FFMPEG_RKMPP = xy ];then
        OPTS="$OPTS --enable-rkmpp"
else
        OPTS="$OPTS --disable-rkmpp"
fi

if [ x$BR2_PACKAGE_FFMPEG_V4L2_M2M = xy ];then
        OPTS="$OPTS --enable-v4l2-m2m"
else
        OPTS="$OPTS --disable-v4l2-m2m"
fi

if [ x$BR2_PACKAGE_FFMPEG_VAAPI = xy ];then
        OPTS="$OPTS --enable-vaapi"
else
        OPTS="$OPTS --disable-vaapi"
fi

if [ x$BR2_PACKAGE_FFMPEG_VDPAU = xy ];then
        OPTS="$OPTS --enable-vdpau"
else
        OPTS="$OPTS --disable-vdpau"
fi

if [ x$BR2_PACKAGE_FFMPEG_VIDEOTOOLBOX = xy ];then
        OPTS="$OPTS --enable-videotoolbox"
else
        OPTS="$OPTS --disable-videotoolbox"
fi

$SCRIPTS_DIR/build_pkgs.sh $ARCH $SUITE "$DEPENDENCIES"
echo $OPTS
$TOP_DIR/external/ffmpeg/configure $OPTS
make
make install

cd -
