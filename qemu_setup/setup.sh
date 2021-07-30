# Network configuration
if [ -n $HOST_INTERFACE ]; then
	echo No host interface name sepcified, defaulting to eth0
	HOST_INTERFACE=eth0
fi

# Setting up a virtual TAP interface linked to the host real interface
tunctl -u `whoami` -t tap0
ifconfig tap0 192.168.0.1
# Authorizing packet forwarding
echo 1 > /proc/sys/net/ipv4/ip_forward
# Setting up the routing table
iptables -t nat -A POSTROUTING -o $HOST_INTERFACE -j MASQUERADE
iptables -I FORWARD 1 -i tap0 -j ACCEPT
iptables -I FORWARD 1 -o tap0 -m state --state RELATED,ESTABLISHED -j ACCEPT


# Passing the SPEC board to QEMU
# Get the PCI ID of the board
PCI_ID=$(lspci | grep CERN | awk {'print $1'})
if [ -n PCI_ID ]; then
	echo Could not find the SPEC board.
	exit 1
fi
PCI_ID="0000:"$PCI_ID
echo PCI ID: $PCI_ID

# Get the vendor and device IDs of the board
VENDOR_DEVICE=$(lspci -ns $PCI_ID | awk {'print $3'} | sed 's/:/ /g')

# Unbind the driver in the host system
if [ -e /sys/bus/pci/devices/$PCI_ID/driver ]; then
	echo $toto > /sys/bus/pci/devices/$PCI_ID/driver/unbind
fi

# Enabling VFIO modules
modprobe vfio
modprobe vfio-pci
# Passing the SPEC board
echo "$VENDOR_DEVICE" > /sys/bus/pci/drivers/vfio-pci/new_id

