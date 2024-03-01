#include <zephyr/kernel.h>
#include "drivers/rc522.h"
#include "fob.h"

static const struct spi_dt_spec rc522 = RC522_SPI_DT_SPEC_GET(DT_NODELABEL(rc522));

int main() {
	int ret;

	rc522_init(&rc522);

	while (1) {
		const uint8_t sector = 1;
		const uint8_t key[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
		const uint8_t new_key[6] = {0x00};
		uint8_t UID[5] = {0};

		while (1) {
			ret = rc522_reqa(&rc522);
			if (ret != STATUS_OK) {
				k_msleep(250);
				continue;
			}

			ret = fob_select(&rc522, UID);
			if (ret != STATUS_OK) {
				continue;
			}

			break;
		}

		fob_auth(&rc522, UID, sector, key);
		fob_write_key(&rc522, UID, sector, new_key);

		fob_auth(&rc522, UID, sector, new_key);
		fob_write_key(&rc522, UID, sector, key);


		const uint8_t passcode[6] = {0xAB};
		uint8_t passcode_read[6] = {0};

		fob_auth(&rc522, UID, sector, key);

		fob_read_passcode(&rc522, UID, sector, passcode_read);
		printf("0x%02X\n", passcode_read[0]);

		fob_write_passcode(&rc522, UID, sector, passcode);

		fob_read_passcode(&rc522, UID, sector, passcode_read);
		printf("0x%02X\n", passcode_read[0]);

		passcode_read[0] = 0x00;
		fob_write_passcode(&rc522, UID, sector, passcode_read);


		rc522_mifare_deauth(&rc522);

		k_msleep(1500);
	}

	return 0;
}
