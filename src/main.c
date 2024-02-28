#include <zephyr/kernel.h>
#include "drivers/rc522.h"

static const struct spi_dt_spec rc522 = RC522_SPI_DT_SPEC_GET(DT_NODELABEL(rc522));

int main() {
	int ret;

	rc522_init(&rc522);

	while (1) {
		uint8_t atqa[2] = {0};
		ret = rc522_reqa(&rc522, atqa);
		if (ret != STATUS_OK) {
			continue;
		}
		rc522_print_status(ret);

		uint8_t UID[5] = {0};
		ret = rc522_select(&rc522, UID, 0, NULL);
		rc522_print_status(ret);

		uint8_t sak[1] = {0};
		ret = rc522_select(&rc522, UID, 1, sak);
		rc522_print_status(ret);
		if (ret != STATUS_OK) {
			continue;
		}

		uint8_t key[6];
		memset(key, 0xFF, 6);
		ret = rc522_authenticate(&rc522, 0, key, UID);
		rc522_print_status(ret);

		uint8_t length = 18;
		uint8_t read_values[18] = {0};
		ret = rc522_read(&rc522, 1, &length, read_values);
		rc522_print_status(ret);

		read_values[0]++;
		ret = rc522_mifare_write(&rc522, 1, 16, read_values);
		rc522_print_status(ret);

		rc522_deauthenticate(&rc522);

		k_msleep(1000);
	}

	return 0;
}
