# Copyright (C) 2022 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#       http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

load("@bazel_skylib//rules:common_settings.bzl", "string_flag")
load("//build/bazel_common_rules/dist:dist.bzl", "copy_to_dist_dir")
load(
    "//build/kernel/kleaf:kernel.bzl",
    "kernel_build",
    "kernel_images",
    "kernel_module",
    "kernel_modules_install",
    "merged_kernel_uapi_headers",
)
load("@kernel_toolchain_info//:dict.bzl", "BRANCH", "CLANG_VERSION")

def define_slider():
    slider_dtbos = [
        "gs101-oriole.dtbo",
        "gs101-oriole-evt1_1.dtbo",
        "gs101-oriole-evt1.dtbo",
        "gs101-oriole-evt-wingboard.dtbo",
        "gs101-oriole-proto1_1.dtbo",
        "gs101-oriole-proto1.dtbo",
        "gs101-raven.dtbo",
        "gs101-raven-evt1_1.dtbo",
        "gs101-raven-evt1.dtbo",
        "gs101-raven-evt-wingboard.dtbo",
        "gs101-raven-proto1_1.dtbo",
        "gs101-raven-proto1.dtbo",
        "gs101-slider2.dtbo",
        "gs101-slider2-o6r4.dtbo",
        "gs101-slider.dtbo",
        "gs101-whitefin2.dtbo",
        "gs101-whitefin2v2.dtbo",
        "gs101-whitefin.dtbo",
    ]

    native.filegroup(
        name = "slider_dt-bindings",
        srcs = native.glob([
            "include/dt-bindings/**",
            "include/dtc/**",
        ]),
        visibility = [
            "//private/google-modules/soc/gs/arch/arm64/boot/dts:__pkg__",
        ],
    )

    _slider_modules = [
        # keep sorted
        "//private/google-modules/amplifiers/audiometrics:audiometrics.slider",
        "//private/google-modules/amplifiers/cs35l41:cs35l41.slider",
        "//private/google-modules/amplifiers/cs40l25:cs40l25.slider",
        "//private/google-modules/amplifiers/drv2624:drv2624.slider",
        "//private/google-modules/aoc:aoc.slider",
        "//private/google-modules/aoc/alsa:alsa.slider",
        "//private/google-modules/bluetooth/broadcom:broadcom.slider",
        "//private/google-modules/bms:bms.slider",
        "//private/google-modules/display:samsung.slider",
        "//private/google-modules/edgetpu/abrolhos/drivers/edgetpu:edgetpu.slider",
        "//private/google-modules/gpu/mali_kbase:mali_kbase.slider",
        "//private/google-modules/gpu/mali_pixel:mali_pixel.slider",
        "//private/google-modules/lwis:lwis.slider",
        "//private/google-modules/nfc:nfc.slider",
        "//private/google-modules/power/reset:reset.slider",
        "//private/google-modules/soc/gs:gs101_soc",
        "//private/google-modules/touch/common:common.slider",
        "//private/google-modules/touch/fts/ftm5:ftm5.slider",
        "//private/google-modules/touch/sec:sec.slider",
        "//private/google-modules/uwb/kernel:kernel.slider",
        "//private/google-modules/wlan/bcmdhd4389:bcmdhd4389.slider",
    ]

    kernel_build(
        name = "slider",
        srcs = native.glob([
            "build.config.*",
            "arch/arm64/configs/slider_gki.fragment",
            "Kconfig.ext",
            "Kconfig.ext_modules",
            "**/Kconfig",
            "arch/arm64/boot/dts/**",
            "include/dt-bindings/**",
            "include/dtc/**",
        ]) + [
            "//common:kernel_aarch64_sources",
            "//private/google-modules/power/reset:reset.kconfig",
            "//private/google-modules/touch/common:common.kconfig",
            "//private/google-modules/touch/fts/ftm5:ftm5.kconfig",
            "//private/google-modules/touch/sec:sec.kconfig",
        ],
        outs = [
            # Sync with build.config.slider
            "gs101-a0.dtb",
            "gs101-b0.dtb",
        ],
        base_kernel = "//common:kernel_aarch64_download_or_build",
        build_config = "build.config.slider",
        dtstree = "//private/google-modules/soc/gs/arch/arm64/boot/dts:slider_dt",
        implicit_outs = slider_dtbos,
        kconfig_ext = "Kconfig.ext",
        module_outs = [
            # keep sorted
            "arm_dsu_pmu.ko",
            "at24.ko",
            "i2c-dev.ko",
            "pktgen.ko",
            "pps-gpio.ko",
            "sg.ko",
            "softdog.ko",
            "spidev.ko",
            "zsmalloc.ko",
        ],
        # Keep in sync with build.config.common
        toolchain_version = CLANG_VERSION,
    )

    native.filegroup(
        name = "gs101_soc_headers",
        srcs = native.glob([
            # Standard pixel headers
            "include/**/*.h",
            "include/**/uapi/*.h",
        ] + [
            # List of headers needed by external modules
            "drivers/dma-buf/heaps/deferred-free-helper.h",
            "drivers/dma-buf/heaps/samsung/samsung-dma-heap.h",

            # Needed for external modules to include in their makefiles
            "Makefile.include",
        ]),
        visibility = [
            # keep sorted
            "//private/google-modules:__subpackages__",
        ],
    )

    kernel_module(
        name = "gs101_soc",
        srcs = native.glob(
            ["**"],
            exclude = [
                ".*",
                ".*/**",
                "BUILD.bazel",
                "**/*.bzl",
                "build.config.*",
            ],
        ),
        outs = [
            "drivers/bts/exynos-bts.ko",
            "drivers/bts/exynos-btsopsgs101.ko",
            "drivers/char/hw_random/exyswd-rng.ko",
            "drivers/clk/gs/clk_exynos_gs.ko",
            "drivers/clocksource/exynos_mct.ko",
            "drivers/cpufreq/exynos-acme.ko",
            "drivers/devfreq/google/arm-memlat-mon.ko",
            "drivers/devfreq/google/exynos_devfreq.ko",
            "drivers/devfreq/google/governor_memlat.ko",
            "drivers/devfreq/google/memlat-devfreq.ko",
            "drivers/dma-buf/heaps/deferred-free-helper.ko",
            "drivers/dma-buf/heaps/page_pool.ko",
            "drivers/dma-buf/heaps/samsung/samsung_dma_heap.ko",
            "drivers/dma/pl330.ko",
            "drivers/dma/samsung-dma.ko",
            "drivers/gpu/exynos/g2d/g2d.ko",
            "drivers/i2c/busses/i2c-acpm.ko",
            "drivers/i2c/busses/i2c-exynos5.ko",
            "drivers/iio/power/odpm.ko",
            "drivers/input/fingerprint/goodixfp.ko",
            "drivers/input/keycombo.ko",
            "drivers/input/keydebug.ko",
            "drivers/input/misc/vl53l1/stmvl53l1.ko",
            "drivers/iommu/samsung-iommu-group.ko",
            "drivers/iommu/samsung-secure-iova.ko",
            "drivers/iommu/samsung_iommu.ko",
            "drivers/media/platform/bigocean/bigocean.ko",
            "drivers/media/platform/exynos/mfc/exynos_mfc.ko",
            "drivers/media/platform/exynos/smfc/smfc.ko",
            "drivers/mfd/s2mpg10-mfd.ko",
            "drivers/mfd/s2mpg11-mfd.ko",
            "drivers/mfd/s2mpg1x-gpio.ko",
            "drivers/mfd/slg51000-core.ko",
            "drivers/misc/bbdpl/bbd.ko",
            "drivers/misc/bbdpl/bcm47765.ko",
            "drivers/misc/gsc-spi.ko",
            "drivers/misc/gvotable.ko",
            "drivers/misc/logbuffer.ko",
            "drivers/misc/sbb-mux/sbb-mux.ko",
            "drivers/misc/sscoredump/sscoredump.ko",
            "drivers/pci/controller/dwc/pcie-exynos-gs.ko",
            "drivers/pci/controller/dwc/pcie-exynos-gs101-rc-cal.ko",
            "drivers/phy/samsung/phy-exynos-mipi.ko",
            "drivers/phy/samsung/phy-exynos-mipi-dsim.ko",
            "drivers/phy/samsung/phy-exynos-usbdrd-super.ko",
            "drivers/pinctrl/gs/pinctrl-exynos-gs.ko",
            "drivers/pinctrl/pinctrl-slg51000.ko",
            "drivers/power/reset/debug-reboot.ko",
            "drivers/regulator/max77826-gs-regulator.ko",
            "drivers/regulator/pmic_class.ko",
            "drivers/regulator/s2mpg10-powermeter.ko",
            "drivers/regulator/s2mpg10-regulator.ko",
            "drivers/regulator/s2mpg11-powermeter.ko",
            "drivers/regulator/s2mpg11-regulator.ko",
            "drivers/regulator/slg51000-regulator.ko",
            "drivers/rtc/rtc-s2mpg10.ko",
            "drivers/scsi/ufs/ufs-exynos-gs.ko",
            "drivers/soc/google/acpm/acpm_flexpmu_dbg.ko",
            "drivers/soc/google/acpm/acpm_mbox_test.ko",
            "drivers/soc/google/acpm/gs_acpm.ko",
            "drivers/soc/google/acpm/power_stats.ko",
            "drivers/soc/google/bcm_dbg.ko",
            "drivers/soc/google/cal-if/cmupmucal.ko",
            "drivers/soc/google/cpif/boot_device_spi.ko",
            "drivers/soc/google/cpif/cp_thermal_zone.ko",
            "drivers/soc/google/cpif/cpif.ko",
            "drivers/soc/google/cpif/exynos_dit.ko",
            "drivers/soc/google/cpif/shm_ipc.ko",
            "drivers/soc/google/dbgcore-dump.ko",
            "drivers/soc/google/debug/debug-snapshot-debug-kinfo.ko",
            "drivers/soc/google/debug/debug-snapshot-qd.ko",
            "drivers/soc/google/debug/debug-snapshot-sfrdump.ko",
            "drivers/soc/google/debug/dss.ko",
            "drivers/soc/google/debug/ehld.ko",
            "drivers/soc/google/debug/exynos-adv-tracer.ko",
            "drivers/soc/google/debug/exynos-adv-tracer-s2d.ko",
            "drivers/soc/google/debug/exynos-coresight.ko",
            "drivers/soc/google/debug/exynos-coresight-etm.ko",
            "drivers/soc/google/debug/exynos-debug-test.ko",
            "drivers/soc/google/debug/exynos-ecc-handler.ko",
            "drivers/soc/google/debug/gs101-itmon.ko",
            "drivers/soc/google/debug/pixel-boot-metrics.ko",
            "drivers/soc/google/debug/pixel-debug-test.ko",
            "drivers/soc/google/debug/sjtag-driver.ko",
            "drivers/soc/google/ect_parser.ko",
            "drivers/soc/google/eh/eh.ko",
            "drivers/soc/google/exynos-bcm_dbg-dump.ko",
            "drivers/soc/google/exynos-cpuhp.ko",
            "drivers/soc/google/exynos-cpupm.ko",
            "drivers/soc/google/exynos-dm.ko",
            "drivers/soc/google/exynos-pd.ko",
            "drivers/soc/google/exynos-pd-dbg.ko",
            "drivers/soc/google/exynos-pd_el3.ko",
            "drivers/soc/google/exynos-pm.ko",
            "drivers/soc/google/exynos-pmu-if.ko",
            "drivers/soc/google/exynos-seclog.ko",
            "drivers/soc/google/exynos_pm_qos.ko",
            "drivers/soc/google/gs-chipid.ko",
            "drivers/soc/google/gsa/gsa.ko",
            "drivers/soc/google/gsa/gsa_gsc.ko",
            "drivers/soc/google/hardlockup-debug.ko",
            "drivers/soc/google/hardlockup-watchdog.ko",
            "drivers/soc/google/pixel_em/pixel_em.ko",
            "drivers/soc/google/pixel_stat/mm/pixel_stat_mm.ko",
            "drivers/soc/google/pixel_stat/pixel_stat_sysfs.ko",
            "drivers/soc/google/pt/slc_acpm.ko",
            "drivers/soc/google/pt/slc_dummy.ko",
            "drivers/soc/google/pt/slc_pmon.ko",
            "drivers/soc/google/pt/slc_pt.ko",
            "drivers/soc/google/s2mpu/s2mpu.ko",
            "drivers/soc/google/sysrq-hook.ko",
            "drivers/soc/google/vh/kernel/systrace.ko",
            "drivers/spi/spi-s3c64xx.ko",
            "drivers/staging/android/delay_init.ko",
            "drivers/thermal/google/google_bcl.ko",
            "drivers/thermal/google/gs101_spmic_thermal.ko",
            "drivers/thermal/samsung/gpu_cooling.ko",
            "drivers/thermal/samsung/gs101_thermal.ko",
            "drivers/trusty/trusty-core.ko",
            "drivers/trusty/trusty-ipc.ko",
            "drivers/trusty/trusty-irq.ko",
            "drivers/trusty/trusty-log.ko",
            "drivers/trusty/trusty-test.ko",
            "drivers/trusty/trusty-virtio.ko",
            "drivers/tty/serial/exynos_tty.ko",
            "drivers/usb/dwc3/dwc3-exynos-usb.ko",
            "drivers/usb/gadget/function/usb_f_dm.ko",
            "drivers/usb/gadget/function/usb_f_dm1.ko",
            "drivers/usb/host/xhci-exynos.ko",
            "drivers/usb/typec/tcpm/google/bc_max77759.ko",
            "drivers/usb/typec/tcpm/google/max77759_contaminant.ko",
            "drivers/usb/typec/tcpm/google/max77759_helper.ko",
            "drivers/usb/typec/tcpm/google/slg46826.ko",
            "drivers/usb/typec/tcpm/google/tcpci_fusb307.ko",
            "drivers/usb/typec/tcpm/google/tcpci_max77759.ko",
            "drivers/usb/typec/tcpm/google/usb_psy.ko",
            "drivers/usb/typec/tcpm/google/usbc_cooling_dev.ko",
            "drivers/watchdog/s3c2410_wdt.ko",
        ],
        kernel_build = "//private/google-modules/soc/gs:slider",
        visibility = [
            # keep sorted
            "//private/google-modules:__subpackages__",
        ],
    )

    kernel_modules_install(
        name = "slider_modules_install",
        kernel_build = ":slider",
        kernel_modules = _slider_modules,
    )

    merged_kernel_uapi_headers(
        name = "slider_merged_kernel_uapi_headers",
        kernel_build = ":slider",
        kernel_modules = _slider_modules,
    )

    kernel_images(
        name = "slider_images",
        build_boot = True,
        build_dtbo = True,
        build_vendor_boot = True,
        build_vendor_dlkm = True,
        dtbo_srcs = [":slider/" + e for e in slider_dtbos],
        kernel_build = ":slider",
        kernel_modules_install = ":slider_modules_install",
        # Keep the following in sync with build.config.slider:
        modules_list = "vendor_boot_modules.slider",
        # No MODULES_BLOCKLIST
        # Keep the following in sync with build.config.gs101:
        vendor_dlkm_modules_list = "vendor_dlkm_modules.slider",
        vendor_dlkm_modules_blocklist = "vendor_dlkm.blocklist.slider",
        vendor_dlkm_props = "vendor_dlkm.props.slider",
        # VENDOR_RAMDISK_BINARY
        vendor_ramdisk_binaries = ["//prebuilts/boot-artifacts/ramdisks:vendor_ramdisk-oriole.img"],
        deps = [
            # Keep the following in sync with vendor_dlkm.props.slider:
            # selinux_fc
            "//prebuilts/boot-artifacts/selinux:file_contexts",
        ],
    )

    slider_dist_targets = [
        ":slider",
        ":slider_images",
        # At the time of writing (2022-02-04), this intentionally diverges from
        # the behavior of build.sh-style mixed builds by also incorporating
        # UAPI headers of external modules, while build.sh-style mixed builds
        # always uses kernel-uapi-headers.tar.gz from GKI_DIST_DIR.
        # To use GKI's kernel-uapi-headers.tar.gz in DIST_DIR, use
        #     //common:kernel_aarch64_uapi_headers
        # instead.
        ":slider_merged_kernel_uapi_headers",
        ":slider_modules_install",
        "//common:kernel_aarch64_download_or_build",
        "//common:kernel_aarch64_additional_artifacts_download_or_build",
    ]

    copy_to_dist_dir(
        name = "slider_dist",
        data = slider_dist_targets,
        dist_dir = "out/{branch}/dist".format(branch = BRANCH),
        flat = True,
    )