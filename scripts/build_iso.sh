#!/bin/bash
# CertOS ISO Builder

set -e

ISO_NAME="certos-v1.0-amd64.iso"
ROOTFS_IMG="certos_rootfs.img"
ROOTFS_DIR="./mnt_rootfs"
BUILD_DIR="./certos_build"

echo "### Starting CertOS ISO Build (Loop Device Mode) ###"

# 1. Install dependencies
sudo apt-get update && sudo apt-get install -y debootstrap xorriso isolinux squashfs-tools mtools

# 2. Create and Mount Image File (Workaround for NTFS/DrvFS)
if [ ! -f "$ROOTFS_IMG" ]; then
    echo "Creating 2GB image file for rootfs..."
    dd if=/dev/zero of="$ROOTFS_IMG" bs=1M count=2048
    mkfs.ext4 "$ROOTFS_IMG"
fi

mkdir -p "$ROOTFS_DIR"
if ! mountpoint -q "$ROOTFS_DIR"; then
    echo "Mounting image file..."
    sudo mount -o loop "$ROOTFS_IMG" "$ROOTFS_DIR"
fi

# 3. Create Rootfs
if [ ! -f "$ROOTFS_DIR/etc/debian_version" ]; then
    echo "Creating base rootfs (Debian)..."
    sudo debootstrap --arch amd64 bookworm "$ROOTFS_DIR" http://deb.debian.org/debian/
    
    echo "Installing Kernel and base utilities..."
    sudo chroot "$ROOTFS_DIR" apt-get update
    sudo chroot "$ROOTFS_DIR" apt-get install -y --no-install-recommends linux-image-amd64 systemd-sysv
fi

# 3. Ensure Kernel and Live components are installed
if [ ! -f "$ROOTFS_DIR/vmlinuz" ] && [ ! -f "$ROOTFS_DIR/boot/vmlinuz"* ]; then
    echo "Kernel not found in rootfs. Installing linux-image-amd64 and live components..."
    sudo chroot "$ROOTFS_DIR" apt-get update
    sudo chroot "$ROOTFS_DIR" apt-get install -y --no-install-recommends linux-image-amd64 systemd-sysv live-boot live-config
fi

# 3. Install CertOS components into rootfs
echo "Installing CertOS binaries..."
sudo mkdir -p "$ROOTFS_DIR/usr/local/bin"
sudo mkdir -p "$ROOTFS_DIR/opt/certos/images"
sudo cp bin/certosc-* "$ROOTFS_DIR/usr/local/bin/"
sudo cp scripts/certos-update.sh "$ROOTFS_DIR/usr/local/bin/"
sudo cp os/installer/wizard.sh "$ROOTFS_DIR/usr/local/bin/"
sudo cp os/bin/certos-monitor.sh "$ROOTFS_DIR/usr/local/bin/"

echo "Installing systemd units..."
sudo mkdir -p "$ROOTFS_DIR/etc/systemd/system"
sudo cp os/systemd/certos-update.* "$ROOTFS_DIR/etc/systemd/system/"
# We will enable them via a chroot later if needed, or by symlinking
sudo ln -sf /etc/systemd/system/certos-update.timer "$ROOTFS_DIR/etc/systemd/system/timers.target.wants/certos-update.timer"
echo "Configuring boot wizard..."
sudo mkdir -p "$ROOTFS_DIR/etc/systemd/system/getty@tty1.service.d"
cat <<EOF | sudo tee "$ROOTFS_DIR/etc/systemd/system/getty@tty1.service.d/override.conf"
[Service]
ExecStart=
ExecStart=-/bin/bash -c "if [ -f /etc/certos/role ]; then /usr/local/bin/certos-monitor.sh; else /usr/local/bin/wizard.sh; fi"
StandardInput=tty
StandardOutput=tty
EOF

# 5. Pack into ISO
echo "Packing into ISO..."
mkdir -p "$BUILD_DIR/live"
mkdir -p "$BUILD_DIR/isolinux"

echo "Creating SquashFS image (this may take a while)..."
sudo mksquashfs "$ROOTFS_DIR" "$BUILD_DIR/live/filesystem.squashfs" -noappend -e boot

echo "Copying boot files..."
# Copy kernel and initrd to ISO layout
sudo cp "$ROOTFS_DIR/boot/vmlinuz-6.1.0-42-amd64" "$BUILD_DIR/live/vmlinuz"
sudo cp "$ROOTFS_DIR/boot/initrd.img-6.1.0-42-amd64" "$BUILD_DIR/live/initrd.img"

# Copy isolinux components from host
cp /usr/lib/ISOLINUX/isolinux.bin "$BUILD_DIR/isolinux/"
cp /usr/lib/syslinux/modules/bios/ldlinux.c32 "$BUILD_DIR/isolinux/"
cp /usr/lib/syslinux/modules/bios/libcom32.c32 "$BUILD_DIR/isolinux/"
cp /usr/lib/syslinux/modules/bios/libutil.c32 "$BUILD_DIR/isolinux/"
cp /usr/lib/syslinux/modules/bios/vesamenu.c32 "$BUILD_DIR/isolinux/"

# Create isolinux configuration
cat <<EOF > "$BUILD_DIR/isolinux/isolinux.cfg"
UI vesamenu.c32
PROMPT 0
TIMEOUT 50
DEFAULT certos

LABEL certos
    MENU LABEL CertOS HPC Cloud (Live)
    KERNEL /live/vmlinuz
    APPEND initrd=/live/initrd.img boot=live quiet
EOF

echo "Generating final ISO file: $ISO_NAME"
xorriso -as mkisofs -o "$ISO_NAME" \
    -b isolinux/isolinux.bin -c isolinux/boot.cat \
    -no-emul-boot -boot-load-size 4 -boot-info-table \
    -eltorito-alt-boot -e live/initrd.img -no-emul-boot \
    -isohybrid-mbr /usr/lib/ISOLINUX/isohdpfx.bin \
    -J -R -V "CERTOS_LIVE" "$BUILD_DIR"

echo "### ISO Build complete! File: $ISO_NAME ###"
echo "Rootfs remains at: $ROOTFS_DIR (unmounted)"

# Clean up
echo "Unmounting rootfs..."
sudo umount "$ROOTFS_DIR"
