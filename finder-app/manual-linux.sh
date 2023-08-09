#!/bin/bash
# Script outline to install and build kernel.
# Author: Siddhant Jajoo.

set -e
set -u

OUTDIR=/tmp/aeld
KERNEL_REPO=git://git.kernel.org/pub/scm/linux/kernel/git/stable/linux-stable.git
KERNEL_VERSION=v5.1.10
BUSYBOX_VERSION=1_33_1
FINDER_APP_DIR=$(realpath $(dirname $0))
ARCH=arm64
CROSS_COMPILE=aarch64-none-linux-gnu-

if [ $# -lt 1 ]
then
	echo "Using default directory ${OUTDIR} for output"
else
	OUTDIR=$1
	echo "Using passed directory ${OUTDIR} for output"
fi

mkdir -p ${OUTDIR}

cd "$OUTDIR"
if [ ! -d "${OUTDIR}/linux-stable" ]; then
    #Clone only if the repository does not exist.
	echo "CLONING GIT LINUX STABLE VERSION ${KERNEL_VERSION} IN ${OUTDIR}"
	git clone ${KERNEL_REPO} --depth 1 --single-branch --branch ${KERNEL_VERSION}
fi
if [ ! -e ${OUTDIR}/linux-stable/arch/${ARCH}/boot/Image ]; then
    cd linux-stable
    echo "Checking out version ${KERNEL_VERSION}"
    git checkout ${KERNEL_VERSION}

    # TODO: Add your kernel build steps here
    make ARCH=${ARCH} defconfig
    make -j 4 ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} all
fi

echo "Adding the Image in outdir"
cp ${OUTDIR}/linux-stable/arch/${ARCH}/boot/Image ${OUTDIR}

echo "Creating the staging directory for the root filesystem"
cd "$OUTDIR"
if [ -d "${OUTDIR}/rootfs" ]
then
	echo "Deleting rootfs directory at ${OUTDIR}/rootfs and starting over"
    sudo rm  -rf ${OUTDIR}/rootfs
fi

# TODO: Create necessary base directories
mkdir rootfs
cd rootfs
mkdir -p bin dev etc home lib lib64 proc sbin sys tmp usr var
mkdir -p usr/bin usr/lib usr/sbin
mkdir -p var/log

cd "$OUTDIR"
if [ ! -d "${OUTDIR}/busybox" ]
then
git clone git://busybox.net/busybox.git
    cd busybox
    git checkout ${BUSYBOX_VERSION}
    # TODO:  Configure busybox
    make distclean
    make ARCH=${ARCH} defconfig
else
    cd busybox
fi

# TODO: Make and install busybox
make -j 4 ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE}
make -j 4 CONFIG_PREFIX=${OUTDIR}/rootfs ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} install
mkdir -p bin
cp busybox bin

echo "Library dependencies"
${CROSS_COMPILE}readelf -a bin/busybox | grep "program interpreter"
${CROSS_COMPILE}readelf -a bin/busybox | grep "Shared library"

# TODO: Add library dependencies to rootfs
TOOLCHAIN_SYSROOT=`${CROSS_COMPILE}gcc -print-sysroot`
echo ${TOOLCHAIN_SYSROOT}
cp ${TOOLCHAIN_SYSROOT}/lib/ld-linux-aarch64.so.1 ${OUTDIR}/rootfs/lib
cp ${TOOLCHAIN_SYSROOT}/lib64/libm.so.6  ${OUTDIR}/rootfs/lib64
cp ${TOOLCHAIN_SYSROOT}/lib64/libresolv.so.2  ${OUTDIR}/rootfs/lib64
cp ${TOOLCHAIN_SYSROOT}/lib64/libc.so.6  ${OUTDIR}/rootfs/lib64


# TODO: Make device nodes
cd "$OUTDIR"
sudo mknod -m 666 rootfs/dev/null c 1 3
sudo mknod -m 666 rootfs/dev/console c 5 1

# TODO: Clean and build the writer utility
cd "$FINDER_APP_DIR"
make clean
make CROSS_COMPILE=aarch64-none-linux-gnu-
cp writer "$OUTDIR/rootfs/home"

# TODO: Copy the finder related scripts and executables to the /home directory
# on the target rootfs
cp finder.sh "$OUTDIR/rootfs/home"
cp finder-test.sh "$OUTDIR/rootfs/home"
mkdir "$OUTDIR/rootfs/conf"
cp -r conf/*.txt "$OUTDIR/rootfs/conf"
cp -r "$OUTDIR/rootfs/conf" "$OUTDIR/rootfs/home/conf" 
cp autorun-qemu.sh "$OUTDIR/rootfs/home"

# TODO: Chown the root directory
cd "$OUTDIR/rootfs"
find . | cpio -H newc -ov --owner root:root > ${OUTDIR}/initramfs.cpio

# TODO: Create initramfs.cpio.gz
cd "$OUTDIR"
gzip -f initramfs.cpio
