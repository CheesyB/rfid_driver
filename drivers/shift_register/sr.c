#include "drivers/sr.h"

int sr_is_ready_dt(const struct sr_dt_spec *sr) {
	return spi_is_ready_dt((struct spi_dt_spec *)sr);
}

int sr_write_dt(const struct sr_dt_spec *sr, uint8_t data) {
	uint8_t buf_send[1] = {data};
	struct spi_buf send_buffers = {
		buf_send,
		1
	};
	struct spi_buf_set send = {
		&send_buffers,
		1
	};
	return spi_write_dt((struct spi_dt_spec *)sr, &send);
}

int sr_read_dt(const struct sr_dt_spec *sr, uint8_t *buffer) {
	struct spi_buf receive_buffers = {
		buffer,
		1
	};
	struct spi_buf_set receive = {
		&receive_buffers,
		1
	};
	return spi_read_dt((struct spi_dt_spec *)sr, &receive);
}
