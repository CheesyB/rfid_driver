#include <rc522.h>
#include <stdint.h>
#include <zephyr/drivers/spi.h>


#define DT_DRV_COMPAT nxp_rc522 

static int rc522_init(const struct device *dev)
{
    const struct rc522_config *config = dev->config;

    if (!device_is_ready(config->spi.bus)) {
        LOG_ERR("SPI bus device not ready");
        return -ENODEV;
    }

    uint8_t version;
    int err = read_version(dev, &version);
    if (err) {
        LOG_ERR("Unable to communicate with rc522 over spi: %d", err);
        return err;
    } else {
        LOG_INF("rc522 firmware version: %s", buf);
    }

    return 0;
}


static int rc522_read_register(const struct spi_dt_spec *rc522, const uint8_t
reg, uint8_t *read_value) { 	uint8_t tx_buf[2] = {0x80 | reg}; 	struct spi_buf
send_buf = {tx_buf, 2}; 	struct spi_buf_set send_buf_set = {&send_buf, 1};

	uint8_t rx_buf[2];
	struct spi_buf receive_buf = {rx_buf, 2};
	struct spi_buf_set receive_buf_set = {&receive_buf, 1};

	int ret;
	ret = spi_transceive_dt(rc522, &send_buf_set, &receive_buf_set);
	*read_value = rx_buf[1];
	return ret;
}


static int rc522_write_register(const struct spi_dt_spec *rc522,
                                const uint8_t reg, const uint8_t write_value) {
  uint8_t tx_buf[2] = {reg, write_value};
  struct spi_buf send_buf = {tx_buf, 2};
  struct spi_buf_set send_buf_set = {&send_buf, 1};
  return spi_write_dt(rc522, &send_buf_set);
}


int read_version(const struct device *dev) {
	uint8_t ret;
  uint8_t version;
	ret = rc522_read_register(dev, VersionReg, &version);
  return version;
}

static const struct rc522_driver_api rc522_api = {
    .rc522_read_version = &read_version,
};


#define RC522_DEFINE(inst)                                      \
    static const struct rc522_config rc522_config_##inst = { \
        .spi = SPI_DT_SPEC_INST_GET(inst),                         \
    };                                                             \
                                                                   \
    DEVICE_DT_INST_DEFINE(inst,                                    \
                  rc522_init,                                   \
                  NULL,                                            \
                  NULL,                                            \
                  &rc522_config_##inst,                         \
                  POST_KERNEL,                                     \
                  CONFIG_rc522_INIT_PRIORITY,                   \
                  &rc522_api);

DT_INST_FOREACH_STATUS_OKAY(rc522_DEFINE)

// static int rc522_write_register_many(const struct spi_dt_spec *rc522, const
// uint8_t reg, const int length, const uint8_t *write_values) { 	uint8_t
// tx_buf[length+1]; 	tx_buf[0] = reg; 	memset(tx_buf+1, 0, length);
// 	memcpy(tx_buf+1, write_values, length);
// 	struct spi_buf send_buf = {tx_buf, length+1};
// 	struct spi_buf_set send_buf_set = {&send_buf, 1};
// 	return spi_write_dt(rc522, &send_buf_set);
// }
//
// static int rc522_read_register_many(const struct spi_dt_spec *rc522, const
// uint8_t reg, const int length, uint8_t *read_values, const uint8_t rx_align)
// { 	if (length == 0) return 0;
//
// 	uint8_t tx_buf[length+1];
// 	memset(tx_buf, 0x80 | reg, length);
// 	tx_buf[length] = 0;
// 	struct spi_buf send_buf = {tx_buf, length+1};
// 	struct spi_buf_set send_buf_set = {&send_buf, 1};
//
// 	uint8_t rx_buf[length+1];
// 	struct spi_buf receive_buf = {rx_buf, length+1};
// 	struct spi_buf_set receive_buf_set = {&receive_buf, 1};
//
// 	int ret;
// 	ret = spi_transceive_dt(rc522, &send_buf_set, &receive_buf_set);
//
// 	memcpy(read_values+1, rx_buf+2, length-1); // Copy all except first
// byte.
//
// 	// Handle alignment of first byte.
// 	uint8_t mask = (0xFF << rx_align) & 0xFF;
// 	read_values[0] = (read_values[0] & ~mask) | (rx_buf[1] & mask);
//
// 	return ret;
// }
//
// int rc522_test(const struct spi_dt_spec *rc522) {
// 	/* 1. Perform a soft reset. */
// 	rc522_write_register(rc522, CommandReg, PCD_SoftReset);
//
// 	/* 2. Clear the internal buffer by writing 25 bytes of 00h and implement
// the config command. */ 	uint8_t ZEROS[25] = {0}; 	rc522_write_register(rc522,
// FIFOLevelReg, 0x80); // Flush FIFO buffer 	rc522_write_register_many(rc522,
// FIFODataReg, 25, ZEROS); // Write 25 zeros to FIFO buffer
// 	rc522_write_register(rc522, CommandReg, PCD_Mem); // Transfer zeros to
// internal buffer
//
// 	/* 3. Enable the self test by writing 09h to the AutoTestReg register.
// */ 	rc522_write_register(rc522, AutoTestReg, 0x09);
//
// 	/* 4. Write 00h to the FIFO buffer. */
// 	rc522_write_register(rc522, FIFODataReg, 0x00);
//
// 	/* 5. Start the self test with the CalcCRC command. */
// 	rc522_write_register(rc522, CommandReg, PCD_CalcCRC);
//
// 	/* 6. The self test is initiated. */
//
// 	/* 7. When the self test has completed, the FIFO buffer contains the
// following 64 bytes: */
// 	// Wait until test is completed.
// 	uint8_t n;
// 	for (uint8_t i = 0; i < 0xFF; i++) {
// 		rc522_read_register(rc522, FIFOLevelReg, &n);
// 		if (n >= 64) {
// 			break;
// 		}
// 	}
// 	rc522_write_register(rc522, CommandReg, PCD_Idle);
//
// 	// Read data in FIFO buffer.
// 	uint8_t buffer[64];
// 	rc522_read_register_many(rc522, FIFODataReg, 64, buffer, 0);
//
// 	rc522_write_register(rc522, AutoTestReg, 0x00);
//
// 	/* Print data. */
// 	for (int i = 0; i < 8; i++) {
// 		for (int j = 0; j < 8; j++) {
// 			printk("0x%02X ", buffer[8*i+j]);
// 		}
// 		printk("\n");
// 	}
// 	printk("\n");
//
// 	return STATUS_OK;
// }
//
// int rc522_init(const struct spi_dt_spec *rc522) {
// 	rc522_write_register(rc522, CommandReg, PCD_SoftReset);
// 	/*uint8_t count = 0;
// 	do {
// 		k_msleep(50);
// 		uint8_t reg;
// 		rc522_read_register(rc522, CommandReg, &reg);
// 		if (!(reg & 0x10)) {
// 			break;
// 		}
// 	} while (++count < 3);*/
//
// 	rc522_write_register(rc522, TxModeReg, 0x00); // Enable CRC and use
// 106kBd. 	rc522_write_register(rc522, RxModeReg, 0x00); // Enable CRC and use
// 106kBd.
//
// 	rc522_write_register(rc522, ModWidthReg, 0x26); // Reset Miller
// modulation width.
//
// 	rc522_write_register(rc522, TModeReg, 0x80); // Timer starts at end of
// transmission. 	rc522_write_register(rc522, TPrescalerReg, 0xA9); // Set timer
// period to 25 us. 	rc522_write_register(rc522, TReloadRegH, 0x03);
// 	rc522_write_register(rc522, TReloadRegL, 0xE8); // Set timeout to 25 ms.
//
// 	rc522_write_register(rc522, TxASKReg, 0x40); // Use 100% ASK
// 	rc522_write_register(rc522, ModeReg, 0x3D); // Set CRC preset to 6363h.
//
// 	uint8_t reg;
// 	rc522_read_register(rc522, TxControlReg, &reg);
// 	rc522_write_register(rc522, TxControlReg, reg | 0x03); // Enable
// antenna.
//
// 	return STATUS_OK;
// }
//
// static int rc522_crc(const struct spi_dt_spec *rc522, const uint8_t *data,
// const uint8_t length, uint8_t *result) { 	rc522_write_register(rc522,
// CommandReg, PCD_Idle); 	rc522_write_register(rc522, DivIrqReg, 0x04); // Clear
// CRCIRq bit. 	rc522_write_register(rc522, FIFOLevelReg, 0x80); // Flush FIFO
// buffer. 	rc522_write_register_many(rc522, FIFODataReg, length, data);
// 	rc522_write_register(rc522, CommandReg, PCD_CalcCRC);
//
// 	uint32_t start = k_uptime_get_32();
// 	do {
// 		uint8_t irq;
// 		rc522_read_register(rc522, DivIrqReg, &irq);
// 		if (irq & 0x04) {
// 			rc522_write_register(rc522, CommandReg, PCD_Idle);
// 			rc522_read_register(rc522, CRCResultRegL, result);
// 			rc522_read_register(rc522, CRCResultRegH, result+1);
// 			return STATUS_OK;
// 		}
// 		k_yield();
// 	} while (k_uptime_get_32() - start < 89);
//
// 	return STATUS_TIMEOUT;
// }
//
// static int rc522_communicate(const struct spi_dt_spec *rc522, struct
// communicate_argument *arg) { 	uint8_t tx_last_bits = arg->valid_bits; 	uint8_t
// bit_framing = (arg->rx_align << 4) + tx_last_bits;
//
// 	rc522_write_register(rc522, CommandReg, PCD_Idle);
// 	rc522_write_register(rc522, ComIrqReg, 0x7f); // Clear interrupt bits.
// 	rc522_write_register(rc522, FIFOLevelReg, 0x80); // Flush FIFO buffer.
// 	rc522_write_register_many(rc522, FIFODataReg, arg->transmit_len,
// arg->transmit_data); 	rc522_write_register(rc522, BitFramingReg, bit_framing);
// 	rc522_write_register(rc522, CommandReg, arg->command);
// 	if (arg->command == PCD_Transceive) {
// 		rc522_write_register(rc522, BitFramingReg, bit_framing | 0x80);
// 	}
//
// 	uint32_t start = k_uptime_get_32();
// 	int completed = 0;
// 	do {
// 		uint8_t irq;
// 		rc522_read_register(rc522, ComIrqReg, &irq);
// 		if (irq & arg->wait_irq) {
// 			completed = 1;
// 			break;
// 		}
// 		if (irq & 0x01) { // 25 ms timer interrupt.
// 			return STATUS_TIMEOUT;
// 		}
// 		/*uint8_t reg;
// 		rc522_read_register(rc522, CommandReg, &reg);
// 		printk("Reg: %02X\n", reg);*/
// 		k_yield();
// 	} while (k_uptime_get_32() - start < 36);
//
// 	if (!completed) {
// 		return STATUS_TIMEOUT;
// 	}
//
// 	uint8_t error_reg_value;
// 	rc522_read_register(rc522, ErrorReg, &error_reg_value);
// 	if (error_reg_value & 0x13) { // BufferOvfl ParityErr ProtocolErr
// 		return STATUS_ERROR;
// 	}
//
// 	if (arg->receive_data && arg->receive_len) {
// 		uint8_t len;
// 		rc522_read_register(rc522, FIFOLevelReg, &len);
// 		/*if (len > arg->receive_len) {
// 			return STATUS_NO_ROOM;
// 		}*/
// 		rc522_read_register_many(rc522, FIFODataReg, arg->receive_len,
// arg->receive_data, arg->rx_align); 		arg->receive_len = len;
//
// 		rc522_read_register(rc522, ControlReg, &arg->valid_bits);
// 		arg->valid_bits &= 0x07;
// 	}
//
// 	if (error_reg_value & 0x08) { // CollErr
// 		return STATUS_COLLISION;
// 	}
//
// 	if (arg->receive_data && arg->receive_len && arg->check_CRC) {
// 		if (arg->receive_len == 1 && arg->valid_bits == 4) {
// 			return STATUS_MIFARE_NACK;
// 		}
//
// 		if (arg->receive_len < 2 || arg->valid_bits != 0) {
// 			return STATUS_CRC_WRONG;
// 		}
//
// 		uint8_t CRC[2];
// 		rc522_crc(rc522, arg->receive_data, arg->receive_len-2, CRC);
//
// 		if ((CRC[0] != arg->receive_data[arg->receive_len-2])
// 		 	|| (CRC[1] != arg->receive_data[arg->receive_len-1])) {
// 			return STATUS_CRC_WRONG;
// 		}
// 	}
//
// 	return STATUS_OK;
// }
//
// static void print_bytes(const uint8_t length, const uint8_t *bytes) {
// 	for (int i = 0; i < length; i++) {
// 		printk("%02X ", bytes[i]);
// 	}
// 	printk("\n");
// }
//
// void rc522_print_status(const uint8_t status_code) {
// 	switch(status_code) {
// 		case STATUS_OK:
// 			printk("STATUS_OK\n");
// 			break;
// 		case STATUS_ERROR:
// 			printk("STATUS_ERROR\n");
// 			break;
// 		case STATUS_COLLISION:
// 			printk("STATUS_COLLISION\n");
// 			break;
// 		case STATUS_TIMEOUT:
// 			printk("STATUS_TIMEOUT\n");
// 			break;
// 		case STATUS_NO_ROOM:
// 			printk("STATUS_NO_ROOM\n");
// 			break;
// 		case STATUS_INTERNAL_ERROR:
// 			printk("STATUS_INTERNAL_ERROR\n");
// 			break;
// 		case STATUS_INVALID:
// 			printk("STATUS_INVALID\n");
// 			break;
// 		case STATUS_CRC_WRONG:
// 			printk("STATUS_CRC_WRONG\n");
// 			break;
// 		case STATUS_MIFARE_NACK:
// 			printk("STATUS_MIFARE_NACK\n");
// 			break;
// 	}
// }
//
// int rc522_reqa(const struct spi_dt_spec *rc522) {
// 	uint8_t reqa[1] = {PICC_CMD_REQA};
// 	uint8_t atqa[2] = {0};
// 	struct communicate_argument arg = {
// 		.command = PCD_Transceive,
// 		.wait_irq = 0x20, // RxIRq
// 		.transmit_data = reqa,
// 		.transmit_len = 1,
// 		.receive_data = atqa,
// 		.receive_len = 2,
// 		.valid_bits = 7
// 	};
// 	printk("-> REQA:\t");
// 	print_bytes(1, reqa);
// 	int ret;
// 	ret = rc522_communicate(rc522, &arg);
// 	printk("<- ATQA:\t");
// 	print_bytes(2, atqa);
// 	rc522_print_status(ret);
// 	return ret;
// }
//
// int rc522_select(const struct spi_dt_spec *rc522, uint8_t *UID, const uint8_t
// UID_valid, uint8_t *sak) { 	uint8_t select[9]; 	select[0] = PICC_CMD_SEL_CL1;
// 	if (UID_valid) {
// 		select[1] = 0x70;
// 		select[2] = UID[0];
// 		select[3] = UID[1];
// 		select[4] = UID[2];
// 		select[5] = UID[3];
// 		select[6] = select[2] ^ select[3] ^ select[4] ^ select[5];
// 		rc522_crc(rc522, select, 7, select+7);
// 	} else {
// 		select[1] = 0x20;
// 	}
//
// 	uint8_t local_sak[1] = {0};
// 	if (UID_valid && sak == NULL) {
// 		sak = local_sak;
// 	}
//
// 	struct communicate_argument arg = {
// 		.command = PCD_Transceive,
// 		.wait_irq = 0x20, // RxIRq
// 		.transmit_data = select,
// 		.transmit_len = UID_valid ? 9 : 2,
// 		.receive_data = UID_valid ? sak : UID,
// 		.receive_len = UID_valid ? 1 : 5,
// 	};
// 	printk("-> Select:\t");
// 	print_bytes(UID_valid ? 9 : 2, select);
// 	int ret;
// 	ret = rc522_communicate(rc522, &arg);
// 	printk("<- %s:\t\t", UID_valid ? "SAK" : "UID");
// 	print_bytes(UID_valid ? 1 : 5, UID_valid ? sak : UID);
// 	rc522_print_status(ret);
// 	return ret;
// }
//
// int rc522_mifare_auth(const struct spi_dt_spec *rc522, const uint8_t
// block_addr, const uint8_t *key, const uint8_t *UID) { 	uint8_t auth[12];
// 	auth[0] = PICC_CMD_MF_AUTH_KEY_A;
// 	auth[1] = block_addr;
// 	memcpy(auth+2, key, 6);
// 	memcpy(auth+8, UID, 4);
//
// 	struct communicate_argument arg = {
// 		.command = PCD_MFAuthent,
// 		.wait_irq = 0x10, // IdleIRq
// 		.transmit_data = auth,
// 		.transmit_len = 12,
// 	};
//
// 	printk("-> Auth:\t");
// 	print_bytes(12, auth);
// 	int ret;
// 	ret = rc522_communicate(rc522, &arg);
// 	if (ret == STATUS_OK) {
// 		printk("<- OK!\n");
// 	}
// 	rc522_print_status(ret);
// 	return ret;
// }
//
// int rc522_mifare_deauth(const struct spi_dt_spec *rc522) {
// 	uint8_t reg;
// 	rc522_read_register(rc522, Status2Reg, &reg);
// 	rc522_write_register(rc522, Status2Reg, reg & 0xF7);
// 	return STATUS_OK;
// }
//
// int rc522_mifare_read(const struct spi_dt_spec *rc522, const uint8_t
// block_addr, uint8_t *length, uint8_t *read_values) { 	if (read_values == NULL
// || *length < 18) { 		return STATUS_NO_ROOM;
// 	}
//
// 	read_values[0] = PICC_CMD_MF_READ;
// 	read_values[1] = block_addr;
// 	rc522_crc(rc522, read_values, 2, read_values+2);
//
// 	struct communicate_argument arg = {
// 		.command = PCD_Transceive,
// 		.wait_irq = 0x20, // RxIRq
// 		.transmit_data = read_values,
// 		.transmit_len = 4,
// 		.receive_data = read_values,
// 		.receive_len = *length,
// 		.check_CRC = 1
// 	};
// 	printk("-> Read:\t");
// 	print_bytes(4, read_values);
// 	int ret;
// 	ret = rc522_communicate(rc522, &arg);
// 	*length = arg.receive_len;
// 	printk("<- Data:\t");
// 	print_bytes(*length, read_values);
// 	rc522_print_status(ret);
// 	return ret;
// }
//
// static int rc522_mifare_transceive(const struct spi_dt_spec *rc522, const
// uint8_t length, const uint8_t *data, const int accept_timeout) { 	uint8_t
// buffer[18]; 	memcpy(buffer, data, length); 	rc522_crc(rc522, buffer, length,
// buffer+length);
//
// 	struct communicate_argument arg = {
// 		.command = PCD_Transceive,
// 		.wait_irq = 0x20, // RxIRq
// 		.transmit_data = buffer,
// 		.transmit_len = length+2,
// 		.receive_data = buffer,
// 		.receive_len = 18
// 	};
// 	printk("-> Cmd:\t\t");
// 	print_bytes(length+2, buffer);
// 	int ret;
// 	ret = rc522_communicate(rc522, &arg);
// 	printk("<- ACK:\t\t");
// 	print_bytes(arg.receive_len, buffer);
// 	if (accept_timeout && ret == STATUS_TIMEOUT) {
// 		return STATUS_OK;
// 	}
// 	if (ret != STATUS_OK) {
// 		return ret;
// 	}
// 	if (arg.receive_len != 1 || arg.valid_bits != 4) {
// 		return STATUS_ERROR;
// 	}
// 	if (buffer[0] != MF_ACK) {
// 		return STATUS_MIFARE_NACK;
// 	}
// 	return STATUS_OK;
// }
//
// int rc522_mifare_write(const struct spi_dt_spec *rc522, const uint8_t
// block_addr, const uint8_t length, const uint8_t *data) { 	if (data == NULL ||
// length != 16) { 		return STATUS_INVALID;
// 	}
// 	uint8_t write[2];
// 	write[0] = PICC_CMD_MF_WRITE;
// 	write[1] = block_addr;
//
// 	int ret;
// 	ret = rc522_mifare_transceive(rc522, 2, write, 0);
// 	if (ret != STATUS_OK) {
// 		rc522_print_status(ret);
// 		return ret;
// 	}
//
// 	ret = rc522_mifare_transceive(rc522, length, data, 0);
// 	rc522_print_status(ret);
// 	return ret;
// }
