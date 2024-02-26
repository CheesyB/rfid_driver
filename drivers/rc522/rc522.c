#include "drivers/rc522.h"

int rc522_write_register(const struct spi_dt_spec *rc522, uint8_t reg, uint8_t write_value) {
	uint8_t tx_buf[2] = {reg, write_value};
	struct spi_buf send_buf = {tx_buf, 2};
	struct spi_buf_set send_buf_set = {&send_buf, 1};
	return spi_write_dt(rc522, &send_buf_set);
}

int rc522_write_register_many(const struct spi_dt_spec *rc522, uint8_t reg, int length, uint8_t *write_values) {
	uint8_t tx_buf[length+1];
	tx_buf[0] = reg;
	memset(tx_buf+1, 0, length);
	memcpy(tx_buf+1, write_values, length);
	struct spi_buf send_buf = {tx_buf, length+1};
	struct spi_buf_set send_buf_set = {&send_buf, 1};
	return spi_write_dt(rc522, &send_buf_set);
}

int rc522_read_register(const struct spi_dt_spec *rc522, uint8_t reg, uint8_t *read_value) {
	uint8_t tx_buf[2] = {0x80 | reg};
	struct spi_buf send_buf = {tx_buf, 2};
	struct spi_buf_set send_buf_set = {&send_buf, 1};

	uint8_t rx_buf[2];
	struct spi_buf receive_buf = {rx_buf, 2};
	struct spi_buf_set receive_buf_set = {&receive_buf, 1};

	int ret;
	ret = spi_transceive_dt(rc522, &send_buf_set, &receive_buf_set);
	*read_value = rx_buf[1];
	return ret;
}

int rc522_read_register_many(const struct spi_dt_spec *rc522, uint8_t reg, int length, uint8_t *read_values) {
	uint8_t tx_buf[length+1];
	memset(tx_buf, 0x80 | reg, length);
	tx_buf[length] = 0;
	struct spi_buf send_buf = {tx_buf, length+1};
	struct spi_buf_set send_buf_set = {&send_buf, 1};

	uint8_t rx_buf[length+1];
	struct spi_buf receive_buf = {rx_buf, length+1};
	struct spi_buf_set receive_buf_set = {&receive_buf, 1};

	int ret;
	ret = spi_transceive_dt(rc522, &send_buf_set, &receive_buf_set);
	memcpy(read_values, rx_buf+1, length);
	return ret;
}

int rc522_test(const struct spi_dt_spec *rc522) {
	/* 1. Perform a soft reset. */
	rc522_write_register(rc522, CommandReg, SoftReset);

	/* 2. Clear the internal buffer by writing 25 bytes of 00h and implement the config command. */
	uint8_t ZEROS[25] = {0};
	rc522_write_register(rc522, FIFOLevelReg, 0x80); // Flush FIFO buffer
	rc522_write_register_many(rc522, FIFODataReg, 25, ZEROS); // Write 25 zeros to FIFO buffer
	rc522_write_register(rc522, CommandReg, Mem); // Transfer zeros to internal buffer
	
	/* 3. Enable the self test by writing 09h to the AutoTestReg register. */
	rc522_write_register(rc522, AutoTestReg, 0x09);

	/* 4. Write 00h to the FIFO buffer. */
	rc522_write_register(rc522, FIFODataReg, 0x00);

	/* 5. Start the self test with the CalcCRC command. */
	rc522_write_register(rc522, CommandReg, CalcCRC);

	/* 6. The self test is initiated. */

	/* 7. When the self test has completed, the FIFO buffer contains the following 64 bytes: */
	// Wait until test is completed
	uint8_t n;
	for (uint8_t i = 0; i < 0xFF; i++) {
		rc522_read_register(rc522, FIFOLevelReg, &n);
		if (n >= 64) {
			break;
		}
	}
	rc522_write_register(rc522, CommandReg, Idle);

	// Read data in FIFO buffer.
	uint8_t buffer[64];
	rc522_read_register_many(rc522, FIFODataReg, 64, buffer);

	rc522_write_register(rc522, AutoTestReg, 0x00);

	/* Print data */
	for (int i = 0; i < 8; i++) {
		for (int j = 0; j < 8; j++) {
			printk("0x%02X ", buffer[8*i+j]);
		}
		printk("\n");
	}
	printk("\n");

	return 0;
}
