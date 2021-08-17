if [ $1 ]; then
	SOURCE=$1
else
	echo "No source file specified"
	exit
fi

if [ $2 ]; then
	OUTFILE=$2
else
	OUTFILE=test
fi

gcc \
	-std=c99 \
	-I${BUILD_DIR}/wrtd/software/include \
	-I${BUILD_DIR}/wrtd/software/lib \
	-I${BUILD_DIR}/adc-lib/lib \
	-I${BUILD_DIR}/fmc-adc-100m14b4cha-sw/kernel \
	-o $OUTFILE \
	$SOURCE \
	-lwrtd \
	-ladc
