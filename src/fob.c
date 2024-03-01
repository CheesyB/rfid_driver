#include "fob.h"

int fob_select(const struct spi_dt_spec *rc522, uint8_t *UID) {
	int ret;
	ret = rc522_select(rc522, UID, 0, NULL);
	if (ret != STATUS_OK) {
		return ret;
	}
	return rc522_select(rc522, UID, 1, NULL);
}

int fob_auth(const struct spi_dt_spec *rc522, const uint8_t *UID, const uint8_t sector, const uint8_t *key) {
	uint8_t block_addr = 4 * sector + 3;
	return rc522_mifare_auth(rc522, block_addr, key, UID);
}

int fob_write_key(const struct spi_dt_spec *rc522, const uint8_t *UID, const uint8_t sector, const uint8_t *key) {
	int ret;

	uint8_t block_addr = 4 * sector + 3;

	uint8_t length = 18;
	uint8_t read_values[18] = {0};
	ret = rc522_mifare_read(rc522, block_addr, &length, read_values);
	if (ret != STATUS_OK) {
		return ret;
	}

	memcpy(read_values, key, 6);
	memcpy(read_values+10, key, 6);

	ret = rc522_mifare_write(rc522, block_addr, 16, read_values);
	if (ret != STATUS_OK) {
		return ret;
	}

	return STATUS_OK;
}

int fob_write_passcode(const struct spi_dt_spec *rc522, const uint8_t *UID, const uint8_t sector, const uint8_t *passcode) {
	int ret;

	uint8_t block_addr = 4 * sector + 2;

	uint8_t write_values[16] = {0};
	memcpy(write_values, passcode, 6);

	ret = rc522_mifare_write(rc522, block_addr, 16, write_values);
	if (ret != STATUS_OK) {
		return ret;
	}
	
	return STATUS_OK;
}

int fob_read_passcode(const struct spi_dt_spec *rc522, const uint8_t *UID, const uint8_t sector, uint8_t *passcode) {
	int ret;

	uint8_t block_addr = 4 * sector + 2;

	uint8_t length = 18;
	uint8_t read_values[18] = {0};
	ret = rc522_mifare_read(rc522, block_addr, &length, read_values);
	if (ret != STATUS_OK) {
		return ret;
	}

	memcpy(passcode, read_values, 6);
	
	return STATUS_OK;
}
