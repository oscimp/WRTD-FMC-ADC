#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>

#include <libwrtd.h>
#include <adc-lib.h>
#include <adc-lib-100m14b4cha.h>
#include <fmc-adc-100m14b4cha.h>

#define NSHOTS 1
#define PRESAMPLE 0
#define POSTSAMPLE 100

#define ZIO_ID 0x0000

#define CSV_FILE "/tmp/data.csv"

static char *rule_msg_id = "RULE-MSG";
static char *rule_trg_id = "RULE-TRG";
static char *alarm_id = "ALARM-1";
static char *message_id = "MESSAGE";
static char *trigger_id = "LC-O1";

void wrtd_check_status(wrtd_status status, int line)
{
	char status_str[256];
	if (status != WRTD_SUCCESS) {
		wrtd_error_message(NULL, status, status_str);
		fprintf(stderr, "WRTD ERROR: Line %i, %s\n", line,  status_str);
		exit(status);
	}
	errno = 0;
}

static void adc_check_error(char *message)
{
	if (errno) {
		fprintf(stderr, "ADC-LIB ERROR: %s: %s\n", message, adc_strerror(errno));
		adc_exit();
		exit(errno);
	}
}

static inline uint32_t adc_ticks_to_ns(uint32_t ticks)
{
	return ticks * FA100M14B4C_UTC_CLOCK_NS;
}

/**
 * Checks if time is synchronized to White Rabbit
 */
static void check_sync(struct wrtd_dev *wrtd)
{
	bool is_synced;
	wrtd_check_status(wrtd_get_attr_bool(wrtd, WRTD_GLOBAL_REP_CAP_ID, WRTD_ATTR_IS_TIME_SYNCHRONIZED, &is_synced), __LINE__);
	if (is_synced)
		printf("WRTD time is synchronized.\n");
	else {
		fprintf(stderr, "WRTD time is not synchronized.\n");
		wrtd_close(wrtd);
		exit(EXIT_FAILURE);
	}
}

/**
 * Creates an alarm which will 
 * trigger the acquisition in 2s
 */
void config_alarm(struct wrtd_dev *wrtd)
{
	// Remove all existing alarms
	wrtd_check_status(wrtd_disable_all_alarms(wrtd), __LINE__);
	wrtd_check_status(wrtd_remove_all_alarms(wrtd), __LINE__);

	wrtd_check_status(wrtd_add_alarm(wrtd, alarm_id), __LINE__);
	// Set repeat to only once
	struct wrtd_tstamp tstamp = { .seconds = 0, .ns = 0, .frac = 0 };
	wrtd_check_status(wrtd_set_attr_tstamp(wrtd, alarm_id, WRTD_ATTR_ALARM_PERIOD, &tstamp), __LINE__);
	// Set time for alarm triggering 2s after current time
	wrtd_check_status(wrtd_get_attr_tstamp(wrtd, WRTD_GLOBAL_REP_CAP_ID, WRTD_ATTR_SYS_TIME, &tstamp), __LINE__);
	tstamp.seconds += 1;
	wrtd_check_status(wrtd_set_attr_tstamp(wrtd, alarm_id, WRTD_ATTR_ALARM_SETUP_TIME, &tstamp), __LINE__);
	tstamp.seconds += 1;
	wrtd_check_status(wrtd_set_attr_tstamp(wrtd, alarm_id, WRTD_ATTR_ALARM_TIME, &tstamp), __LINE__);
	// Enable alarm
	wrtd_check_status(wrtd_set_attr_bool(wrtd, alarm_id, WRTD_ATTR_ALARM_ENABLED, true), __LINE__);
}

/**
 * Creates two rules which will trigger the ADC
 * once the alarm rings and send a message to the slave(s).
 */
void config_rule(struct wrtd_dev *wrtd)
{
	// Remove all existing rules
	wrtd_check_status(wrtd_disable_all_rules(wrtd), __LINE__);
	wrtd_check_status(wrtd_remove_all_rules(wrtd), __LINE__);

	// Create rule
	wrtd_check_status(wrtd_add_rule(wrtd, rule_msg_id), __LINE__);
	wrtd_check_status(wrtd_add_rule(wrtd, rule_trg_id), __LINE__);
	// Set input events
	wrtd_check_status(wrtd_set_attr_string(wrtd, rule_msg_id, WRTD_ATTR_RULE_SOURCE, alarm_id), __LINE__);
	wrtd_check_status(wrtd_set_attr_string(wrtd, rule_trg_id, WRTD_ATTR_RULE_SOURCE, alarm_id), __LINE__);
	// Set output events
	wrtd_check_status(wrtd_set_attr_string(wrtd, rule_msg_id, WRTD_ATTR_RULE_DESTINATION, message_id), __LINE__);
	wrtd_check_status(wrtd_set_attr_string(wrtd, rule_trg_id, WRTD_ATTR_RULE_DESTINATION, trigger_id), __LINE__);
	// Add delay to the event timestamp
	struct wrtd_tstamp delay = { .seconds = 0, .ns = 0, .frac = 0 };
	wrtd_check_status(wrtd_set_attr_tstamp(wrtd, rule_msg_id, WRTD_ATTR_RULE_DELAY, &delay), __LINE__);
	delay.ns = 500e3;
	wrtd_check_status(wrtd_set_attr_tstamp(wrtd, rule_trg_id, WRTD_ATTR_RULE_DELAY, &delay), __LINE__);
	// Enable rules
	wrtd_check_status(wrtd_set_attr_bool(wrtd, rule_msg_id, WRTD_ATTR_RULE_ENABLED, true), __LINE__);
	wrtd_check_status(wrtd_set_attr_bool(wrtd, rule_trg_id, WRTD_ATTR_RULE_ENABLED, true), __LINE__);
}

/**
 * Sets up the paramaters of the ADC for
 * acquisition and external trigger
 */
static void config_adc(struct adc_dev *adc)
{
	struct adc_conf config;

	// Acquisition setup
	memset(&config, 0, sizeof(struct adc_conf));
	config.type = ADC_CONF_TYPE_ACQ;
	adc_set_conf(&config, ADC_CONF_ACQ_N_SHOTS, NSHOTS);
	adc_set_conf(&config, ADC_CONF_ACQ_POST_SAMP, POSTSAMPLE);
	adc_set_conf(&config, ADC_CONF_ACQ_PRE_SAMP, PRESAMPLE);
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
void write_acq(struct adc_dev *adc, struct adc_buffer *buffer)
{
	int value1, value2, value3, value4;

	// Getting the time of the 1st sample
	struct adc_timestamp timestamp;
	adc_tstamp_buffer(buffer, &timestamp);
	uint32_t time = adc_ticks_to_ns(timestamp.ticks);

	FILE *file = fopen(CSV_FILE, "w");
	adc_check_error("Failed to open file for writing data");
	
	for (int i = 0; i < PRESAMPLES + POSTSAMPLES; i++) {
		adc_buffer_get_sample(buffer, 0, i, &value1);
		adc_buffer_get_sample(buffer, 1, i, &value2);
		adc_buffer_get_sample(buffer, 2, i, &value3);
		adc_buffer_get_sample(buffer, 3, i, &value4);
		adc_check_error("Failed to get sample");
		fprintf(file, "%lu,%i,%i,%i,%i\n", time, value1, value2, value3, value4);
		adc_check_error("Could not write data");
		time += 10;
	}

	fclose(file);
	adc_check_error("Failed to close file descriptor for software trigger");
}

/**
 * Acquires data from the ADC
 */
void acquire(struct adc_dev *adc)
{
	struct adc_buffer *buffer = adc_request_buffer(adc, NSHOTS * (PRESAMPLE + POSTSAMPLE), NULL,  0);
	adc_check_error("Failed to allocate buffer");

	struct timeval timeout = { .tv_sec = 0, .tv_usec = 0 };

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

int main()
{
	struct wrtd_dev *wrtd;
	uint32_t node_id;
	struct adc_dev *adc;

	// WRTD initialization
	wrtd_check_status(wrtd_get_node_id(1, &node_id), __LINE__);
	wrtd_check_status(wrtd_init(node_id, false, NULL, &wrtd), __LINE__);
	
	check_sync(wrtd);
	config_alarm(wrtd);
	config_rule(wrtd);
	printf("Master configured.\n");
	wrtd_check_status(wrtd_close(wrtd), __LINE__);
	
	// ADC-lib initialization
	adc_init();
	adc_check_error("Failed to initialize the library");

	adc = adc_open("fmc-adc-100m14b4cha", ZIO_ID, 0, 0, ADC_F_FLUSH);
	adc_check_error("Failed to open device");
	config_adc(adc);
	acquire(adc);

	adc_close(adc);
	adc_check_error("Failed to close device");
	adc_exit();
	return EXIT_SUCCESS;
}
