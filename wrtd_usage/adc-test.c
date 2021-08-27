#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>

#include <adc-lib.h>
#include <adc-lib-100m14b4cha.h>

#define NSHOTS 1
#define PRESAMPLES 0
#define POSTSAMPLES 100
#define SAMPLES (NSHOTS * (PRESAMPLES + POSTSAMPLES))

#define ZIO_ID 0x0000
#define SW_TRG "/sys/bus/zio/devices/adc-100m14b-0000/cset0/trigger/sw-trg-fire"

#define CSV_FILE "/tmp/data.csv"

static void adc_check_error(char *message) {
	if (errno) {
		fprintf(stderr, "ADC-LIB ERROR: %s: %s\n", message, adc_strerror(errno));
		adc_exit();
		exit(errno);
	}
}

static void config(struct adc_dev *adc) {
	struct adc_conf config;
	memset(&config, 0, sizeof(struct adc_conf));
	config.type = ADC_CONF_TYPE_ACQ;
	adc_set_conf(&config, ADC_CONF_ACQ_N_SHOTS, NSHOTS);
	adc_set_conf(&config, ADC_CONF_ACQ_POST_SAMP, POSTSAMPLES);
	adc_set_conf(&config, ADC_CONF_ACQ_PRE_SAMP, PRESAMPLES);
	adc_set_conf(&config, ADC_CONF_ACQ_UNDERSAMPLE, 0);
	adc_apply_config(adc, 0, &config);
	adc_check_error("Failed to apply acquisition configuration");	
}

static void trigger_fire() {
	int fd = open(SW_TRG, O_WRONLY);
	adc_check_error("Failed to open file descriptor for software trigger");
	
	write(fd, "1", 1);
	close(fd);
	adc_check_error("Failed to close file descriptor for software trigger");
}


static void write_acq(struct adc_buffer *buffer) {
	int value1, value2, value3, value4;

	FILE *file = fopen(CSV_FILE, "w");
	adc_check_error("Failed to open file for writing data");
	
	for (int i = 0; i < PRESAMPLES + POSTSAMPLES; i++) {
		adc_buffer_get_sample(buffer, 0, i, &value1);
		adc_check_error("Failed to get sample");
		adc_buffer_get_sample(buffer, 1, i, &value2);
		adc_check_error("Failed to get sample");
		adc_buffer_get_sample(buffer, 2, i, &value3);
		adc_check_error("Failed to get sample");
		adc_buffer_get_sample(buffer, 3, i, &value4);
		adc_check_error("Failed to get sample");
		fprintf(file, "%i,%i,%i,%i,%i\n", i, value1, value2, value3, value4);
		adc_check_error("Could not write data");
	}

	fclose(file);
	adc_check_error("Failed to close file descriptor for software trigger");
}

static void acquire(struct adc_dev *adc) {
	struct adc_buffer *buffer = adc_request_buffer(adc, SAMPLES, NULL,  0);
	adc_check_error("Failed to allocate buffer");

	struct timeval timeout = { 0, 0 };

	adc_acq_stop(adc, 0);
	adc_check_error("Failed to stop acquisition");
	adc_acq_start(adc, ADC_F_FLUSH, &timeout);
	adc_check_error("Failed to start acquisition");

	timeout.tv_sec = 5;

	for (int i = 0; i < NSHOTS; i++) {
		//adc_trigger_fire(adc);
		//adc_check_error("Failed to fire software trigger");
		trigger_fire();

		adc_fill_buffer(adc, buffer, ADC_F_FIXUP, &timeout);
		adc_check_error("Failed to fill buffer");
	}

	write_acq(buffer);

	adc_release_buffer(adc, buffer, NULL);
	adc_check_error("Failed to release buffer");
}

int main() {
	adc_init();
	adc_check_error("Failed to initialize the library");

	struct adc_dev *adc = adc_open("fmc-adc-100m14b4cha", ZIO_ID, SAMPLES, NSHOTS, ADC_F_FLUSH);
	adc_check_error("Failed to open device");

	config(adc);

	acquire(adc);

	adc_exit();
	return EXIT_SUCCESS;
}
