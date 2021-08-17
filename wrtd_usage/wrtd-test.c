#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>

#include <libwrtd.h>
#include <adc-lib.h>
#include <adc-lib-100m14b4cha.h>

#define NSHOTS 1
#define PRESAMPLES 0
#define POSTSAMPLES 100
#define SAMPLES (NSHOTS * (PRESAMPLES + POSTSAMPLES))

#define ZIO_ID 0x0000

#define CSV_FILE "/tmp/data.csv"

static char *rule_id = "RULE-1";
static char *alarm_id = "ALARM-1";
static char *trigger_id = "LC-O1";

void wrtd_check_status(wrtd_status status, int line) {
	char status_str[256];
	if (status != WRTD_SUCCESS) {
		wrtd_error_message(NULL, status, status_str);
		printf("WRTD ERROR: Line %i, %s\n", line,  status_str);
		exit(status);
	}
	errno = 0;
}

void wrtd_check_error(wrtd_dev *wrtd, wrtd_status status) {
	if (status != WRTD_SUCCESS) {
		int buf_size = wrtd_get_error(wrtd, NULL, 0, NULL);
		wrtd_status err_code;
		char err_msg[buf_size];
		wrtd_get_error(wrtd, &err_code, buf_size, err_msg);
		printf("WRTD ERROR: %d, %s\n", err_code,  err_msg);
		exit(status);
	}
}

static void adc_check_error(char *message) {
	if (errno) {
		fprintf(stderr, "ADC-LIB ERROR: %s: %s\n", message, adc_strerror(errno));
		adc_exit();
		exit(errno);
	}
}

/**
  * Creates an alarm which will 
  * trigger the acquisition in 2s
  */
void config_alarm(struct wrtd_dev *wrtd) {
	// Remove all existing alarms
	wrtd_check_status(wrtd_disable_all_alarms(wrtd), __LINE__);
	wrtd_check_status(wrtd_remove_all_alarms(wrtd), __LINE__);
	// Create alarm
	wrtd_check_status(wrtd_add_alarm(wrtd, alarm_id), __LINE__);
	// Set repeat to only once
	struct wrtd_tstamp tstamp = { 0, 0, 0 };
	wrtd_check_status(wrtd_set_attr_tstamp(wrtd, alarm_id, WRTD_ATTR_ALARM_PERIOD, &tstamp), __LINE__);
	// Set time for alarm triggering 2s after current time
	wrtd_check_status(wrtd_get_attr_tstamp(wrtd, WRTD_GLOBAL_REP_CAP_ID, WRTD_ATTR_SYS_TIME, &tstamp), __LINE__);
	tstamp.seconds += 1;
	wrtd_check_status(wrtd_set_attr_tstamp(wrtd, alarm_id, WRTD_ATTR_ALARM_SETUP_TIME, &tstamp), __LINE__);
	tstamp.seconds += 1;
	wrtd_check_status(wrtd_set_attr_tstamp(wrtd, alarm_id, WRTD_ATTR_ALARM_TIME, &tstamp), __LINE__);
	// Enable alarm
	wrtd_check_status(wrtd_set_attr_bool(wrtd, alarm_id, WRTD_ATTR_ALARM_ENABLED, true), __LINE__);
	printf("Alarm configured!\n");
}

/**
  * Creates a rule which will trigger the ADC
  * once the alarm rings.
  */
void config_rule(struct wrtd_dev *wrtd) {
	// Remove all existing rules
	wrtd_check_status(wrtd_disable_all_rules(wrtd), __LINE__);
	wrtd_check_status(wrtd_remove_all_rules(wrtd), __LINE__);
	// Create rule
	wrtd_check_status(wrtd_add_rule(wrtd, rule_id), __LINE__);
	// Set input event as an alarm
	wrtd_check_status(wrtd_set_attr_string(wrtd, rule_id, WRTD_ATTR_RULE_SOURCE, alarm_id), __LINE__);
	// Set output event as the local channel which triggers the ADC
	wrtd_check_status(wrtd_set_attr_string(wrtd, rule_id, WRTD_ATTR_RULE_DESTINATION, trigger_id), __LINE__);
	// Enable rule
	wrtd_check_status(wrtd_set_attr_bool(wrtd, rule_id, WRTD_ATTR_RULE_ENABLED, true), __LINE__);
	printf("Rule configured!\n");
}

/**
  * Sets up the paramaters of the ADC for
  * acquisition and external trigger
  */
static void config_adc(struct adc_dev *adc) {
	struct adc_conf config;

	// Acquisition setup
	memset(&config, 0, sizeof(struct adc_conf));
	config.type = ADC_CONF_TYPE_ACQ;
	adc_set_conf(&config, ADC_CONF_ACQ_N_SHOTS, NSHOTS);
	adc_set_conf(&config, ADC_CONF_ACQ_POST_SAMP, POSTSAMPLES);
	adc_set_conf(&config, ADC_CONF_ACQ_PRE_SAMP, PRESAMPLES);
	adc_set_conf(&config, ADC_CONF_ACQ_UNDERSAMPLE, 0);
	adc_apply_config(adc, 0, &config);
	adc_check_error("Failed to apply acquisition configuration");

	// External trigger setup
	memset(&config, 0, sizeof(struct adc_conf));
	config.type = ADC_CONF_TYPE_TRG_EXT;
	adc_set_conf(&config, ADC_CONF_TRG_EXT_ENABLE, true);
	adc_set_conf(&config, ADC_CONF_TRG_EXT_POLARITY, ADC_TRG_POL_POS);
	adc_set_conf(&config, ADC_CONF_TRG_EXT_DELAY, 0);
	adc_apply_config(adc, 0, &config);
	adc_check_error("Failed to apply external trigger configuration");
}


/**
  * Writes the data from a buffer into a file in csv format
  */
void write_acq(struct adc_dev *adc, struct adc_buffer *buffer) {
	int value1, value2, value3, value4;
	double time = 0;
	FILE *file = fopen(CSV_FILE, "w");
	adc_check_error("Failed to open file for writting data");
	
	for (int i = 0; i < PRESAMLPLES + POSTSAMPLES; i++) {
		adc_buffer_get_sample(buffer, 0, i, &value1);
		adc_buffer_get_sample(buffer, 1, i, &value2);
		adc_buffer_get_sample(buffer, 2, i, &value3);
		adc_buffer_get_sample(buffer, 3, i, &value4);
		adc_check_error("Failed to get sample");
		fprintf(file, "%i,%i,%i,%i,%i\n", i, value1, value2, value3, value4);
		adc_check_error("Could not write data");
	}

	fclose(file);
	adc_check_error("Failed to close file descriptor for software trigger");
}

/**
  * Acquires data from the ADC
  */
void acquire(struct adc_dev *adc) {
	struct adc_buffer *buffer = adc_request_buffer(adc, SAMPLES, NULL,  0);
	adc_check_error("Failed to allocate buffer");

	struct timeval timeout = { 0, 0 };

	adc_acq_stop(adc, 0);
	adc_check_error("Failed to stop acquisition");
	adc_acq_start(adc, ADC_F_FLUSH, &timeout);
	adc_check_error("Failed to start acquisition");

	timeout.tv_sec = 5;

	for (int i = 0; i < NSHOTS; i++) {
		adc_fill_buffer(adc, buffer, ADC_F_FIXUP, &timeout);
		adc_check_error("Failed to fill buffer");
	}

	write_acq(adc, buffer);

	adc_release_buffer(adc, buffer, NULL);
	adc_check_error("Failed to release buffer");
}

int main() {
	struct wrtd_dev *wrtd;
	uint32_t node_id;
	struct adc_dev *adc;

	// Initialize WRTD
	wrtd_check_status(wrtd_get_node_id(1, &node_id), __LINE__);
	wrtd_check_status(wrtd_init(node_id, false, NULL, &wrtd), __LINE__);
	
	config_alarm(wrtd);
	config_rule(wrtd);

	wrtd_check_status(wrtd_close(wrtd), __LINE__);

	// Initialize ADC-lib
	adc_init();
	adc_check_error("Failed to initialize the library");

	adc = adc_open("fmc-adc-100m14b4cha", ZIO_ID, SAMPLES, NSHOTS, ADC_F_FLUSH);
	adc_check_error("Failed to open device");

	config_adc(adc);
	acquire(adc);

	adc_close(adc);
	adc_check_error("Failed to close device");
	adc_exit();
	return EXIT_SUCCESS;
}
