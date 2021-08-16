# How to use WRTD with the FMC ADC

## 1. Acquiring data without WRTD

The ADC has actually very few connections with WRTD.
In fact the only interaction the two have is a trigger.
Thus we can learn how to use the ADC before looking at WRTD.

### ZIO device ID

To do an acquisition, it is required to know the ZIO device ID of the ADC card.
If the installation worked and the FPGA is programmed, this ID can be found in sysfs.
A folder named `adc-100m14b-XXXX` should be found at `/sys/bus/zio/devices`, where `XXXX` is the ZIO device ID in hexadecimal.

Running the script `zio_id.sh` should return this ID (printed in hexadecimal).
