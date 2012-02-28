# Inherit AOSP device configuration for passion.
$(call inherit-product, $(SRC_TARGET_DIR)/product/languages_full.mk)
$(call inherit-product, device/common/gps/gps_eu_supl.mk)

#PRODUCT_SPECIFIC_DEFINES += TARGET_PRELINKER_MAP=$(TOP)/vendor/samsung/apollo/prelink-linux-arm-apollo.map

# density in DPI of the LCD of this board. This is used to scale the UI
# appropriately. If this property is not defined, the default value is 160 dpi. 
PRODUCT_PROPERTY_OVERRIDES += \
    ro.sf.lcd_density=120

PRODUCT_COPY_FILES += \
        device/common/gps/gps.conf_EU_SUPL:system/etc/gps.conf

# Used by BusyBox
KERNEL_MODULES_DIR:=/lib/modules

# Enable Windows Media if supported by the board
WITH_WINDOWS_MEDIA:=true

# Add device package overlay
  DEVICE_PACKAGE_OVERLAYS += device/samsung/apollo/overlay

# Add LDPI assets, in addition to MDPI
  PRODUCT_LOCALES += ldpi mdpi

# Extra overlay for LDPI
  PRODUCT_PACKAGE_OVERLAYS += vendor/cyanogen/overlay/ldpi

# Broadcom FM radio
#  $(call inherit-product, vendor/cyanogen/products/bcm_fm_radio.mk)

DISABLE_DEXPREOPT := false

# Kernel debug file
PRODUCT_COPY_FILES += \
	device/samsung/apollo/klogtail:system/xbin/klogtail

# Install the features available on this device.
PRODUCT_COPY_FILES += \
    frameworks/base/data/etc/handheld_core_hardware.xml:system/etc/permissions/handheld_core_hardware.xml \
    frameworks/base/data/etc/android.hardware.audio.low_latency.xml:system/etc/permissions/android.hardware.audio.low_latency.xml \
    frameworks/base/data/etc/android.hardware.location.gps.xml:system/etc/permissions/android.hardware.location.gps.xml \
    frameworks/base/data/etc/android.hardware.sensor.accelerometer.xml:system/etc/permissions/android.hardware.sensor.accelerometer.xml \
    frameworks/base/data/etc/android.hardware.sensor.compass.xml:system/etc/permissions/android.hardware.sensor.compass.xml \
    frameworks/base/data/etc/android.hardware.sensor.gyroscope.xml:system/etc/permissions/android.hardware.sensor.gyroscope.xml \
    frameworks/base/data/etc/android.hardware.sensor.proximity.xml:system/etc/permissions/android.hardware.sensor.proximity.xml \
    frameworks/base/data/etc/android.hardware.sensor.light.xml:system/etc/permissions/android.hardware.sensor.light.xml \
    frameworks/base/data/etc/android.hardware.camera.autofocus.xml:system/etc/permissions/android.hardware.camera.autofocus.xml \
    frameworks/base/data/etc/android.hardware.telephony.gsm.xml:system/etc/permissions/android.hardware.telephony.gsm.xml \
    frameworks/base/data/etc/android.hardware.wifi.xml:system/etc/permissions/android.hardware.wifi.xml \
    frameworks/base/data/etc/android.hardware.touchscreen.multitouch.jazzhand.xml:system/etc/permissions/android.hardware.touchscreen.multitouch.jazzhand.xml \
    frameworks/base/data/etc/android.software.sip.voip.xml:system/etc/permissions/android.software.sip.voip.xml \
    packages/wallpapers/LivePicker/android.software.live_wallpaper.xml:system/etc/permissions/android.software.live_wallpaper.xml

# apns config file
PRODUCT_COPY_FILES += \
        vendor/cyanogen/prebuilt/common/etc/apns-conf.xml:system/etc/apns-conf.xml

# HAL libs and other system binaries
PRODUCT_PACKAGES += \
    copybit.GT-I5800 \
    gralloc.GT-I5800 \
    gps.GT-I5800 \
    sensors.GT-I5800 \
    lights.GT-I5800 \
    libGLES_fimg \
    brcm_patchram_plus \
    screencap \
    dexpreopt \
    libstagefrighthw 

# Samsung Specific tools
PRODUCT_PACKAGES += \
    SamsungServiceMode \
    G3Parts

PRODUCT_PACKAGES += \
    CMWallpapers \
    Pacman \
    Stk \
    Superuser

#Hardware OMX Codecs
PRODUCT_PACKAGES += \
    libSEC_OMX_Core.s5p6442 \
    libOMX.SEC.AVC.Decoder.s5p6442 \
    libOMX.SEC.M4V.Decoder.s5p6442

PRODUCT_COPY_FILES += \
	device/samsung/apollo/sec_mm/sec_omx/sec_omx_core/secomxregistry:system/etc/secomxregistry

# Theme packages
PRODUCT_PACKAGES += \
    Androidian \
    Cyanbread 

# Bluetooth MAC Address
PRODUCT_PACKAGES += \
    bdaddr_read

PRODUCT_COPY_FILES += \
    device/samsung/apollo/media_profiles.xml:system/etc/media_profiles.xml \
    device/samsung/apollo/asound.conf:system/etc/asound.conf \
    device/samsung/apollo/dhcpcd.conf:system/etc/dhcpcd.conf \
    device/samsung/apollo/vold.fstab:system/etc/vold.fstab \
    device/samsung/apollo/dbus.conf:system/etc/dbus.conf \
    device/samsung/apollo/recovery.fstab:recovery/root/etc/recovery.fstab \
    device/samsung/apollo/Credits-CM.html:system/etc/Credits-CM.html


# Live wallpaper packages
PRODUCT_PACKAGES += \
    LiveWallpapers \
    LiveWallpapersPicker \
    MagicSmokeWallpapers \
    VisualizationWallpapers \
    librs_jni

# The OpenGL ES API level that is natively supported by this device.
# This is a 16.16 fixed point number
PRODUCT_PROPERTY_OVERRIDES := \
    ro.opengles.version=131072

# Keyboard maps
PRODUCT_COPY_FILES += \
	device/samsung/apollo/prebuilt/keychars/qwerty.kcm.bin:system/usr/keychars/qwerty.kcm.bin \
	device/samsung/apollo/prebuilt/keychars/qwerty2.kcm.bin:system/usr/keychars/qwerty2.kcm.bin \
	device/samsung/apollo/prebuilt/keychars/qt602240_ts_input.kcm.bin:system/usr/keychars/qt602240_ts_input.kcm.bin \
	device/samsung/apollo/prebuilt/keychars/s3c-keypad.kcm.bin:system/usr/keychars/s3c-keypad.kcm.bin \
	device/samsung/apollo/prebuilt/keychars/sec_jack.kcm.bin:system/usr/keychars/sec_jack.kcm.bin \
	device/samsung/apollo/prebuilt/keylayout/qt602240_ts_input.kl:system/usr/keylayout/qt602240_ts_input.kl \
	device/samsung/apollo/prebuilt/keylayout/s3c-keypad.kl:system/usr/keylayout/s3c-keypad.kl \
	device/samsung/apollo/prebuilt/keylayout/sec_headset.kl:system/usr/keylayout/sec_headset.kl 

# wifi
PRODUCT_COPY_FILES += \
	device/samsung/apollo/wifi/bcm4329_aps.bin:system/etc/wifi/bcm4329_aps.bin \
	device/samsung/apollo/wifi/bcm4329_mfg.bin:system/etc/wifi/bcm4329_mfg.bin \
	device/samsung/apollo/wifi/bcm4329_sta.bin:system/etc/wifi/bcm4329_sta.bin \
	device/samsung/apollo/wifi/nvram_mfg.txt:system/etc/wifi/nvram_mfg.txt \
	device/samsung/apollo/wifi/nvram_net.txt:system/etc/wifi/nvram_net.txt \
	device/samsung/apollo/wifi/wifi.conf:system/etc/wifi/wifi.conf \
	device/samsung/apollo/wifi/wpa_supplicant.conf:system/etc/wifi/wpa_supplicant.conf 

PRODUCT_PROPERTY_OVERRIDES += \
       wifi.interface=eth0 \
       wifi.supplicant_scan_interval=20 \
       ro.telephony.ril_class=samsung \
       ro.telephony.sends_barcount=1 \
       mobiledata.interfaces=pdp0,eth0,gprs,ppp0 \
       dalvik.vm.dexopt-flags=m=y \
       dalvik.vm.execution-mode=int:jit \
       dalvik.vm.heapsize=32m \
       persist.sys.purgeable_assets=0 \
       ro.compcache.default=0 \
       persist.sys.use_dithering=0 \
       persist.sys.use_16bpp_alpha=1 \
       persist.sys.led_trigger=none
#       persist.service.usb.setting=0 \
#       dev.sfbootcomplete=0 \
#       persist.sys.vold.switchexternal=1


# enable Google-specific location features,
# like NetworkLocationProvider and LocationCollector
PRODUCT_PROPERTY_OVERRIDES += \
        ro.com.google.locationfeatures=1 \
        ro.com.google.networklocation=1


# we have enough storage space to hold precise GC data
PRODUCT_TAGS += dalvik.gc.type-precise

# Extended JNI checks
# The extended JNI checks will cause the system to run more slowly, but they can spot a variety of nasty bugs 
# before they have a chance to cause problems.
# Default=true for development builds, set by android buildsystem.
PRODUCT_PROPERTY_OVERRIDES += \
    ro.kernel.android.checkjni=0 \
    dalvik.vm.checkjni=false

# Discard inherited values and use our own instead.
PRODUCT_NAME := full_apollo
PRODUCT_DEVICE := apollo
PRODUCT_MODEL := GT-I5800
PRODUCT_BRAND := samsung
PRODUCT_MANUFACTURER := Samsung

# See comment at the top of this file. This is where the other
# half of the device-specific product definition file takes care
# of the aspects that require proprietary drivers that aren't
# commonly available
$(call inherit-product-if-exists, vendor/samsung/apollo/apollo-vendor.mk)

