#pragma once

#include <zephyr/drivers/spi.h>

struct sr_dt_spec {
	const struct device *bus;
	struct spi_config config;
};

#define SR_DT_SPEC_GET(node_label) \
	SPI_DT_SPEC_GET( \
		node_label, \
		SPI_OP_MODE_MASTER | SPI_TRANSFER_MSB | SPI_WORD_SET(8) | SPI_LINES_SINGLE, \
		0 \
	)

int sr_is_ready_dt(const struct sr_dt_spec *sr);
int sr_write_dt(const struct sr_dt_spec *sr, uint8_t data);
int sr_read_dt(const struct sr_dt_spec *sr, uint8_t *buffer);
