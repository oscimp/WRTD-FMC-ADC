#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <adc-lib.h>
#include <adc-lib-100m14b4cha.h>

#define ZIO_ID 0x0000

static void adc_check_error(char *message)
{
	if (errno) {
		fprintf(stderr, "ADC-LIB ERROR: %s: %s\n", message, adc_strerror(errno));
		adc_exit();
		exit(errno);
	}
}

int adc_get_time(struct adc_dev *adc)
{
	struct adc_conf config;
	memset(&config, 0, sizeof(struct adc_conf));
	config.type = ADC_CONF_TYPE_BRD;
	adc_set_conf_mask_all(&config, adc);
	adc_retrieve_config(adc, &config);
	adc_check_error("Failed to retrieve board configuration");

	int time;
	adc_get_conf(&config, ADC_CONF_UTC_TIMING_BASE_S, &time);
	adc_check_error("Failed to get time");

	return time;
}

int main(int argc, char **argv)
{	int zio_id=ZIO_ID;
	if (argc>1) zio_id=strtol(argv[1],NULL,16);
	adc_init();
	adc_check_error("Failed to initialize the library");

	struct adc_dev *adc = adc_open("fmc-adc-100m14b4cha", zio_id, 0, 0, ADC_F_FLUSH);
	adc_check_error("Failed to open device");

	printf("%i\n", adc_get_time(adc));

	adc_exit();
	return EXIT_SUCCESS;
}
