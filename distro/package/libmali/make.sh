#!/bin/bash

DEPENDENCIES="libdrm libegl1-mesa-dev libgles2-mesa-dev libgbm-dev libwayland-egl1"
$SCRIPTS_DIR/build_pkgs.sh $ARCH $SUITE "$DEPENDENCIES"
rm_so()
{
	rm -f $TARGET_DIR/usr/lib/$TOOLCHAIN/libmali*
	rm -f $TARGET_DIR/usr/lib/$TOOLCHAIN/libMali*
	rm -f $TARGET_DIR/usr/lib/$TOOLCHAIN/libEGL.so*
	rm -f $TARGET_DIR/usr/lib/$TOOLCHAIN/libgbm.so*
	rm -f $TARGET_DIR/usr/lib/$TOOLCHAIN/libGLESv1_CM.so*
	rm -f $TARGET_DIR/usr/lib/$TOOLCHAIN/libGLESv2.so*
	rm -f $TARGET_DIR/usr/lib/$TOOLCHAIN/libMaliOpenCL.so
	rm -f $TARGET_DIR/usr/lib/$TOOLCHAIN/libOpenCL.so
	rm -f $TARGET_DIR/usr/lib/$TOOLCHAIN/libwayland-egl.so*
}

link_opengl()
{
	ln -s libmali.so libMali.so
	ln -s libmali.so libEGL.so
	ln -s libmali.so libEGL.so.1
	ln -s libmali.so libgbm.so
	ln -s libmali.so libgbm.so.1
	ln -s libmali.so libGLESv1_CM.so
	ln -s libmali.so libGLESv1_CM.so.1
	ln -s libmali.so libGLESv2.so
	ln -s libmali.so libGLESv2.so.2
	ln -s libmali.so libwayland-egl.so
	ln -s libmali.so libwayland-egl.so.1
}

link_opencl()
{
	ln -s libmali.so libMaliOpenCL.so
	ln -s libmali.so libOpenCL.so
}

rm_so
cd $TARGET_DIR/usr/lib/$TOOLCHAIN
if [ $RK_TARGET_PRODUCT = rk3399 ];then
	install -m 0644 -D $TOP_DIR/external/libmali/lib/$TOOLCHAIN/libmali-midgard-t86x-r14p0-wayland.so $TARGET_DIR/usr/lib/$TOOLCHAIN/
	ln -s libmali-midgard-t86x-r14p0-wayland.so libmali.so
	link_opencl
elif [ $RK_TARGET_PRODUCT = rk3288 ];then
	install -m 0644 -D $TOP_DIR/external/libmali/lib/$TOOLCHAIN/libmali-midgard-t76x-r14p0-r0p0-wayland.so $TARGET_DIR/usr/lib/$TOOLCHAIN/
	install -m 0644 -D $TOP_DIR/external/libmali/lib/$TOOLCHAIN/libmali-midgard-t76x-r14p0-r1p0-wayland.so $TARGET_DIR/usr/lib/$TOOLCHAIN/
	install -m 0755 -D $TOP_DIR/external/libmali/overlay/S10libmali_rk3288 $TARGET_DIR/usr/bin/S10libmali
	link_opencl
elif [ $RK_TARGET_PRODUCT = rk3328 ];then
	install -m 0644 -D $TOP_DIR/external/libmali/lib/$TOOLCHAIN/libmali-utgard-450-r7p0-r0p0-wayland.so $TARGET_DIR/usr/lib/$TOOLCHAIN/
	ln -s libmali-utgard-450-r7p0-r0p0-wayland.so libmali.so
	link_opencl
elif [ $RK_TARGET_PRODUCT = px3se ];then
	install -m 0644 -D $TOP_DIR/external/libmali/lib/$TOOLCHAIN/libmali-utgard-400-r7p0-r3p0-wayland.so $TARGET_DIR/usr/lib/$TOOLCHAIN
	install -m 0755 -D $TOP_DIR/external/libmali/overlay/S10libmali_px3se $TARGET_DIR/usr/bin/S10libmali
	install -m 0755 -D $TOP_DIR/external/libmali/overlay/px3seBase $TARGET_DIR/usr/sbin/
	ln -s libmali-utgard-400-r7p0-r3p0-wayland.so libmali.so
elif [ $RK_TARGET_PRODUCT = rk3128 ];then
	install -m 0644 -D $TOP_DIR/external/libmali/lib/$TOOLCHAIN/libmali-utgard-400-r7p0-r1p1-wayland.so $TARGET_DIR/usr/lib/$TOOLCHAIN
	ln -s libmali-utgard-400-r7p0-r1p1-wayland.so
elif [ $RK_TARGET_PRODUCT = rk3326 ] || [ $RK_TARGET_PRODUCT == px30 ];then
	if [ $RK_ARCH = arm64 ];then
		install -m 0644 -D $TOP_DIR/external/libmali/lib/$TOOLCHAIN/libmali-bifrost-g31-rxp0-wayland-gbm.so $TARGET_DIR/usr/lib/$TOOLCHAIN
		ln -s libmali-bifrost-g31-rxp0-wayland-gbm.so libmali.so
		link_opencl
	elif [ $RK_ARCH = arm ];then
		install -m 0644 -D $TOP_DIR/external/libmali/lib/$TOOLCHAIN/libmali-bifrost-g31-rxp0-wayland-gbm.so $TARGET_DIR/usr/lib/$TOOLCHAIN
		ln -s libmali-bifrost-g31-rxp0-wayland-gbm.so libmali.so
		link_opencl
	fi
fi
link_opengl
cd -
