IS_ANDROID_OS = false
IS_NEED_SHARED_PTR = false
IS_NEED_COMPILE_STLPORT = false
IS_NEED_LINK_STLPORT = false
IS_NEED_COMPILE_TINYXML2 = true
IS_NEED_COMPILE_EXPAT = true
IS_SUPPORT_ION = true
IS_SUPPORT_DMABUF = false
IS_BUILD_TEST_APP = true
IS_BUILD_DUMPSYS = true
IS_CAM_IA10_API = false
IS_RK_ISP10 = false
IS_USE_RK_V4L2_HEAD = true

#loadxml, loadcode, dumpfile
ifdef IQ_BIN_NAME
IQDATA_MODE = loadcode
else
IQDATA_MODE = loadxml
endif

#BUILD_TARGET = rk3288
#BUILD_TARGET = rk3399
BUILD_TARGET = rv1108
#BUILD_TARGET = rv1108_fastboot
BUILD_OUPUT_EXTERNAL_LIBS:=$(CURDIR)/../../../../out/system/lib/
CROSS_COMPILE ?= $(CURDIR)/../../../../prebuilts/toolschain/usr/bin/arm-linux-
BASE_DIR ?= $(CURDIR)/../../../../out
TARGET_DIR ?= $(BASE_DIR)/system
#CROSS_COMPILE ?= /extra/zyc/android6.0/prebuilts/gcc/linux-x86/arm/arm-eabi-4.6/bin/arm-eabi-
#CROSS_COMPILE ?=
