#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>

#include <libwrtd.h>

static void wrtd_check_status(wrtd_status status, int line)
{
	char status_str[256];
	if (status != WRTD_SUCCESS) {
		wrtd_error_message(NULL, status, status_str);
		fprintf(stderr, "WRTD ERROR: Line %i, %s\n", line,  status_str);
		exit(status);
	}
	errno = 0;
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
		fprintf(stderr, "WRTD time is not synchronized");
		wrtd_close(wrtd);
		exit(EXIT_FAILURE);
	}
}

int main()
{
	struct wrtd_dev *wrtd;
	uint32_t node_id;
	struct adc_dev *adc;

	// Initialize WRTD
	wrtd_check_status(wrtd_get_node_id(1, &node_id), __LINE__);
	wrtd_check_status(wrtd_init(node_id, false, NULL, &wrtd), __LINE__);

	check_sync(wrtd);	

	wrtd_check_status(wrtd_close(wrtd), __LINE__);
	return EXIT_SUCCESS;
}
