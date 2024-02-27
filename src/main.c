#include <zephyr/kernel.h>
#include "drivers/rc522.h"

static const struct spi_dt_spec rc522 = RC522_SPI_DT_SPEC_GET(DT_NODELABEL(rc522));

int main() {
	int ret;

	rc522_init(&rc522);

	uint8_t atqa[2] = {0};
	uint8_t UID[4] = {0};
	uint8_t sak[1] = {0};
	while (1) {
		k_msleep(5000);

		ret = rc522_reqa(&rc522, atqa);
		if (ret != STATUS_OK) {
			printk("Error: %d\n", ret);
			continue;
		} else {
			printk("Received ATQA: 0x%02X 0x%02X\n", atqa[1], atqa[0]);
		}

		ret = rc522_select(&rc522, UID, 0, NULL);
		if (ret != STATUS_OK) {
			printk("Error: %d\n", ret);
			continue;
		} else {
			printk("Received UID: 0x%02X 0x%02X 0x%02X 0x%02X\n", UID[0], UID[1], UID[2], UID[3]);
		}

		ret = rc522_select(&rc522, UID, 1, sak);
		if (ret != STATUS_OK) {
			printk("Error: %d\n", ret);
			continue;
		} else {
			printk("Received SAK: 0x%02X\n", sak[0]);
		}
	}

	return 0;
}
