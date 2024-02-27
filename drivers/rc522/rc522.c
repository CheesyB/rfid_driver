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

int rc522_read_register_many(const struct spi_dt_spec *rc522, uint8_t reg, int length, uint8_t *read_values, uint8_t rx_align) {
	if (length == 0) return 0;

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

	memcpy(read_values+1, rx_buf+2, length-1); // Copy all except first byte.
	
	// Handle alignment of first byte.
	uint8_t mask = (0xFF << rx_align) & 0xFF;
	read_values[0] = (read_values[0] & ~mask) | (rx_buf[1] & mask);
	
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
	// Wait until test is completed.
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
	rc522_read_register_many(rc522, FIFODataReg, 64, buffer, 0);

	rc522_write_register(rc522, AutoTestReg, 0x00);

	/* Print data. */
	for (int i = 0; i < 8; i++) {
		for (int j = 0; j < 8; j++) {
			printk("0x%02X ", buffer[8*i+j]);
		}
		printk("\n");
	}
	printk("\n");

	return STATUS_OK;
}

int rc522_init(const struct spi_dt_spec *rc522) {
	rc522_write_register(rc522, CommandReg, SoftReset);
	/*uint8_t count = 0;
	do {
		k_msleep(50);
		uint8_t reg;
		rc522_read_register(rc522, CommandReg, &reg);
		if (!(reg & 0x10)) {
			break;
		}
	} while (++count < 3);*/

	rc522_write_register(rc522, TxModeReg, 0x00); // Enable CRC and use 106kBd.
	rc522_write_register(rc522, RxModeReg, 0x00); // Enable CRC and use 106kBd.

	rc522_write_register(rc522, ModWidthReg, 0x26); // Reset Miller modulation width.

	rc522_write_register(rc522, TModeReg, 0x80); // Timer starts at end of transmission.
	rc522_write_register(rc522, TPrescalerReg, 0xA9); // Set timer period to 25 us.
	rc522_write_register(rc522, TReloadRegH, 0x03);
	rc522_write_register(rc522, TReloadRegL, 0xE8); // Set timeout to 25 ms.

	rc522_write_register(rc522, TxASKReg, 0x40); // Use 100% ASK
	rc522_write_register(rc522, ModeReg, 0x3D); // Set CRC preset to 6363h.

	uint8_t reg;
	rc522_read_register(rc522, TxControlReg, &reg);
	rc522_write_register(rc522, TxControlReg, reg | 0x03); // Enable antenna.
	
	return STATUS_OK;
}

int rc522_communicate(const struct spi_dt_spec *rc522, struct communicate_argument *arg) {
	uint8_t tx_last_bits = arg->valid_bits;
	uint8_t bit_framing = (arg->rx_align << 4) + tx_last_bits;

	rc522_write_register(rc522, CommandReg, Idle);
	rc522_write_register(rc522, ComIrqReg, 0x7f); // Clear interrupt bits.
	rc522_write_register(rc522, FIFOLevelReg, 0x80); // Flush FIFO buffer.
	rc522_write_register_many(rc522, FIFODataReg, arg->transmit_len, arg->transmit_data);
	rc522_write_register(rc522, BitFramingReg, bit_framing);
	rc522_write_register(rc522, CommandReg, arg->command);
	if (arg->command == Transceive) {
		rc522_write_register(rc522, BitFramingReg, bit_framing | 0x80);
	}

	uint32_t start = k_uptime_get_32();
	int completed = 0;
	do {
		uint8_t irq;
		rc522_read_register(rc522, ComIrqReg, &irq);
		if (irq & arg->wait_irq) {
			completed = 1;
			break;
		}
		if (irq & 0x01) { // 25 ms timer interrupt.
			return STATUS_TIMEOUT;
		}
		k_yield();
	} while (k_uptime_get_32() - start < 36);
	
	if (!completed) {
		return STATUS_TIMEOUT;
	}

	uint8_t error_reg_value;
	rc522_read_register(rc522, ErrorReg, &error_reg_value);
	if (error_reg_value & 0x13) { // BufferOvfl ParityErr ProtocolErr
		return STATUS_ERROR;
	}

	if (arg->receive_data && arg->receive_len) {
		uint8_t len;
		rc522_read_register(rc522, FIFOLevelReg, &len);
		if (len > arg->receive_len) {
			return STATUS_NO_ROOM;
		}
		arg->receive_len = len;
		rc522_read_register_many(rc522, FIFODataReg, len, arg->receive_data, arg->rx_align);

		rc522_read_register(rc522, ControlReg, &arg->valid_bits);
		arg->valid_bits &= 0x07;
	}

	if (error_reg_value & 0x08) { // CollErr
		return STATUS_COLLISION;
	}

	// TODO: CRC validation.
	
	return STATUS_OK;
}

int rc522_reqa(const struct spi_dt_spec *rc522, uint8_t *atqa) {
	uint8_t reqa[1] = {0x26};
	struct communicate_argument arg = {
		.command = Transceive,
		.wait_irq = 0x30, // RxIrq IdleIrq
		.transmit_data = reqa,
		.transmit_len = 1,
		.receive_data = atqa,
		.receive_len = 2,
		.valid_bits = 7
	};
	return rc522_communicate(rc522, &arg);
}
