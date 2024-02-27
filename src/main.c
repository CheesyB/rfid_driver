#include <zephyr/kernel.h>
#include "drivers/rc522.h"

static const struct spi_dt_spec rc522 = RC522_SPI_DT_SPEC_GET(DT_NODELABEL(rc522));

int main() {
	int ret;

	rc522_init(&rc522);

	uint8_t atqa[2] = {0};
	while (1) {
		k_msleep(1000);

		ret = rc522_reqa(&rc522, atqa);
		if (ret != STATUS_OK) {
			printk("Error: %d\n", ret);
		} else {
			printk("ATQA: 0x%02X 0x%02X\n", atqa[1], atqa[0]);
		}
	}

	return 0;
}
