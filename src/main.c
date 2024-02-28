#include <zephyr/kernel.h>
#include "drivers/rc522.h"

static const struct spi_dt_spec rc522 = RC522_SPI_DT_SPEC_GET(DT_NODELABEL(rc522));

int main() {
	int ret;

	rc522_init(&rc522);

	uint8_t atqa[2] = {0};
	uint8_t UID[5] = {0};
	uint8_t sak[1] = {0};
	uint8_t key[6];
	memset(key, 0xFF, 6);
	uint8_t read_values[18] = {0};
	while (1) {
		k_msleep(3000);

		ret = rc522_reqa(&rc522, atqa);
		rc522_print_status(ret);
		if (ret != STATUS_OK) {
			continue;
		}

		ret = rc522_select(&rc522, UID, 0, NULL);
		rc522_print_status(ret);
		ret = rc522_select(&rc522, UID, 1, sak);
		rc522_print_status(ret);
		ret = rc522_authenticate(&rc522, 0, key, UID);
		rc522_print_status(ret);

		uint8_t length;
		for (int i = 0; i < 4; i++) {
			length = 18;
			ret = rc522_read(&rc522, i, &length, read_values);
			rc522_print_status(ret);
		}

		rc522_deauthenticate(&rc522);
	}

	return 0;
}
