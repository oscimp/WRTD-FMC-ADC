cd /sys/bus/zio/devices
if [ $? -ne 0 ]; then
	echo Could not access ZIO sysfs.
	echo Make sure you have sudo permissions and that the bitstream was sent to the FPGA.
	exit 1
fi

ZIO_ID="$(ls | grep ^adc)"
ZIO_ID="$(echo "$ZIO_ID" | sed 's:adc-100m14b-::')"
if [ -z "$ZIO_ID" ]; then
	echo Could not find the FMC ADC device in ZIO sysfs.
	exit 1
elif [ ${#ZIO_ID} -gt 4 ]; then
	echo Found several FMC ADC devices:
	echo $ZIO_ID
	exit 1
fi
echo $ZIO_ID

