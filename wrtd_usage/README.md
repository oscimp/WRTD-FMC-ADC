# How to use WRTD with the FMC ADC

## 1. Acquiring data without WRTD

The ADC has actually very few connections with WRTD.
In fact the only interaction the two have is a trigger.
Thus we can learn how to use the ADC before looking at WRTD.

To interact with the ADC, we will be using the adc-lib C library, which should be installed if you ran the installation scripts.

To use adc-lib, you have to include the following headers:
```c
#include <adc-lib.h>
#include <adc-lib-100m14b4cha.h>
```
You may also need to specify the path to these headers when compiling (-I option for gcc). These are located inside your build directory at `$BUILD_DIR/adc-lib/lib`.
Finally you want to tell gcc to load the library using the `-ladc` option.

### ZIO device ID

To do an acquisition, it is required to know the ZIO device ID of the ADC card.
If the installation worked and the FPGA is programmed, this ID can be found in sysfs.
A folder named `adc-100m14b-XXXX` should be found at `/sys/bus/zio/devices`, where `XXXX` is the ZIO device ID in hexadecimal.

Running the script `zio_id.sh` should return this ID (printed in hexadecimal).

### Initializing the library

TODO

### Configuring the ADC

The first step before acquiring any data is to configure the ADC.
There are a lot of parameters that can be changed, which are somewhat documented in the adc-lib documentation.

- adc-lib sorts parameters in several categories such as "acquisition", "timing triggers" or "channels" for example.
- When setting any parameter, we will have to fill a structure of type `struct adc_conf`.
- We first tell the category of parameters we want by setting the `.type` member.
- Then we can provide values for one or several parameters corresponding to the category using the function `adc_set_conf`.
- Finally we apply the changes with the function `adc_apply_config`.

Here is an example for setting the number of samples:
```c
// Initializing the configuration to default
struct adc_conf config;
memset(&config, 0, sizeof(struct adc_conf);

// Setting the category (acquisition in this case)
config.type = ADC_CONF_TYPE_ACQ;

// Setting a value for the number of samples to acquire after the trigger
adc_set_conf(&config, ADC_CONF_ACQ_POST_SAMP, 10);
// You can set several parameters at the same time as long as they fall in the same category
adc_set_conf(&config, ADC_CONF_ACQ_PRE_SAMP, 0);

// Applying the configuration (adc is your adc_dev struct pointer, 0 if a flag)
adc_apply_config(adc, 0, &config);
```

The most important parameters for our purposes are the number of samples to be acquired and the various triggers we would want to use.
