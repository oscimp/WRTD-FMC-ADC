# Checking disk argument
if [ $1 ]; then
	HDA=$1
	if [ ! -e $HDA ]; then
		echo Invalid disk file.
		exit 1
	fi
else
	echo No disk file specified.
	echo Usage: $0 disk.img
	exit 1
fi

# Getting the SPEC PCI ID
PCI_ID=$(lspci | grep CERN | awk {'print $1'})
if [ -z PCI_ID ]; then
	echo Could not find the SPEC board.
	exit 1
fi

# Checking shared directory
if [ -z $SHARED_DIR ]; then
	SHARED_DIR=/mnt/shared
	echo No host shared directory specified, defaulting to $SHARED_DIR
fi
if [ ! -e $SHARED_DIR ]; then
	echo Invalid shared directory.
	exit 1
fi

qemu-system-x86_64 \
	-enable-kvm -cpu host \
	-machine type=q35,accel=kvm \
	-m 2G \
	-smp 2 \
    	-hda $HDA \
	-device virtio-net,netdev=network0 -netdev tap,id=network0,ifname=tap0,script=no,downscript=no,vhost=on \
	-device intel-iommu,caching-mode=on \
    	-device vfio-pci,host=$PCI_ID \
	-virtfs local,path=$SHARED_DIR,mount_tag=host0,security_model=passthrough,id=host0
