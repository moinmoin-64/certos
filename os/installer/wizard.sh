#!/bin/bash
# CertOS Wizard Installer

# Ensure dialog is installed
if ! command -v dialog &> /dev/null; then
    echo "Installer requires 'dialog' package. Installing..."
    sudo apt-get update && sudo apt-get install -y dialog
fi

welcome_msg="Welcome to the CertOS HPC Platform Installer.\n\nThis wizard will guide you through the process of installing CertOS on your hardware."

dialog --title "CertOS Installer" --msgbox "$welcome_msg" 15 60

# 1. Select Disk
DISKS=$(lsblk -d -n -o NAME,SIZE | awk '{print $1 " " $2}')
SELECTED_DISK=$(dialog --title "Select Disk" --menu "Choose the disk to install CertOS:" 15 60 5 $DISKS 3>&1 1>&2 2>&3)

if [ -z "$SELECTED_DISK" ]; then
    dialog --msgbox "Installation cancelled." 10 40
    exit 0
fi

# 2. Confirm Partitioning
dialog --title "WARNING" --yesno "This will WIP ALL DATA on /dev/$SELECTED_DISK. Are you sure?" 10 60
if [ $? -ne 0 ]; then
    exit 0
fi

# 3. Partitioning Logic (Simplified)
echo "Partitioning /dev/$SELECTED_DISK..."
# Here we would use fdisk/parted and mkfs.ext4
# For simulation:
# 4. Set Node Role
ROLE=$(dialog --title "Node Role" --menu "Select the role for this node:" 15 60 3 \
    "MASTER" "Orchestrator and Gateway" \
    "AGENT" "Compute Node" 3>&1 1>&2 2>&3)

# 5. Save Configuration
mkdir -p /etc/certos
echo "$ROLE" > /etc/certos/role

# 6. Finalize
dialog --title "Success" --msgbox "CertOS has been installed successfully as a $ROLE node.\n\nPlease reboot and remove the installation media." 12 60

clear
echo "Installation complete. Please reboot."
