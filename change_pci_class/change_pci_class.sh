# Search for PCI device ID
DEV_ID="$(lspci | grep CERN | awk '{print substr($1, 1, 5)}')"
if [ -z "$DEV_ID" ]; then
	echo No PCI device detected matching SPEC.
	exit 1
elif [ ${#DEV_ID} -gt 5 ]; then
	echo Found several boards from CERN, exiting to avoid ambiguity.
	exit 1
fi


# Search for ht-flasher executable
if [ -z "$HT_FLASHER" ]; then
	HT_FLASHER=$(pwd)/ht-flasher/src/ht-flasher
	echo No ht-flasher path specified, searching at $HT_FLASHER.
fi
if [ -e $HT_FLASHER ]; then
	echo Successfully found ht-flasher executable.
else
	echo Failed to find ht-flasher executable.
	exit 1
fi


echo Reading the EEPROM.
$HT_FLASHER -a spec-eeprom -d $DEV_ID -r eeprom.img
if [ $? -ne 0 ]; then
	echo Failed to read the EEPROM.
	exit 1
else
	echo Successfully read the EEPROM.
fi


echo Modifying the class code
# Write 0xFF at 0x17 = 23
printf '\xff' | dd of=eeprom.img bs=1 seek=23 count=1 conv=notrunc


echo Writing back to the EEPROM
$HT_FLASHER -a spec-eeprom -d $DEV_ID -w eeprom.img
if [ $? -ne 0 ]; then
	echo Failed to write to the EEPROM.
	rm eeprom.img
	exit 1
else
	echo Successfully wrote to the EEPROM.
fi
rm eeprom.img

