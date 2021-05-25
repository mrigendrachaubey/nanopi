# Makefile for buildroot
#
# Copyright (C) 1999-2005 by Erik Andersen <andersen@codepoet.org>
# Copyright (C) 2006-2014 by the Buildroot developers <buildroot@uclibc.org>
# Copyright (C) 2014-2019 by the Buildroot developers <buildroot@buildroot.org>
# Copyright (C) 2019 by the Jeffy Chen <jeffy.chen@rock-chips.com>
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
# General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
#

# Delete default rules. We don't use them. This saves a bit of time.
.SUFFIXES:

# we want bash as shell
SHELL := $(shell if [ -x "$$BASH" ]; then echo $$BASH; \
	 else if [ -x /bin/bash ]; then echo /bin/bash; \
	 else echo sh; fi; fi)

# Set O variable if not already done on the command line;
# or avoid confusing packages that can use the O=<dir> syntax for out-of-tree
# build by preventing it from being forwarded to sub-make calls.
ifneq ("$(origin O)", "command line")
ifneq ($(OUTPUT_DIR),)
O := $(OUTPUT_DIR)
else
O := $(CURDIR)/output
endif
endif

# This is our default rule, so must come first
all:
.PHONY: all

TOPDIR := $(CURDIR)
CONFIG_CONFIG_IN = Config.in
CONFIG = support/kconfig
DATE := $(shell date +%Y%m%d)

# List of targets and target patterns for which .config doesn't need to be read in
noconfig_targets := menuconfig nconfig gconfig xconfig config oldconfig randconfig \
	defconfig %_defconfig allyesconfig allnoconfig alldefconfig syncconfig olddefconfig distclean

CONFIG_DIR := $(O)
BUILD_DIR ?= $(O)/build

# bash prints the name of the directory on 'cd <dir>' if CDPATH is
# set, so unset it here to not cause problems. Notice that the export
# line doesn't affect the environment of $(shell ..) calls.
export CDPATH :=

BR2_CONFIG = $(CONFIG_DIR)/.config

# To put more focus on warnings, be less verbose as default
# Use 'make V=1' to see the full commands
ifeq ("$(origin V)", "command line")
  KBUILD_VERBOSE = $(V)
endif
ifndef KBUILD_VERBOSE
  KBUILD_VERBOSE = 0
endif

ifeq ($(KBUILD_VERBOSE),1)
  Q =
ifndef VERBOSE
  VERBOSE = 1
endif
export VERBOSE
else
  Q = @
endif

# kconfig uses CONFIG_SHELL
CONFIG_SHELL := $(SHELL)

export SHELL CONFIG_SHELL Q KBUILD_VERBOSE

ifndef HOSTAR
HOSTAR := ar
endif
ifndef HOSTAS
HOSTAS := as
endif
ifndef HOSTCC
HOSTCC := gcc
HOSTCC := $(shell which $(HOSTCC) || type -p $(HOSTCC) || echo gcc)
endif
HOSTCC_NOCCACHE := $(HOSTCC)
ifndef HOSTCXX
HOSTCXX := g++
HOSTCXX := $(shell which $(HOSTCXX) || type -p $(HOSTCXX) || echo g++)
endif
HOSTCXX_NOCCACHE := $(HOSTCXX)
ifndef HOSTCPP
HOSTCPP := cpp
endif
ifndef HOSTLD
HOSTLD := ld
endif
ifndef HOSTLN
HOSTLN := ln
endif
ifndef HOSTNM
HOSTNM := nm
endif
ifndef HOSTOBJCOPY
HOSTOBJCOPY := objcopy
endif
ifndef HOSTRANLIB
HOSTRANLIB := ranlib
endif
HOSTAR := $(shell which $(HOSTAR) || type -p $(HOSTAR) || echo ar)
HOSTAS := $(shell which $(HOSTAS) || type -p $(HOSTAS) || echo as)
HOSTCPP := $(shell which $(HOSTCPP) || type -p $(HOSTCPP) || echo cpp)
HOSTLD := $(shell which $(HOSTLD) || type -p $(HOSTLD) || echo ld)
HOSTLN := $(shell which $(HOSTLN) || type -p $(HOSTLN) || echo ln)
HOSTNM := $(shell which $(HOSTNM) || type -p $(HOSTNM) || echo nm)
HOSTOBJCOPY := $(shell which $(HOSTOBJCOPY) || type -p $(HOSTOBJCOPY) || echo objcopy)
HOSTRANLIB := $(shell which $(HOSTRANLIB) || type -p $(HOSTRANLIB) || echo ranlib)
SED := $(shell which sed || type -p sed) -i -e

export HOSTAR HOSTAS HOSTCC HOSTCXX HOSTLD
export HOSTCC_NOCCACHE HOSTCXX_NOCCACHE

# Determine the userland we are running on.
#
# Note that, despite its name, we are not interested in the actual
# architecture name. This is mostly used to determine whether some
# of the binary tools (e.g. pre-built external toolchains) can run
# on the current host. So we need to know if the userland we're
# running on can actually run those toolchains.
#
# For example, a 64-bit prebuilt toolchain will not run on a 64-bit
# kernel if the userland is 32-bit (e.g. in a chroot for example).
#
# So, we extract the first part of the tuple the host gcc was
# configured to generate code for; we assume this is our userland.
#
export HOSTARCH := $(shell LC_ALL=C $(HOSTCC_NOCCACHE) -v 2>&1 | \
	sed -e '/^Target: \([^-]*\).*/!d' \
	    -e 's//\1/' \
	    -e 's/i.86/x86/' \
	    -e 's/sun4u/sparc64/' \
	    -e 's/arm.*/arm/' \
	    -e 's/sa110/arm/' \
	    -e 's/ppc64/powerpc64/' \
	    -e 's/ppc/powerpc/' \
	    -e 's/macppc/powerpc/' \
	    -e 's/sh.*/sh/' )

# When adding a new host gcc version in Config.in,
# update the HOSTCC_MAX_VERSION variable:
HOSTCC_MAX_VERSION := 8

HOSTCC_VERSION := $(shell V=$$($(HOSTCC_NOCCACHE) --version | \
	sed -n -r 's/^.* ([0-9]*)\.([0-9]*)\.([0-9]*)[ ]*.*/\1 \2/p'); \
	[ "$${V%% *}" -le $(HOSTCC_MAX_VERSION) ] || V=$(HOSTCC_MAX_VERSION); \
	printf "%s" "$${V}")

# For gcc >= 5.x, we only need the major version.
ifneq ($(firstword $(HOSTCC_VERSION)),4)
HOSTCC_VERSION := $(firstword $(HOSTCC_VERSION))
endif

# configuration
# ---------------------------------------------------------------------------

HOSTCFLAGS = $(CFLAGS_FOR_BUILD)
export HOSTCFLAGS

.PHONY: prepare-kconfig
prepare-kconfig:

$(BUILD_DIR)/buildroot-config/%onf:
	mkdir -p $(@D)/lxdialog
	PKG_CONFIG_PATH="$(HOST_PKG_CONFIG_PATH)" $(MAKE) CC="$(HOSTCC_NOCCACHE)" HOSTCC="$(HOSTCC_NOCCACHE)" \
	    obj=$(@D) -C $(CONFIG) -f Makefile.br $(@F)

DEFCONFIG = defconfig

COMMON_CONFIG_ENV = \
	KCONFIG_AUTOCONFIG=$(BUILD_DIR)/buildroot-config/auto.conf \
	KCONFIG_AUTOHEADER=$(BUILD_DIR)/buildroot-config/autoconf.h \
	KCONFIG_TRISTATE=$(BUILD_DIR)/buildroot-config/tristate.config \
	BR2_CONFIG=$(BR2_CONFIG) \
	HOST_GCC_VERSION="$(HOSTCC_VERSION)" \
	BUILD_DIR=$(BUILD_DIR) \
	SKIP_LEGACY=

xconfig: $(BUILD_DIR)/buildroot-config/qconf prepare-kconfig
	@$(COMMON_CONFIG_ENV) $< $(CONFIG_CONFIG_IN)

gconfig: $(BUILD_DIR)/buildroot-config/gconf prepare-kconfig
	@$(COMMON_CONFIG_ENV) srctree=$(TOPDIR) $< $(CONFIG_CONFIG_IN)

menuconfig: $(BUILD_DIR)/buildroot-config/mconf prepare-kconfig
	@$(COMMON_CONFIG_ENV) $< $(CONFIG_CONFIG_IN)

nconfig: $(BUILD_DIR)/buildroot-config/nconf prepare-kconfig
	@$(COMMON_CONFIG_ENV) $< $(CONFIG_CONFIG_IN)

config: $(BUILD_DIR)/buildroot-config/conf prepare-kconfig
	@$(COMMON_CONFIG_ENV) $< $(CONFIG_CONFIG_IN)

# For the config targets that automatically select options, we pass
# SKIP_LEGACY=y to disable the legacy options. However, in that case
# no values are set for the legacy options so a subsequent oldconfig
# will query them. Therefore, run an additional olddefconfig.

randconfig allyesconfig alldefconfig allnoconfig: $(BUILD_DIR)/buildroot-config/conf prepare-kconfig
	@$(COMMON_CONFIG_ENV) $< --olddefconfig $(CONFIG_CONFIG_IN) >/dev/null

oldconfig syncconfig olddefconfig: $(BUILD_DIR)/buildroot-config/conf prepare-kconfig
	@$(COMMON_CONFIG_ENV) $< --$@ $(CONFIG_CONFIG_IN)

defconfig: $(BUILD_DIR)/buildroot-config/conf prepare-kconfig
	@$(COMMON_CONFIG_ENV) $< --defconfig$(if $(DEFCONFIG),=$(DEFCONFIG)) $(CONFIG_CONFIG_IN)

define percent_defconfig
%_defconfig: $(BUILD_DIR)/buildroot-config/conf $(1)/configs/%_defconfig prepare-kconfig
	$(TOPDIR)/scripts/expand_install.sh $(1)/configs/$$@ $(O)/.install
	$(TOPDIR)/scripts/defconfig_hook.py -m $(1)/configs/$$@ $(O)/.rockchipconfig
	@$$(COMMON_CONFIG_ENV) BR2_DEFCONFIG=$(1)/configs/$$@ \
		$$< --defconfig=$(O)/.rockchipconfig $$(CONFIG_CONFIG_IN)
endef
$(eval $(call percent_defconfig,$(TOPDIR))$(sep))

update-defconfig: savedefconfig

CFG_ = $(shell grep BR2_DEFCONFIG $(BR2_CONFIG) | cut -d= -f2)
savedefconfig: $(BUILD_DIR)/buildroot-config/conf prepare-kconfig
	grep "#include" $(CFG_) > $(CFG_).split || true

	@$(COMMON_CONFIG_ENV) $< --savedefconfig=$(CFG_) $(CONFIG_CONFIG_IN)
	@$(SED) '/BR2_DEFCONFIG=/d' $(CFG_)

	cat $(CFG_) >> $(CFG_).split
	$(TOPDIR)/scripts/defconfig_hook.py -s $(CFG_).split $(CFG_)
	rm $(CFG_).split

.PHONY: defconfig savedefconfig update-defconfig

################################################################################
#
# Cleanup and misc junk
#
################################################################################

# staging and target directories do NOT list these as
# dependencies anywhere else
$(BUILD_DIR):
	@mkdir -p $@

# printvars prints all the variables currently defined in our
# Makefiles. Alternatively, if a non-empty VARS variable is passed,
# only the variables matching the make pattern passed in VARS are
# displayed.
.PHONY: printvars
printvars:
	@:
	$(foreach V, \
		$(sort $(if $(VARS),$(filter $(VARS),$(.VARIABLES)),$(.VARIABLES))), \
		$(if $(filter-out environment% default automatic, \
				$(origin $V)), \
		$(if $(QUOTED_VARS),\
			$(info $V='$(subst ','\'',$(if $(RAW_VARS),$(value $V),$($V)))'), \
			$(info $V=$(if $(RAW_VARS),$(value $V),$($V))))))
# ' Syntax colouring...

.PHONY: help
help:
	@echo 'Configuration:'
	@echo '  menuconfig             - interactive curses-based configurator'
	@echo '  nconfig                - interactive ncurses-based configurator'
	@echo '  xconfig                - interactive Qt-based configurator'
	@echo '  gconfig                - interactive GTK-based configurator'
	@echo '  oldconfig              - resolve any unresolved symbols in .config'
	@echo '  syncconfig             - Same as oldconfig, but quietly, additionally update deps'
	@echo '  olddefconfig           - Same as syncconfig but sets new symbols to their default value'
	@echo '  randconfig             - New config with random answer to all options'
	@echo '  savedefconfig          - Save current config to BR2_DEFCONFIG (minimal config)'
	@echo '  update-defconfig       - Same as savedefconfig'
	@echo '  allyesconfig           - New config where all options are accepted with yes'
	@echo '  allnoconfig            - New config where all options are answered with no'
	@echo '  alldefconfig           - New config where all options are set to default'
	@echo
	@echo 'Miscellaneous:'
	@echo '  printvars              - dump all the internal variables'
	@echo
	@echo '  make V=0|1             - 0 => quiet build (default), 1 => verbose build'
	@echo '  make O=dir             - Locate all output files in "dir", including .config'
	@echo

.PHONY: $(noconfig_targets)
