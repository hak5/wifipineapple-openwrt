#
# Copyright (C) 2017 Hak5.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

define Profile/HAK5_PINEAPPLE_NANO
	NAME:=Hak5 WiFi Pineapple NANO
	PACKAGES:=kmod-usb-core kmod-usb2 kmod-ledtrig-usbdev kmod-ath9k-htc
endef

define Profile/HAK5_PINEAPPLE_NANO/Description
	Package set optimized for the WiFi Pineapple NANO.
endef

$(eval $(call Profile,HAK5_PINEAPPLE_NANO))
