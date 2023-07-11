#
# Copyright (C) 2008 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/kernel.mk

PKG_NAME:=avs-loadfpga
PKG_RELEASE:=1

include $(INCLUDE_DIR)/package.mk

define KernelPackage/avs-loadfpga
  SUBMENU:=Artel SMART Support
  TITLE:=Artel Program FPGA Driver
  DEPENDS:=@GPIO_SUPPORT
  FILES:=$(PKG_BUILD_DIR)/avs_loadfpga.ko $(PKG_BUILD_DIR)/avs_io.ko
  KCONFIG:=
endef

define KernelPackage/avs-loadfpga/description
 Kernel module for loading the Xilinx FPGA device.
endef

MAKE_OPTS:= \
	$(KERNEL_MAKE_FLAGS) \
	M="$(PKG_BUILD_DIR)"

define Build/Compile
	$(MAKE) -C "$(LINUX_DIR)" \
		$(MAKE_OPTS) \
		modules
endef


$(eval $(call KernelPackage,avs-loadfpga))
