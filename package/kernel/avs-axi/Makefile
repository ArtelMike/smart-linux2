#
# Copyright (C) 2008 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/kernel.mk

PKG_NAME:=avs-axi
PKG_RELEASE:=1

include $(INCLUDE_DIR)/package.mk

define KernelPackage/avs-axi
  SUBMENU:=Artel SMART Support
  TITLE:=Artel PCIe FPGA Drivers
  DEPENDS:=
  FILES:=$(PKG_BUILD_DIR)/avs_ioctl_xilinx_axi.ko $(PKG_BUILD_DIR)/avs_pci_xilinx_axi.ko
  KCONFIG:=
endef

define KernelPackage/avs-axi/description
 Kernel module PCIe to Xilinx FPGA device.
endef

MAKE_OPTS:= \
	$(KERNEL_MAKE_FLAGS) \
	M="$(PKG_BUILD_DIR)"

define Build/Compile
	$(MAKE) -C "$(LINUX_DIR)" \
		$(MAKE_OPTS) \
		modules
endef


$(eval $(call KernelPackage,avs-axi))
