# Get the PCI ID of the board
PCI_ID=$(lspci | grep CERN | awk {'print $1'})
if [ -z PCI_ID ]; then
	echo Could not find the SPEC board.
	exit 1
fi
PCI_ID=0000:$PCI_ID

echo -e -n "wrtd_ref_spec150t_adc.bin\0" > /sys/kernel/debug/$PCI_ID/fpga_firmware
