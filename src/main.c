#include <zephyr/kernel.h>
#include "drivers/rc522.h"

static const struct spi_dt_spec rc522 = RC522_SPI_DT_SPEC_GET(DT_NODELABEL(rc522));

int main() {
	rc522_test(&rc522);

	return 0;
}
