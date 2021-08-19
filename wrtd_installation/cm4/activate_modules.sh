modprobe i2c-mux

cd /lib/modules/$(uname -r)

insmod zio.ko
insmod buffers/zio-buf-vmalloc.ko
insmod fmc.ko
insmod fmc-adc-100m14b4ch.ko

insmod fpga-mgr.ko
insmod htvic.ko
insmod i2c-ocores.ko
insmod spi-ocores.ko
insmod gn412x-fcl.ko
insmod gn412x-gpio.ko
insmod spec-gn412x-dma.ko
insmod spec-fmc-carrier.ko

insmod mockturtle.ko
insmod wrtd-ref-spec150t-adc.ko

