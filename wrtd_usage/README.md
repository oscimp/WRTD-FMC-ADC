# How to use WRTD with the FMC ADC

We will be using the C libraries provided by the WRTD repository as well as the ADC-lib to interact with the SPEC board.

To facilitate basic compilation for a single source file, the script `compile.sh` can be used to automatically load the relevent libraries and include the headers.
It takes the input file as its first command line parameter, and output file as second.

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
You will also need to specify the path to these headers when compiling (`-I` option for gcc).
These are located inside your build directory at `$BUILD_DIR/adc-lib/lib`.
Finally you want to tell gcc to load the library using the `-ladc` option.

The Documentation for adc-lib was provided in html format in this repository to avoid having to build it from sources.

### ZIO device ID

To do an acquisition, it is required to know the ZIO device ID of the ADC card.
If the installation worked and the FPGA is programmed, this ID can be found in sysfs.
A folder named `adc-100m14b-XXXX` should be found at `/sys/bus/zio/devices`, where `XXXX` is the ZIO device ID in hexadecimal.

Running the script `zio_id.sh` should return this ID (printed in hexadecimal).

### Initializing the library

Before you use any function from adc-lib, you should call `adc_init` once. Similarly, `adc_exit` should be called before quitting your program.

The first thing to do after initialization is to get a structure for your device, which will be used by most functions from the library.
To do so we can call `adc_open`.

Its signature is the following:
```c
struct adc_dev *adc_open(char *name,
                         unsigned int dev_id,
                         unsigned long totalsamples
                         unsigned int nbuffer,
                         unsigned long flags)
```
In our case `name` will be `"fmc-adc-100m14b4cha"` and `dev_id` will be the ZIO device ID mentionned above.

Reading the documentation, we get that `totalsamples` is a hint to how big the buffer needs to be, this should be set to `NSHOTS * (PRESAMPLES + POSTSAMPLES)`.
`nbuffer` is a hint to how many buffers are to be used at the same time.
And `flags` can be set to `ADC_F_FLUSH`.

In the end our program should look like the following:
```c
int main()
{
	struct adc_dev *adc;

	adc_init();

	adc = adc_open("fmc_adc_100m14b4cha",
	               ZIO_ID,
                       NSHOTS * (PRESAMPLE + POSTSAMPLE),
	               NSHOTS,
                       ADC_F_FLUSH);

	// Configuring the ADC
	config(adc);
	// Acquiring data
	acquire(adc);

	adc_exit();
	return EXIT_SUCCESS;
}
```

### Error checking

adc-lib uses `errno` to track errors (from the `errno.h` header).
Thus we can check after each adc-lib function call if it went fine.

There is also a function to get a string from an error code: `adc_strerror`.

I like to write and use the following function, and call it after any adc-lib function call where I want to check for errors:
```c
static void adc_check_error(char *message)
{
	if (errno) {
		fprintf(stderr, "ADC-LIB ERROR: %s: %s", message, adc_strerror(errno));
		adc_exit();
		exit(errno);
	}
}
```

### Configuring the ADC

The first step before acquiring any data is to configure the ADC.
There are a lot of parameters that can be changed, which are documented in the adc-lib documentation.

- adc-lib sorts parameters in several categories such as "acquisition", "timing triggers" or "channels" for example. All categories are listed in the `adc_configuration_type` enumeration.
- When setting any parameter, we will have to fill a structure of type `struct adc_conf`.
- We first tell the category of parameters we want by setting the `.type` member.
- Then we can provide values for one or several parameters corresponding to the category using the function `adc_set_conf`. Parameters for each category are listed in an enumeration called `adc_configuration_<category>`.
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

### Acquiring data

To acquire data, we first need to allocate a buffer using `adc_request_buffer`.

The signature of the function is the following:
```c
struct adc_buffer *adc_request_buffer(struct adc_dev *adc,
                                      int nsamples,
                                      void *(*alloc_fn)(size_t),
                                      unsigned int flags)
```
`adc` will be our pointer to the ADC device. `nsamples` will be the total number of samples to acquire `NSHOTS * (PRESAMPLES + POSTSAMPLES)`. And we can leave the allocation function to default with `NULL` and set flags to `0`.

We will need to free the buffer once we are done with the acquisition using `adc_release_buffer`.

Then we can start the acquisition by calling `adc_acq_start`. The ADC will then wait for a trigger before acquiring samples.
It may be good practice to stop any pending acquisition before starting one by calling `adc_acq_stop`.

To fill your buffer with the acquired data, use `adc_fill_buffer`. If no trigger has happened before this function is called, it will wait until one happens, or until the specified timeout is passed.

Finally, here is an example of how an acquisition looks in code:
```c
// Allocating a buffer
struct adc_buffer *buffer;
buffer = adc_request_buffer(adc, NSHOTS * (PRESAMPLES + POSTSAMPLES), NULL, 0);

// Time struct to specify timeouts
struct timeval timeout = { 0, 0 };

// Stopping any pending acquisition
adc_acq_stop(adc, 0);
// Starting a new acquisition immediately (timeout = 0)
adc_acq_start(adc, ADC_F_FLUSH, &timeout);

// Setting a 5s timeout for trigger waiting
timeout.tv_sec = 5;

for (int i = 0; i < NSHOTS; i++) {
	adc_fill_buffer(adc, buffer, ADC_F_FIXUP, &timeout);

	process_buffer(buffer);
}

// Freeing the buffer
adc_release_buffer(adc, buffer, NULL);
```

#### Software triggers

We can trigger manually a trigger in software, which is very handy for testing.
Normally we should be able to use `adc_trigger_fire` according to the documentation.
Unfortunately this function does not work so a workaround is needed.

My workaround was to manually write to a sysfs file using the following function:
```c
static void trigger_fire()
{
	int fd = open("/sys/bus/zio/devices/adc-100m14b-<ZIO ID>/cset0/trigger/sw-trg-fire", O_WRONLY);
	write(fd, "1", 1);
	close(fd);
}
```

#### Time triggers

It is possible to tell the ADC to trigger an acquisition at a certain date.
Note that this date comes from the ADC's internal clock, and this is not how the ADC should be configured for use with WRTD.
Nonetheless this can be useful for testing or other applications.
What WRTD uses is external triggers, which will be discussed later.

We only need to configure the time triggers with the API described earlier.
We can set a date in two parts: seconds and nanoseconds.
```c
// Initializing the configuration to default
struct adc_conf config;
memset(&config, 0, sizeof(struct adc_conf);

// Setting the category (time triggers in this case)
config.type = ADC_CONF_TYPE_TRG_TIM;

// Enabling time trigger
adc_set_conf(&config, ADC_CONF_TRG_TIM_ENABLE, 1);
// Setting seconds
adc_set_conf(&config, ADC_CONF_TRG_TIM_SECONDS, seconds);
// Setting nanoseconds
adc_set_conf(&config, ADC_CONF_TRG_TIM_NANO_SECONDS, nano_seconds);

// Applying the configuration
adc_apply_config(adc, 0, &config);
```

Generally you would want to retrieve the current time of the clock before you set up the trigger.
To do so you can read some variables from the current configuration using `adc_retrieve_config` and `adc_get_conf`.
The API provides 3 variables to get the current time in "seconds", "coarse sub-seconds", and "fine sub-seconds".
Since I generally just use time triggers for testing, I do not need nanosecond precision. Thus I just retrieve the seconds and increment it, and set the nanoseconds to 0.
```c
// Initializing the configuration to default
struct adc_conf config;
memset(&config, 0, sizeof(struct adc_conf);

// Setting the category (board in this case)
config.type = ADC_CONF_TYPE_BRD;

// Retrieving the current configuration
adc_set_conf_mask_all(&config, adc);
adc_retrieve_config(adc, &config);

// Getting the number of seconds
adc_get_conf(&config, ADC_CONF_UTC_TIMING_BASE_S, &seconds);

seconds += 1;
nano_seconds = 0;
```

`adc-time.c` is a test program which prints the time in seconds of the ADC clock.
It was used to test timing triggers with the `adc-acq` tool provided with adc-lib.
If you want to use it, make sure to modify the ZIO_ID macro to the correct value.
