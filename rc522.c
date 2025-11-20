#define DT_DRV_COMPAT nxp_rc522

#include "include/rfid.h"
#include <errno.h>
#include <rfid.h>
#include <stdint.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(dev, CONFIG_RC522_LOG_LEVEL);

#include "rc522.h"

static int rc522_read_register(const struct device *dev, const uint8_t reg,
                               uint8_t *read_value) {

  const struct rc522_config *config = dev->config;
  uint8_t tx_buf[2] = {0x80 | reg};
  struct spi_buf send_buf = {tx_buf, 2};
  struct spi_buf_set send_buf_set = {&send_buf, 1};

  uint8_t rx_buf[2];
  struct spi_buf receive_buf = {rx_buf, 2};
  struct spi_buf_set receive_buf_set = {&receive_buf, 1};

  int ret;
  ret = spi_transceive_dt(&config->spi, &send_buf_set, &receive_buf_set);
  *read_value = rx_buf[1];
  return ret;
}

static int rc522_write_register(const struct device *dev, const uint8_t reg,
                                const uint8_t write_value) {
  const struct rc522_config *config = dev->config;
  uint8_t tx_buf[2] = {reg, write_value};
  struct spi_buf send_buf = {tx_buf, 2};
  struct spi_buf_set send_buf_set = {&send_buf, 1};
  return spi_write_dt(&config->spi, &send_buf_set);
}
static int rc522_write_register_many(const struct device *dev,
                                     const uint8_t reg, const int length,
                                     const uint8_t *write_values) {
  const struct rc522_config *config = dev->config;
  uint8_t tx_buf[length + 1];
  tx_buf[0] = reg;
  memset(tx_buf + 1, 0, length);
  memcpy(tx_buf + 1, write_values, length);
  struct spi_buf send_buf = {tx_buf, length + 1};
  struct spi_buf_set send_buf_set = {&send_buf, 1};
  return spi_write_dt(&config->spi, &send_buf_set);
}

static int rc522_read_register_many(const struct device *dev, const uint8_t reg,
                                    const int length, uint8_t *read_values,
                                    const uint8_t rx_align) {
  const struct rc522_config *config = dev->config;
  if (length == 0)
    return 0;

  uint8_t tx_buf[length + 1];
  memset(tx_buf, 0x80 | reg, length);
  tx_buf[length] = 0;
  struct spi_buf send_buf = {tx_buf, length + 1};
  struct spi_buf_set send_buf_set = {&send_buf, 1};

  uint8_t rx_buf[length + 1];
  struct spi_buf receive_buf = {rx_buf, length + 1};
  struct spi_buf_set receive_buf_set = {&receive_buf, 1};

  int ret;
  ret = spi_transceive_dt(&config->spi, &send_buf_set, &receive_buf_set);

  memcpy(read_values + 1, rx_buf + 2, length - 1);
  // Copy all except first byte.

  // Handle alignment of first byte.
  uint8_t mask = (0xFF << rx_align) & 0xFF;
  read_values[0] = (read_values[0] & ~mask) | (rx_buf[1] & mask);

  return ret;
}

int rc522_read_version(const struct device *dev, uint8_t *read_value) {
  uint8_t ret;
  ret = rc522_read_register(dev, VersionReg, read_value);
  return ret;
}

/**
 * @brief Get firmware version as a string
 *
 * @param version Firmware version byte read from VersionReg
 * @return const char* String representation of the firmware version
 */
const char *get_firmware_version_str(uint8_t version) {
  switch (version) {
  case 0x88: /* Fudan Semiconductor FM17522 clone */
    return FM17522_VERSION_STR;

  case 0x90: /* Version 0.0 */
    return MFRC522_VERSION_V0_0_STR;

  case 0x91: /* Version 1.0 */
    return MFRC522_VERSION_V1_0_STR;

  case 0x92: /* Version 2.0 */
    return MFRC522_VERSION_V2_0_STR;

  default: /* Unknown version */
    return MFRC522_VERSION_UNKNOWN;
  }
}

static int rc522_init(const struct device *dev) {
  const struct rc522_config *config = dev->config;
  if (!spi_is_ready_dt(&config->spi)) {
    LOG_ERR("SPI bus device not ready");
    return -ENODEV;
  }

  rc522_write_register(dev, CommandReg, PCD_SoftReset);

  rc522_write_register(dev, TxModeReg, 0x00); // Enable CRC and use 106kBd.
  rc522_write_register(dev, RxModeReg, 0x00); // Enable CRC and use 106kBd.

  rc522_write_register(dev, ModWidthReg,
                       0x26); // Reset Miller modulation width.

  rc522_write_register(dev, TModeReg,
                       0x80); // Timer starts at end of transmission.
  rc522_write_register(dev, TPrescalerReg,
                       0xA9); // Set timer period to 25 us.
  rc522_write_register(dev, TReloadRegH, 0x03);
  rc522_write_register(dev, TReloadRegL, 0xE8); // Set timeout to 25 ms.

  rc522_write_register(dev, TxASKReg, 0x40); // Use 100% ASK
  rc522_write_register(dev, ModeReg, 0x3D);  // Set CRC preset to 6363h.

  uint8_t reg;
  rc522_read_register(dev, TxControlReg, &reg);
  rc522_write_register(dev, TxControlReg, reg | 0x03); // Enable antenna.

  uint8_t version;
  int err = rc522_read_version(dev, &version);
  if (err) {
    LOG_ERR("Unable to communicate with rc522 over spi: %d", err);
    return err;
  } else {
    LOG_DBG("%s", get_firmware_version_str(version));
  }
  LOG_DBG("init mfrc522 success");
  return 0;
}

int rc522_hw_test(const struct device *dev) {
  /* 1. Perform a soft reset. */
  rc522_write_register(dev, CommandReg, PCD_SoftReset);

  /* 2. Clear the internal buffer by writing 25 bytes of 00h and implement
  the config command. */
  uint8_t ZEROS[25] = {0};
  rc522_write_register(dev, FIFOLevelReg, 0x80); // Flush FIFO buffer

  /* Write 25 zeros to FIFO buffer 	rc522_write_register(dev,
   CommandReg, PCD_Mem); Transfer zeros to internal buffer */
  rc522_write_register_many(dev, FIFODataReg, 25, ZEROS);

  /* 3. Enable the self test by writing 09h to
   * the AutoTestReg register.
   */
  rc522_write_register(dev, AutoTestReg, 0x09);

  /* 4. Write 00h to the FIFO buffer. */
  rc522_write_register(dev, FIFODataReg, 0x00);

  /* 5. Start the self test with the CalcCRC command. */
  rc522_write_register(dev, CommandReg, PCD_CalcCRC);

  /* 6. The self test is initiated. */

  /* 7. When the self test has completed, the FIFO buffer contains the
  following 64 bytes: */
  // Wait until test is completed.
  uint8_t n;
  for (uint8_t i = 0; i < 0xFF; i++) {
    rc522_read_register(dev, FIFOLevelReg, &n);
    if (n >= 64) {
      break;
    }
  }
  rc522_write_register(dev, CommandReg, PCD_Idle);

  // Read data in FIFO buffer.
  uint8_t buffer[64];
  rc522_read_register_many(dev, FIFODataReg, 64, buffer, 0);

  rc522_write_register(dev, AutoTestReg, 0x00);

  /* Print data. */
  for (int i = 0; i < 8; i++) {
    for (int j = 0; j < 8; j++) {
      LOG_DBG("0x%02X ", buffer[8 * i + j]);
    }
    LOG_DBG("\n");
  }
  LOG_DBG("\n");
  LOG_INF("test okay");

  return 0;
}

static int rc522_crc(const struct device *dev, const uint8_t *data,
                     const uint8_t length, uint8_t *result) {

  rc522_write_register(dev, CommandReg, PCD_Idle);
  rc522_write_register(dev, DivIrqReg, 0x04);    // Clear CRCIRq bit.
  rc522_write_register(dev, FIFOLevelReg, 0x80); // Flush FIFO buffer.
  rc522_write_register_many(dev, FIFODataReg, length, data);
  rc522_write_register(dev, CommandReg, PCD_CalcCRC);

  uint32_t start = k_uptime_get_32();
  do {
    uint8_t irq;
    rc522_read_register(dev, DivIrqReg, &irq);
    if (irq & 0x04) {
      rc522_write_register(dev, CommandReg, PCD_Idle);
      rc522_read_register(dev, CRCResultRegL, result);
      rc522_read_register(dev, CRCResultRegH, result + 1);
      return 0;
    }
    k_yield();
  } while (k_uptime_get_32() - start < 89);

  return -ETIME;
}

static int rc522_communicate(const struct device *dev,
                             struct communicate_argument *arg) {
  uint8_t tx_last_bits = arg->valid_bits;
  uint8_t bit_framing = (arg->rx_align << 4) + tx_last_bits;

  rc522_write_register(dev, CommandReg, PCD_Idle);
  rc522_write_register(dev, ComIrqReg, 0x7f);    // Clear interrupt bits.
  rc522_write_register(dev, FIFOLevelReg, 0x80); // Flush FIFO buffer.
  rc522_write_register_many(dev, FIFODataReg, arg->transmit_len,
                            arg->transmit_data);
  rc522_write_register(dev, BitFramingReg, bit_framing);
  rc522_write_register(dev, CommandReg, arg->command);
  if (arg->command == PCD_Transceive) {
    rc522_write_register(dev, BitFramingReg, bit_framing | 0x80);
  }

  uint32_t start = k_uptime_get_32();
  int completed = 0;
  do {
    uint8_t irq;
    rc522_read_register(dev, ComIrqReg, &irq);
    if (irq & arg->wait_irq) {
      completed = 1;
      break;
    }
    if (irq & 0x01) { // 25 ms timer interrupt.
      return -ETIME;
    }
    k_yield();
  } while (k_uptime_get_32() - start < 36);

  if (!completed) {
    return -ETIME;
  }

  uint8_t error_reg_value;
  rc522_read_register(dev, ErrorReg, &error_reg_value);
  if (error_reg_value & 0x13) { // BufferOvfl ParityErr ProtocolErr
    return STATUS_ERROR;
  }

  if (arg->receive_data && arg->receive_len) {
    uint8_t len;
    rc522_read_register(dev, FIFOLevelReg, &len);
    rc522_read_register_many(dev, FIFODataReg, arg->receive_len,
                             arg->receive_data, arg->rx_align);
    arg->receive_len = len;

    rc522_read_register(dev, ControlReg, &arg->valid_bits);
    arg->valid_bits &= 0x07;
  }

  if (error_reg_value & 0x08) { // CollErr
    return STATUS_COLLISION;
  }

  if (arg->receive_data && arg->receive_len && arg->check_CRC) {
    if (arg->receive_len == 1 && arg->valid_bits == 4) {
      return STATUS_MIFARE_NACK;
    }

    if (arg->receive_len < 2 || arg->valid_bits != 0) {
      return STATUS_CRC_WRONG;
    }

    uint8_t CRC[2];
    rc522_crc(dev, arg->receive_data, arg->receive_len - 2, CRC);

    if ((CRC[0] != arg->receive_data[arg->receive_len - 2]) ||
        (CRC[1] != arg->receive_data[arg->receive_len - 1])) {
      return STATUS_CRC_WRONG;
    }
  }

  return 0;
}

// max 64 bytes
static char *format_bytes(const uint8_t length, const uint8_t *bytes) {
  static char hex_string[64]; // Adjust size as needed
  char *ptr = hex_string;

  for (int i = 0; i < length; i++) {
    ptr += sprintf(ptr, "%02X ", bytes[i]);
  }
  return hex_string;
}

void rc522_print_status(const uint8_t status_code) {
  switch (status_code) {
  case 0:
    LOG_DBG("0");
    break;
  case STATUS_ERROR:
    LOG_DBG("STATUS_ERROR");
    break;
  case STATUS_COLLISION:
    LOG_DBG("STATUS_COLLISION");
    break;
  case -ETIME:
    LOG_DBG("-ETIME");
    break;
  case STATUS_NO_ROOM:
    LOG_DBG("STATUS_NO_ROOM");
    break;
  case STATUS_INTERNAL_ERROR:
    LOG_DBG("STATUS_INTERNAL_ERROR");
    break;
  case -EIO:
    LOG_DBG("-EIO");
    break;
  case STATUS_CRC_WRONG:
    LOG_DBG("STATUS_CRC_WRONG");
    break;
  case STATUS_MIFARE_NACK:
    LOG_DBG("STATUS_MIFARE_NACK");
    break;
  }
}

int rc522_fob_reqa(const struct device *dev) {
  uint8_t reqa[1] = {PICC_CMD_REQA};
  uint8_t atqa[2] = {0};
  struct communicate_argument arg = {.command = PCD_Transceive,
                                     .wait_irq = 0x20, // RxIRq
                                     .transmit_data = reqa,
                                     .transmit_len = 1,
                                     .receive_data = atqa,
                                     .receive_len = 2,
                                     .valid_bits = 7};
  LOG_HEXDUMP_DBG(reqa, 1, "-> REQA:");
  int ret;
  ret = rc522_communicate(dev, &arg);
  LOG_HEXDUMP_DBG(atqa, 2, "<- ATQA:");
  rc522_print_status(ret);
  return ret;
}

int rc522_select(const struct device *dev, uint8_t *UID,
                 const uint8_t UID_valid, uint8_t *sak) {
  uint8_t select[9];
  select[0] = PICC_CMD_SEL_CL1;
  if (UID_valid) {
    select[1] = 0x70;
    select[2] = UID[0];
    select[3] = UID[1];
    select[4] = UID[2];
    select[5] = UID[3];
    select[6] = select[2] ^ select[3] ^ select[4] ^ select[5];
    rc522_crc(dev, select, 7, select + 7);
  } else {
    select[1] = 0x20;
  }

  uint8_t local_sak[1] = {0};
  if (UID_valid && sak == NULL) {
    sak = local_sak;
  }

  struct communicate_argument arg = {
      .command = PCD_Transceive,
      .wait_irq = 0x20, // RxIRq
      .transmit_data = select,
      .transmit_len = UID_valid ? 9 : 2,
      .receive_data = UID_valid ? sak : UID,
      .receive_len = UID_valid ? 1 : 5,
  };
  LOG_HEXDUMP_DBG(select, UID_valid ? 9 : 2, "->");
  int ret;
  ret = rc522_communicate(dev, &arg);
  LOG_HEXDUMP_DBG(UID_valid ? sak : UID, UID_valid ? 1 : 5, "<-");
  rc522_print_status(ret);
  return ret;
}

int rc522_mifare_auth(const struct device *dev, const uint8_t block_addr,
                      const uint8_t *key, const uint8_t *UID) {
  uint8_t auth[12];
  auth[0] = PICC_CMD_MF_AUTH_KEY_A;
  auth[1] = block_addr;
  memcpy(auth + 2, key, 6);
  memcpy(auth + 8, UID, 4);

  struct communicate_argument arg = {
      .command = PCD_MFAuthent,
      .wait_irq = 0x10, // IdleIRq
      .transmit_data = auth,
      .transmit_len = 12,
  };

  LOG_DBG("-> Auth:\t");
  format_bytes(12, auth);
  int ret;
  ret = rc522_communicate(dev, &arg);
  if (ret == 0) {
    LOG_DBG("<- OK!\n");
  }
  rc522_print_status(ret);
  return ret;
}

int rc522_mifare_deauth(const struct device *dev) {
  uint8_t reg;
  rc522_read_register(dev, Status2Reg, &reg);
  rc522_write_register(dev, Status2Reg, reg & 0xF7);
  return 0;
}

int rc522_mifare_read(const struct device *dev, const uint8_t block_addr,
                      uint8_t *length, uint8_t *read_values) {
  if (read_values == NULL || *length < 18) {
    return STATUS_NO_ROOM;
  }

  read_values[0] = PICC_CMD_MF_READ;
  read_values[1] = block_addr;
  rc522_crc(dev, read_values, 2, read_values + 2);

  struct communicate_argument arg = {.command = PCD_Transceive,
                                     .wait_irq = 0x20, // RxIRq
                                     .transmit_data = read_values,
                                     .transmit_len = 4,
                                     .receive_data = read_values,
                                     .receive_len = *length,
                                     .check_CRC = 1};
  LOG_DBG("-> Read:\t");
  format_bytes(4, read_values);
  int ret;
  ret = rc522_communicate(dev, &arg);
  *length = arg.receive_len;
  LOG_DBG("<- Data:\t");
  format_bytes(*length, read_values);
  rc522_print_status(ret);
  return ret;
}

static int rc522_mifare_transceive(const struct device *dev,
                                   const uint8_t length, const uint8_t *data,
                                   const int accept_timeout) {
  uint8_t buffer[18];
  memcpy(buffer, data, length);
  rc522_crc(dev, buffer, length, buffer + length);

  struct communicate_argument arg = {.command = PCD_Transceive,
                                     .wait_irq = 0x20, // RxIRq
                                     .transmit_data = buffer,
                                     .transmit_len = length + 2,
                                     .receive_data = buffer,
                                     .receive_len = 18};
  LOG_DBG("-> Cmd:\t\t");
  format_bytes(length + 2, buffer);
  int ret;
  ret = rc522_communicate(dev, &arg);
  LOG_DBG("<- ACK:\t\t");
  format_bytes(arg.receive_len, buffer);
  if (accept_timeout && ret == -ETIME) {
    return 0;
  }
  if (ret != 0) {
    return ret;
  }
  if (arg.receive_len != 1 || arg.valid_bits != 4) {
    return STATUS_ERROR;
  }
  if (buffer[0] != MF_ACK) {
    return STATUS_MIFARE_NACK;
  }
  return 0;
}

int rc522_mifare_write(const struct device *dev, const uint8_t block_addr,
                       const uint8_t length, const uint8_t *data) {
  if (data == NULL || length != 16) {
    LOG_ERR("not data or wrong length");
    return -EINVAL;
  }
  uint8_t write[2];
  write[0] = PICC_CMD_MF_WRITE;
  write[1] = block_addr;

  int ret;
  ret = rc522_mifare_transceive(dev, 2, write, 0);
  if (ret != 0) {
    rc522_print_status(ret);
    return ret;
  }

  ret = rc522_mifare_transceive(dev, length, data, 0);
  rc522_print_status(ret);
  return ret;
}

static const struct rfid_driver_api rc522_api = {
    .hw_test = &rc522_hw_test,
    .fob_reqa = &rc522_fob_reqa,
    .fob_select = &rc522_select,
};

#define RC522_DEFINE(inst)                                                     \
  static const struct rc522_config rc522_config_##inst = {                     \
      .spi = SPI_DT_SPEC_INST_GET(inst,                                        \
                                  SPI_OP_MODE_MASTER | SPI_TRANSFER_MSB |      \
                                      SPI_WORD_SET(8) | SPI_LINES_SINGLE,      \
                                  0)};                                         \
  DEVICE_DT_INST_DEFINE(inst, rc522_init, NULL, NULL, &rc522_config_##inst,    \
                        POST_KERNEL, CONFIG_RC522_INIT_PRIORITY, &rc522_api);

DT_INST_FOREACH_STATUS_OKAY(RC522_DEFINE)
