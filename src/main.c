#include <zephyr/kernel.h>
#include "drivers/rc522.h"

static const struct spi_dt_spec rc522 = RC522_SPI_DT_SPEC_GET(DT_NODELABEL(rc522));

int main() {
	int ret;

	rc522_init(&rc522);

	while (1) {
		ret = rc522_reqa(&rc522);
		if (ret != STATUS_OK) {
			k_msleep(250);
			continue;
		}

		uint8_t UID[5] = {0};
		rc522_select(&rc522, UID, 0, NULL);

		ret = rc522_select(&rc522, UID, 1, NULL);
		if (ret != STATUS_OK) {
			continue;
		}

		const uint8_t block = 1;
		const uint8_t key[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
		rc522_mifare_auth(&rc522, block, key, UID);

		uint8_t length = 18;
		uint8_t read_values[18] = {0};
		rc522_mifare_read(&rc522, block, &length, read_values);

		read_values[0]++;
		rc522_mifare_write(&rc522, block, 16, read_values);

		rc522_mifare_deauth(&rc522);

		k_msleep(1500);
	}

	return 0;
}
