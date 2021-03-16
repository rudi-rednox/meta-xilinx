DESCRIPTION = "PMU Firmware"

inherit xlnx-embeddedsw deploy

COMPATIBLE_HOST = "microblaze.*-elf"
COMPATIBLE_MACHINE = "none"
COMPATIBLE_MACHINE_zynqmp = ".*"

S = "${WORKDIR}/git"
B = "${S}/lib/sw_apps/zynqmp_pmufw/src"

# The makefile does not handle parallelization
PARALLEL_MAKE = "-j1"

do_configure() {
    # manually do the copy_bsp step first, so as to be able to fix up use of
    # mb-* commands
    ${B}/../misc/copy_bsp.sh
}

# All do_compiles need this, even if it's overriden
MB_OBJCOPY ??= "${OBJCOPY}"

# Only add a dependency if we need to use OUR binutils
DEPENDS .= "${@' virtual/${TARGET_PREFIX}binutils' if d.getVar('MB_OBJCOPY') == d.getVar('OBJCOPY') else ''}"

COMPILER = "${CC}"
COMPILER_FLAGS = "-O2 -c"
EXTRA_COMPILER_FLAGS = "-g -Wall -Wextra -Os -flto -ffat-lto-objects"
ARCHIVER = "${AR}"

BSP_DIR ?= "${B}/../misc/zynqmp_pmufw_bsp"
BSP_TARGETS_DIR ?= "${BSP_DIR}/psu_pmu_0/libsrc"

def bsp_make_vars(d):
    s = ["COMPILER", "CC", "COMPILER_FLAGS", "EXTRA_COMPILER_FLAGS", "ARCHIVER", "AR", "AS"]
    return " ".join(["\"%s=%s\"" % (v, d.getVar(v)) for v in s])

EXTRA_OEMAKE = "${@bsp_make_vars(d)}"

do_compile() {
    # the Makefile in ${B}/../misc/Makefile, does not handle CC, AR, AS, etc
    # properly. So do its job manually. Preparing the includes first, then libs.
    for i in $(ls ${BSP_TARGETS_DIR}/*/src/Makefile); do
        oe_runmake -C $(dirname $i) -s include
    done
    for i in $(ls ${BSP_TARGETS_DIR}/*/src/Makefile); do
        oe_runmake -C $(dirname $i) -s libs
    done

    # --build-id=none is required due to linker script not defining a location for it.
    # Again, recipe-systoot include is necessary
    oe_runmake CC_FLAGS="-MMD -MP -Wl,--build-id=none -I${STAGING_DIR_TARGET}/usr/include"

    ${MB_OBJCOPY} -O binary ${B}/${ESW_COMPONENT} ${B}/executable.bin
}

do_install() {
    :
}

PACKAGES = ""

# This is the default in most BSPs.  A MACHINE.conf can override this!
PMU_FIRMWARE_IMAGE_NAME ??= "pmu-firmware-zynqmp-pmu"

inherit image-artifact-names

PMU_FIRMWARE_BASE_NAME ?= "${PMU_FIRMWARE_IMAGE_NAME}-${PKGE}-${PKGV}-${PKGR}-${MACHINE}${IMAGE_VERSION_SUFFIX}"

ESW_COMPONENT ??= "executable.elf"

do_deploy() {
    install -Dm 0644 ${B}/${ESW_COMPONENT} ${DEPLOYDIR}/${PMU_FIRMWARE_BASE_NAME}.elf
    ln -sf ${PMU_FIRMWARE_BASE_NAME}.elf ${DEPLOYDIR}/${PMU_FIRMWARE_IMAGE_NAME}.elf
    install -m 0644 ${B}/executable.bin ${DEPLOYDIR}/${PMU_FIRMWARE_BASE_NAME}.bin
    ln -sf ${PMU_FIRMWARE_BASE_NAME}.bin ${DEPLOYDIR}/${PMU_FIRMWARE_IMAGE_NAME}.bin
}

addtask deploy before do_build after do_install