#include <zephyr/kernel.h>
#include "fob.h"

static const struct spi_dt_spec rc522 = RC522_SPI_DT_SPEC_GET(DT_NODELABEL(rc522));

int main() {
	int ret;

	rc522_init(&rc522);

	while (1) {
		const uint8_t sector = 1;
		const uint8_t key[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
		const uint8_t new_key[6] = {0x00};

		fob_write_key(&rc522, sector, new_key, new_key);
		fob_write_key(&rc522, sector, key, new_key);
		fob_write_key(&rc522, sector, key, key);
		fob_write_key(&rc522, sector, new_key, key);

		const uint8_t passcode[6] = {0xAB};
		uint8_t passcode_read[6] = {0};

		fob_read_passcode(&rc522, sector, key, passcode_read);
		printf("0x%02X\n", passcode_read[0]);

		fob_write_passcode(&rc522, sector, key, passcode);

		fob_read_passcode(&rc522, sector, key, passcode_read);
		printf("0x%02X\n", passcode_read[0]);

		passcode_read[0] = 0x00;
		fob_write_passcode(&rc522, sector, key, passcode_read);

		k_msleep(1500);
	}

	return 0;
}
