cd $(dirname $0)

git clone https://gitlab.cern.ch/coht/ht-flasher.git
cd ht-flasher
# Checkout this hash in case something changes in the future
# master branch on July 29th 2021
git checkout -b tmp 8fc29043
git submodule update --init

PATCH_FILE=../0001-Fix-wrong-signatures-for-spec_bus_i2c_eeprom-ops.patch
if [ ! -e $PATCH_FILE ]; then
	echo "Could not find patch file in the script's directory."
	exit 1
fi
patch -p 1 < $PATCH_FILE

make
